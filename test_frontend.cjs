// Browser API mocks: test interaction logic without installing a browser.
const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');
const draws = [], listeners = {}, requests = [], revoked = [], rows = [];
const elements = {};
for (const id of ['image', 'overlay', 'reset', 'back', 'refresh', 'cancel', 'iterations', 'iterations-manual', 'backend', 'status', 'download', 'palette', 'palette-value', 'palette-play', 'palette-reset', 'palette-direction', 'config-save', 'config-load', 'config-file']) {
  elements[id] = {files: [], options: [], add(option) { this.options.push(option); }, click() {}, style: {}, setAttribute(name, value) { this[name] = value; }, value: id === 'backend' ? 'long-double' : id === 'palette-direction' ? '1' : '250', classList: {toggle() {}},
    getContext: () => ({drawImage: (...args) => draws.push(args), clearRect() {}, fillRect() {}, strokeRect() {},
          createImageData: (w,h) => ({data: new Uint8ClampedArray(w*h*4)}), putImageData: (row, _x, y) => { rows.push(y); draws.push([Array.from(row.data.slice(0,16)),row.data.length]); }}),
    addEventListener: (event, callback) => { listeners[event] = callback; },
    setPointerCapture() {}, hasPointerCapture: () => true,
    getBoundingClientRect: () => ({left: 0, top: 0, width: 640, height: 480})};
}
let pending = null, fail = false, number = 0, rowPause = null, truncated = false, streamError = false, fallbackNotice = false;
const rgb = Buffer.alloc(1920).toString('base64');
const line = y => JSON.stringify({type: 'row', y, rgb}) + '\n';
const remainingRows = Array.from({length: 479}, (_, i) => line(i+1)).join('');
function abortable(promise, signal) {
  signal.throwIfAborted();
  return new Promise((resolve, reject) => {
    const abort = () => reject(signal.reason);
    signal.addEventListener('abort', abort, {once: true});
    Promise.resolve(promise).then(value => {
      signal.removeEventListener('abort', abort); resolve(value);
    }, error => { signal.removeEventListener('abort', abort); reject(error); });
  });
}
function response(requested, signal) {
  const backends = {'gmp': 'gmp', 'perturb': 'opencl-perturb-gmp', 'long-double': 'cpu-long-double'};
  const finish = streamError ? {type: 'error', message: 'Streamfehler'} : {type: 'done', backend: fallbackNotice ? 'gmp' : backends[requested] || 'opencl-fp32', seconds: 0.1, png: 'AA=='};
  const text = line(0);
  // Split a JSON frame across network reads, then batch many frames together.
  const notice = fallbackNotice ? JSON.stringify({type: 'notice', reason: 'long-double-precision', message: 'FPU → GMP: höhere Präzision nötig. Zeitlimit: 5 Minuten.'})+'\n' : '';
  const chunks = [notice+text.slice(0, 17), text.slice(17)];
  if (!truncated) chunks.push(remainingRows + JSON.stringify(finish) + '\n');
  let position = 0;
  return {ok: !fail, text: async () => 'Testfehler', body: {getReader: () => ({
    async read() {
      signal.throwIfAborted();
      if (position === 2 && rowPause) await abortable(rowPause, signal);
      return position < chunks.length ? {value: new TextEncoder().encode(chunks[position++]), done: false} : {done: true};
    }, async cancel() {}, releaseLock() {}
  })}};
}
const frames = new Map(), timers = [], downloads = [], configRequests = [], blobs = new Map();
let configFailure = false;
let frameNumber = 0;
const exportContext = {filter: 'none', drawImage(source) { downloads.push({source, filter: this.filter}); }};
const context = vm.createContext({TextDecoder, Blob, atob, AbortController,
  requestAnimationFrame: fn => { frames.set(++frameNumber, fn); return frameNumber; },
  cancelAnimationFrame: id => frames.delete(id), setTimeout: fn => timers.push(fn),
  document: {getElementById: id => elements[id], createElement: tag => tag === 'canvas'
    ? {getContext: () => exportContext, toBlob: callback => callback(new Blob(['png']))}
    : {click() { downloads.push({href: this.href, filename: this.download}); }}}, 
  Image: class {async decode() {}},
  URL: {createObjectURL: blob => { const url = 'blob:' + ++number; blobs.set(url, blob); return url; }, revokeObjectURL: url => revoked.push(url)},
  fetch: async (_url, options) => {
    const body = JSON.parse(options.body);
    if (_url.startsWith('/config/')) {
      configRequests.push({_url, body});
      return {ok: !configFailure, text: async () => 'Ungültige Konfiguration', json: async () =>
        _url === '/config/export' ? {format: 'apfelmaennchen', version: 1, ...body,
          bounds: {left: '-2.5', right: '1', top: '1.3125', bottom: '-1.3125'}} : body};
    }
    assert.equal(_url, '/render/stream'); requests.push(body);
    if (pending) await abortable(pending, options.signal); return response(body.backend, options.signal);
  }});
const tick = () => new Promise(resolve => setImmediate(resolve));
function zoom() {
  listeners.pointerdown({button: 0, pointerId: 1, clientX: 128, clientY: 96});
  listeners.pointermove({pointerId: 1, clientX: 288, clientY: 216});
  listeners.pointerup({pointerId: 1});
  listeners.lostpointercapture({pointerId: 1});
}
(async () => {
  vm.runInContext(fs.readFileSync('app.js', 'utf8'), context);
  await tick(); assert.equal(requests.length, 1);
  assert.equal(requests[0].backend, 'long-double');
  const html = fs.readFileSync('index.html', 'utf8');
  assert.match(html, /<option value="long-double" selected>/);
  const backendOptions = html.match(/<select id="backend">(.*?)<\/select>/)[1];
  assert.deepEqual(Array.from(backendOptions.matchAll(/value="([^"]+)"/g), match => match[1]), ['long-double', 'auto', 'gmp', 'perturb']);
  let release; pending = new Promise(resolve => { release = resolve; });
  zoom();
  assert.equal(draws.at(-1)[1], 640*480*4, 'Indexed preview painted before HTTP completes');
  assert.match(elements.status.textContent, /Vorschau/);
  assert.equal(elements.download.hidden, true);
  release(); pending = null; await tick();
  assert.equal(draws.at(-1)[1], 640*480*4, 'Completed image replaces preview');
  assert.equal(requests.length, 2);
  await elements.back.onclick();
  assert.equal(requests.length, 2, 'Back uses cache');
  assert.match(elements.status.textContent, /Bildcache/);
  zoom(); await tick(); assert.equal(requests.length, 2, 'Repeated zoom uses cache');
  const previous = draws.at(-1)[0];
  fail = true; zoom(); await tick();
  assert.deepEqual(draws.at(-1)[0], previous, 'Error restores previous image');
  assert.match(elements.status.textContent, /Testfehler/);
  fail = false;
  await elements.refresh.onclick(); assert.equal(requests.length, 4, 'Refresh bypasses cache');
  elements.backend.value = 'gmp'; await elements.reset.onclick();
  assert.equal(requests.at(-1).backend, 'gmp');
  for (let i=0;i<14;i++) { elements.iterations.value = String(100+i); elements.iterations.onchange(); await elements.reset.onclick(); }
  assert.ok(revoked.length > 0, 'Cache eviction releases URLs');
  assert.equal(vm.runInContext('cache.size', context), 12);
  let resume; rowPause = new Promise(resolve => { resume = resolve; });
  const progressive = elements.refresh.onclick();
  await tick();
  assert.equal(rows.at(-1), 0, 'First row displayed before stream completion');
  assert.match(elements.status.textContent, /1 \/ 480 Zeilen/);
  assert.equal(elements.download.hidden, true);
  resume(); rowPause = null; await progressive;
  assert.ok(rows.includes(479));
  const completeImage = draws.at(-1)[0];
  truncated = true; await elements.refresh.onclick(); truncated = false;
  assert.match(elements.status.textContent, /abgebrochen/);
  assert.deepEqual(draws.at(-1)[0], completeImage, 'Truncated stream restores last image');
  streamError = true; await elements.refresh.onclick(); streamError = false;
  assert.match(elements.status.textContent, /Streamfehler/);
  assert.deepEqual(draws.at(-1)[0], completeImage, 'Stream error restores last image');
  const requestCount = requests.length;
  assert.equal(vm.runInContext(`Array.from({length:256},(_,n)=>n*3).every(c =>
    colorIndices.get((palette[c*3]<<16)|(palette[c*3+1]<<8)|palette[c*3+2]) === c)`, context), true,
    'All reachable server palette indices are recovered exactly');
  vm.runInContext('visibleIndices.set([0,256,512,BLACK]); setPalette(256)', context);
  assert.deepEqual(draws.at(-1)[0], [255,0,0,255, 0,255,0,255, 0,0,255,255, 0,0,0,255],
    'One-third cycle maps blue to red, red to green, green to blue; black stays black');
  vm.runInContext('setPalette(768)', context);
  assert.deepEqual(draws.at(-1)[0], [0,0,255,255, 255,0,0,255, 0,255,0,255, 0,0,0,255],
    'Full cycle restores original colors exactly');
  vm.runInContext('setPalette(-256)', context);
  assert.deepEqual(draws.at(-1)[0], [0,255,0,255, 0,0,255,255, 255,0,0,255, 0,0,0,255],
    'Reverse cycle reverses the wave direction');
  elements.palette.value = '120'; elements.palette.oninput();
  assert.equal(vm.runInContext('paletteOffset', context), 120);
  assert.equal(elements['palette-value'].textContent, '120 / 768');
  elements['palette-play'].onclick();
  assert.equal(elements['palette-play']['aria-pressed'], 'true');
  function advance(time) { const [id, callback] = frames.entries().next().value; frames.delete(id); callback(time); }
  advance(0); advance(100);
  assert.equal(vm.runInContext('Math.floor(paletteOffset)', context), 129);
  elements['palette-play'].onclick();
  assert.equal(frames.size, 0, 'Pause stops animation');
  assert.equal(requests.length, requestCount, 'Palette rotation never requests a render');
  let prevented = false;
  elements.download.onclick({preventDefault() { prevented = true; }});
  assert.equal(prevented, true);
  assert.equal(downloads[0].filter, 'none');
  assert.equal(downloads[0].source, elements.image);
  assert.equal(downloads[1].filename, 'apfelmaennchen-palette-129.png');
  timers.forEach(fn => fn());
  assert.ok(revoked.includes(downloads[1].href), 'Export URL released');
  elements['palette-direction'].value = '-1';
  elements['palette-play'].onclick(); advance(0); advance(100);
  assert.equal(vm.runInContext('Math.round(paletteOffset)', context), 120);
  elements['palette-reset'].onclick();
  assert.equal(vm.runInContext('paletteOffset', context), 0);
  assert.equal(elements['palette-play']['aria-pressed'], 'false');
  const actualView = JSON.parse(vm.runInContext('JSON.stringify(current.view)', context));
  elements.iterations.value = '2000'; elements.iterations.onchange(); elements.backend.value = 'perturb';
  elements.palette.value = '256'; elements.palette.oninput();
  await elements['config-save'].onclick();
  assert.deepEqual(configRequests.at(-1).body.view, actualView, 'Export uses rendered settings, not unapplied controls');
  const saved = JSON.parse(await blobs.get(downloads.at(-1).href).text());
  assert.equal(downloads.at(-1).filename, 'apfelmaennchen-config.json');
  assert.equal(saved.palette.offset, 256);
  const imported = {...saved, view: {path: [[100000,200000,500000]], iterations: 777, backend: 'perturb'}, palette: {offset: 333, direction: -1}};
  async function load(data, size) {
    const text = typeof data === 'string' ? data : JSON.stringify(data);
    elements['config-file'].files = [{size: size ?? Buffer.byteLength(text), text: async () => text}];
    await elements['config-file'].onchange();
  }
  await load(imported);
  assert.deepEqual(requests.at(-1), imported.view);
  assert.equal(elements.iterations.value, '777');
  assert.ok(elements.iterations.options.some(option => option.value === '777'));
  assert.equal(elements['iterations-manual'].value, '777', 'Config import synchronizes manual input');
  assert.equal(elements.backend.value, 'perturb');
  assert.equal(vm.runInContext('paletteOffset', context), 333);
  assert.equal(elements['palette-direction'].value, '-1');
  assert.equal(vm.runInContext('rotating', context), false);
  const goodView = vm.runInContext('JSON.stringify(current.view)', context);
  await load('{broken JSON');
  assert.match(elements.status.textContent, /Konfiguration nicht geladen/);
  await load(imported, 16385);
  assert.match(elements.status.textContent, /16 KiB/);
  configFailure = true; await load(imported); configFailure = false;
  assert.match(elements.status.textContent, /Ungültige Konfiguration/);
  fail = true; await load({...imported, view: {...imported.view, iterations: 778}}); fail = false;
  assert.equal(vm.runInContext('JSON.stringify(current.view)', context), goodView, 'Failed import render preserves previous view');
  assert.equal(elements.iterations.value, '777');
  assert.equal(vm.runInContext('paletteOffset', context), 333);
  assert.equal(elements['config-file'].value, '');
  await load({...imported, view: {...imported.view, backend: 'long-double'}});
  assert.equal(requests.at(-1).backend, 'long-double');
  assert.equal(elements.backend.value, 'long-double');
  assert.match(elements.status.textContent, /CPU \/ long double/);
  await elements['config-save'].onclick();
  assert.equal(configRequests.at(-1).body.view.backend, 'long-double');
  fallbackNotice = true;
  let finishFallback; rowPause = new Promise(resolve => { finishFallback = resolve; });
  const fallbackRender = elements.refresh.onclick();
  await tick();
  assert.match(elements.status.textContent, /FPU → GMP/);
  assert.match(elements.status.textContent, /1 \/ 480 Zeilen/);
  assert.match(elements.status.textContent, /5 Minuten/);
  finishFallback(); rowPause = null; await fallbackRender;
  assert.match(elements.status.textContent, /automatischer FPU → GMP-Rückfall/);
  const countBeforeCache = requests.length;
  await vm.runInContext('render([...path])', context);
  assert.equal(requests.length, countBeforeCache);
  assert.match(elements.status.textContent, /Bildcache/);
  assert.match(elements.status.textContent, /FPU → GMP/);
  elements['iterations-manual'].value = '1234'; elements['iterations-manual'].oninput();
  assert.equal(elements.iterations.value, '1234');
  let entered = false;
  await elements['iterations-manual'].onkeydown({key: 'Enter', preventDefault() { entered = true; }});
  assert.equal(entered, true);
  assert.equal(requests.at(-1).iterations, 1234);
  assert.equal(elements['iterations-manual'].disabled, false);
  const validImage = vm.runInContext('JSON.stringify(current.view)', context);
  const beforeInvalid = requests.length;
  for (const invalid of ['', '49', '100001', '55.5', 'NaN']) {
    elements['iterations-manual'].value = invalid; elements['iterations-manual'].oninput();
    await elements.refresh.onclick();
    assert.match(elements.status.textContent, /ganze Zahl von 50 bis 100000/);
    assert.equal(requests.length, beforeInvalid);
    assert.equal(vm.runInContext('JSON.stringify(current.view)', context), validImage);
  }
  for (const boundary of ['50', '2001', '10000', '100000']) {
    elements['iterations-manual'].value = boundary; elements['iterations-manual'].oninput();
    await elements.refresh.onclick();
    assert.equal(requests.at(-1).iterations, Number(boundary));
  }
  elements.iterations.value = '500'; elements.iterations.onchange();
  assert.equal(elements['iterations-manual'].value, '500');
  const beforeCancel = vm.runInContext('JSON.stringify(current.view)', context);
  const beforeCancelCache = vm.runInContext('cache.size', context);
  pending = new Promise(() => {});
  const beforeHeaders = elements.refresh.onclick();
  assert.equal(elements.cancel.disabled, false);
  assert.equal(elements['iterations-manual'].disabled, true);
  elements.cancel.onclick();
  assert.equal(await beforeHeaders, false);
  pending = null;
  assert.match(elements.status.textContent, /Berechnung abgebrochen/);
  assert.equal(elements.cancel.disabled, true);
  assert.equal(elements['iterations-manual'].disabled, false);
  assert.equal(elements.backend.disabled, false);
  assert.equal(vm.runInContext('JSON.stringify(current.view)', context), beforeCancel);
  rowPause = new Promise(() => {});
  const betweenRows = elements.refresh.onclick();
  await tick();
  assert.match(elements.status.textContent, /1 \/ 480 Zeilen/);
  elements.cancel.onclick();
  assert.equal(await betweenRows, false);
  rowPause = null;
  assert.equal(vm.runInContext('JSON.stringify(current.view)', context), beforeCancel);
  assert.equal(vm.runInContext('cache.size', context), beforeCancelCache);
  elements['iterations-manual'].value = '12345'; elements['iterations-manual'].oninput();
  assert.equal(await elements.refresh.onclick(), true, 'New render succeeds after cancellation');
  assert.equal(requests.at(-1).iterations, 12345);
  assert.equal(elements.cancel.disabled, true);
  console.log('Frontend: cancellation before headers/between rows, parameter editing, rerender, cache and config passed.');
})().catch(error => { console.error(error); process.exitCode = 1; });
