'use strict';
const $ = id => document.getElementById(id);
const image = $('image'), canvas = $('overlay'), ctx = canvas.getContext('2d');
const display = image.getContext('2d');
const cache = new Map();
let path = [], busy = false, configBusy = false, ready = false, anchor = null, selection = null, current = null;
let renderController = null;
$('cancel').onclick = () => {
  if (!renderController || renderController.signal.aborted) return;
  renderController.abort();
  $('cancel').disabled = true;
  $('status').textContent = 'Berechnung wird abgebrochen …';
};
const PALETTE_SIZE = 768, BLACK = 768;
const palette = new Uint8Array((PALETTE_SIZE+1)*3), colorIndices = new Map();
for (let c=0; c<PALETTE_SIZE; c++) {
  const f=c%256, k=Math.floor(c/256);
  const r=k===0?f:k===1?255-f:0;
  const g=k===1?f:k===2?255-f:0;
  const b=k===2?f:k===0?255-f:0;
  palette.set([r,g,b], c*3);
  // The renderer uses (iteration * 9) % 768: only multiples of three occur.
  // Their RGB values are unique, even at the duplicated segment endpoints.
  if (c%3===0) colorIndices.set((r<<16)|(g<<8)|b, c);
}
colorIndices.set(0, BLACK);
const visibleIndices = new Uint16Array(640*480).fill(BLACK);
let paletteOffset = 0, rotating = false, rotationFrame = null, rotationTime = null;
function paintRows(y=0, count=480) {
  const pixels = display.createImageData(640, count), shift = Math.floor(paletteOffset);
  for (let i=0; i<640*count; i++) {
    const index = visibleIndices[y*640+i];
    const c = (index===BLACK ? BLACK : (index+shift)%PALETTE_SIZE)*3;
    pixels.data[i*4]=palette[c]; pixels.data[i*4+1]=palette[c+1];
    pixels.data[i*4+2]=palette[c+2]; pixels.data[i*4+3]=255;
  }
  display.putImageData(pixels, 0, y);
}
function setPalette(offset) {
  paletteOffset = ((offset % PALETTE_SIZE) + PALETTE_SIZE) % PALETTE_SIZE;
  $('palette').value = String(Math.floor(paletteOffset));
  $('palette-value').textContent = `${Math.floor(paletteOffset)} / 768`;
  paintRows();
}
function animatePalette(time) {
  if (!rotating) return;
  if (rotationTime === null) rotationTime = time;
  if (time-rotationTime >= 33) {
    setPalette(paletteOffset + Math.min(time-rotationTime, 100)*0.096*Number($('palette-direction').value));
    rotationTime = time;
  }
  rotationFrame = requestAnimationFrame(animatePalette);
}
function stopPalette() {
  rotating = false;
  if (rotationFrame !== null) cancelAnimationFrame(rotationFrame);
  rotationFrame = rotationTime = null;
  $('palette-play').textContent = 'Rotation starten';
  $('palette-play').setAttribute('aria-pressed', 'false');
}
$('palette').oninput = () => { stopPalette(); setPalette(Number($('palette').value)); };
$('palette-reset').onclick = () => { stopPalette(); setPalette(0); };
$('palette-play').onclick = () => {
  if (rotating) { stopPalette(); return; }
  rotating = true;
  $('palette-play').textContent = 'Rotation pausieren';
  $('palette-play').setAttribute('aria-pressed', 'true');
  rotationFrame = requestAnimationFrame(animatePalette);
};
$('download').onclick = event => {
  if (busy || !ready) { event.preventDefault(); return; }
  if (Math.floor(paletteOffset) === 0) return;
  event.preventDefault();
  // Freeze the currently displayed palette, even while animation continues.
  const snapshot = document.createElement('canvas');
  snapshot.width = 640; snapshot.height = 480;
  const offset = Math.floor(paletteOffset);
  snapshot.getContext('2d').drawImage(image, 0, 0);
  snapshot.toBlob(blob => {
    if (!blob) { $('status').textContent = 'PNG-Export fehlgeschlagen.'; return; }
    const url = URL.createObjectURL(blob), link = document.createElement('a');
    link.href = url; link.download = `apfelmaennchen-palette-${offset}.png`;
    link.click();
    setTimeout(() => URL.revokeObjectURL(url), 60000);
  }, 'image/png');
};
function validIterations(value) {
  return Number.isInteger(value) && value >= 50 && value <= 100000;
}
function setIterations(value) {
  const select = $('iterations'), text = String(value);
  if (!Array.from(select.options).some(option => option.value === text)) {
    const option = document.createElement('option'); option.value = text; option.textContent = text; select.add(option);
  }
  select.value = text;
  $('iterations-manual').value = text;
}
$('iterations').onchange = () => setIterations(Number($('iterations').value));
$('iterations-manual').oninput = () => {
  const value = Number($('iterations-manual').value);
  if (validIterations(value)) setIterations(value);
};
$('iterations-manual').onkeydown = event => {
  if (event.key === 'Enter') { event.preventDefault(); return render([...path], null, true); }
};
function controls() {
  const locked = busy || configBusy;
  for (const id of ['reset', 'refresh', 'iterations', 'iterations-manual', 'backend', 'config-load']) $(id).disabled = locked;
  $('config-save').disabled = locked || !ready;
  $('cancel').disabled = !busy || !renderController || renderController.signal.aborted;
  $('back').disabled = locked || !path.length;
  $('download').hidden = locked || !ready;
  canvas.classList.toggle('busy', busy);
}
function show(entry) { visibleIndices.set(entry.indices); paintRows(); }
function bytes64(value) {
  return Uint8Array.from(atob(value), character => character.charCodeAt(0));
}
async function streamEntry(body, signal) {
  const response = await fetch('/render/stream', {method: 'POST', headers: {'Content-Type': 'application/json'}, body, signal});
  signal.throwIfAborted();
  if (!response.ok) throw new Error(await response.text());
  const reader = response.body.getReader(), decoder = new TextDecoder();
  const seen = new Set();
  let pending = '', completed = null, notice = ''; 
  function event(message) {
    if (completed) throw new Error('Unerwartete Daten nach Bildabschluss.');
    if (message.type === 'error') throw new Error(message.message);
    if (message.type === 'notice') {
      notice = message.message;
      $('status').textContent = notice;
      return;
    }
    if (message.type === 'row') {
      if (!Number.isInteger(message.y) || message.y < 0 || message.y >= 480) throw new Error('Ungültige Bildzeile.');
      const rgb = bytes64(message.rgb);
      if (rgb.length !== 640*3) throw new Error('Unvollständige Bildzeile.');
      for (let x = 0; x < 640; x++) {
        const index = colorIndices.get((rgb[x*3]<<16)|(rgb[x*3+1]<<8)|rgb[x*3+2]);
        if (index === undefined) throw new Error('Unbekannte Farbe in der Serverpalette.');
        visibleIndices[message.y*640+x] = index;
      }
      paintRows(message.y, 1);
      seen.add(message.y);
      $('status').textContent = `${notice ? notice + ' · ' : ''}${seen.size} / 480 Zeilen fertig · ${Math.floor(seen.size/480*100)} % · Server berechnet …`;
    } else if (message.type === 'done' && seen.size === 480) {
      completed = message;
    } else {
      throw new Error('Unvollständiger Bilddatenstrom.');
    }
  }
  try {
    while (true) {
      const {value, done} = await reader.read();
      signal.throwIfAborted();
      pending += done ? decoder.decode() : decoder.decode(value, {stream: true});
      let newline;
      while ((newline = pending.indexOf('\n')) !== -1) {
        event(JSON.parse(pending.slice(0, newline)));
        pending = pending.slice(newline+1);
      }
      if (pending.length > 2000000) throw new Error('Ungültiger Bilddatenstrom.');
      if (done) break;
    }
    if (pending.trim() || !completed) throw new Error('Bildübertragung abgebrochen.');
  } finally {
    await reader.cancel().catch(() => {});
    reader.releaseLock();
  }
  const url = URL.createObjectURL(new Blob([bytes64(completed.png)], {type: 'image/png'}));
  const probe = new Image(); probe.src = url;
  try { await probe.decode(); signal.throwIfAborted(); } catch (error) { URL.revokeObjectURL(url); throw error; }
  return {url, probe, indices: visibleIndices.slice(), backend: completed.backend, seconds: completed.seconds, notice};
}
async function render(next, preview = null, force = false, settings = null) {
  if (busy || configBusy) return false;
  const iterations = settings ? settings.iterations : Number($('iterations-manual').value);
  if (!validIterations(iterations)) {
    $('status').textContent = 'Bitte eine ganze Zahl von 50 bis 100000 Iterationen eingeben. Das bisherige Bild bleibt erhalten.';
    return false;
  }
  const controller = new AbortController();
  renderController = controller;
  busy = true; controls();
  if (preview && current) {
    const [x, y, size] = preview.map(v => v / 1000000);
    // Nearest-neighbour preview preserves exact palette indices for cycling.
    for (let py=0; py<480; py++) {
      const sy=Math.min(479, Math.floor(y*480+(py+0.5)*size));
      for (let px=0; px<640; px++) {
        const sx=Math.min(639, Math.floor(x*640+(px+0.5)*size));
        visibleIndices[py*640+px]=current.indices[sy*640+sx];
      }
    }
    paintRows();
  }
  $('status').textContent = preview
    ? 'Vergrößerte Vorschau – der Server berechnet die neuen Details …'
    : 'Server berechnet mit FPU, GPU oder GMP … (maximal 5 Minuten)';
  const key = JSON.stringify({path: next, iterations, backend: settings ? settings.backend : $('backend').value});
  try {
    let entry = force ? null : cache.get(key);
    const cached = Boolean(entry);
    if (!entry) {
      entry = await streamEntry(key, controller.signal);
    }
    show(entry);
    const old = cache.get(key);
    if (old && old !== entry) URL.revokeObjectURL(old.url);
    cache.delete(key); cache.set(key, entry);
    entry.view = JSON.parse(key);
    current = entry; path = next; ready = true;
    while (cache.size > 12) {
      const first = cache.keys().next().value;
      URL.revokeObjectURL(cache.get(first).url); cache.delete(first);
    }
    $('download').href = entry.url;
    const labels = {
      'cpu-long-double': 'CPU / long double · FPU',
      'opencl-fp32': 'Intel/OpenCL GPU · FP32',
      'opencl-fp64': 'OpenCL GPU · FP64',
      'opencl-perturb': 'GMP-Referenzbahn + GPU-Perturbation',
      'opencl-perturb-gmp': 'GMP-Referenzbahn + GPU · mit GMP-Pixelkorrekturen'
    };
    const backend = labels[entry.backend] || `CPU / GMP · ${128 + 20 * path.length} Bit angefordert`;
    $('status').textContent = `Zoom-Stufe ${path.length} · ${backend}${entry.notice ? ' · automatischer FPU → GMP-Rückfall' : ''} · ${cached ? 'aus Bildcache' : entry.seconds + ' s'} · Ausschnitt aufziehen zum Vergrößern`;
    return true;
  } catch (error) {
    if (current) show(current);
    else { visibleIndices.fill(BLACK); paintRows(); }
    $('status').textContent = controller.signal.aborted
      ? 'Berechnung abgebrochen. Parameter können jetzt angepasst werden.'
      : `Fehler: ${error.message} Bisherige Ansicht bleibt erhalten.`;
    return false;
  } finally { renderController = null; busy = false; controls(); }
}
async function configRequest(route, data) {
  const response = await fetch(route, {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(data)});
  if (!response.ok) throw new Error(await response.text());
  return response.json();
}
$('config-save').onclick = async () => {
  if (busy || configBusy || !current) return;
  const snapshot = {view: current.view, palette: {offset: Math.floor(paletteOffset), direction: Number($('palette-direction').value)}};
  configBusy = true; controls();
  try {
    const config = await configRequest('/config/export', snapshot);
    const url = URL.createObjectURL(new Blob([JSON.stringify(config, null, 2)+'\n'], {type: 'application/json'}));
    const link = document.createElement('a');
    link.href = url; link.download = 'apfelmaennchen-config.json'; link.click();
    setTimeout(() => URL.revokeObjectURL(url), 60000);
    $('status').textContent = 'Konfiguration als JSON-Datei heruntergeladen.';
  } catch (error) { $('status').textContent = `Konfiguration konnte nicht gespeichert werden: ${error.message}`; }
  finally { configBusy = false; controls(); }
};
$('config-load').onclick = () => { if (!busy && !configBusy) $('config-file').click(); };
$('config-file').onchange = async () => {
  const file = $('config-file').files[0];
  $('config-file').value = '';
  if (!file || busy || configBusy) return;
  configBusy = true; controls();
  try {
    if (file.size > 16384) throw new Error('Die Datei darf höchstens 16 KiB groß sein.');
    const data = JSON.parse(await file.text());
    const config = await configRequest('/config/import', data);
    configBusy = false;
    if (await render(config.view.path, null, false, config.view)) {
      $('backend').value = config.view.backend;
      setIterations(config.view.iterations);
      stopPalette();
      $('palette-direction').value = String(config.palette.direction);
      setPalette(config.palette.offset);
    }
  } catch (error) { $('status').textContent = `Konfiguration nicht geladen: ${error.message}`; }
  finally { configBusy = false; controls(); }
};
function point(event) {
  const r = canvas.getBoundingClientRect();
  return [Math.max(0, Math.min(1, (event.clientX-r.left)/r.width)), Math.max(0, Math.min(1, (event.clientY-r.top)/r.height))];
}
function clear() { anchor = selection = null; ctx.clearRect(0, 0, 640, 480); }
canvas.addEventListener('pointerdown', event => {
  if (busy || configBusy || !ready || event.button !== 0 || anchor) return;
  anchor = point(event); canvas.setPointerCapture(event.pointerId);
});
canvas.addEventListener('pointermove', event => {
  if (!anchor || !canvas.hasPointerCapture(event.pointerId)) return;
  const p = point(event), dx = p[0]-anchor[0], dy = p[1]-anchor[1];
  const size = Math.min(Math.max(Math.abs(dx), Math.abs(dy)), dx < 0 ? anchor[0] : 1-anchor[0], dy < 0 ? anchor[1] : 1-anchor[1]);
  selection = [dx < 0 ? anchor[0]-size : anchor[0], dy < 0 ? anchor[1]-size : anchor[1], size];
  ctx.clearRect(0, 0, 640, 480); ctx.fillStyle = '#ffffff22'; ctx.strokeStyle = '#ffffff'; ctx.lineWidth = 2;
  ctx.fillRect(selection[0]*640, selection[1]*480, size*640, size*480);
  ctx.strokeRect(selection[0]*640, selection[1]*480, size*640, size*480);
});
canvas.addEventListener('pointerup', event => {
  if (!canvas.hasPointerCapture(event.pointerId)) return;
  const rect = selection; clear();
  if (!rect || rect[2] < 0.01) return;
  if (path.length >= 80) { $('status').textContent = 'Maximale Zoom-Tiefe erreicht. Bitte zurückgehen.'; return; }
  const zoomRect = rect.map(v => Math.floor(v*1000000));
  render([...path, zoomRect], zoomRect);
});
canvas.addEventListener('pointercancel', clear);
canvas.addEventListener('lostpointercapture', clear);
$('back').onclick = () => render(path.slice(0, -1));
$('reset').onclick = () => render([]);
$('refresh').onclick = () => render([...path], null, true);
render([]);
