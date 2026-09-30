# Vertrag: Renderer und Mathematik

## Prozessschnittstelle
Aufruf: `./render ITERATIONEN [X Y SIZE ...]`. Pro Zoom genau drei ganze Zahlen. Höchstens 80 Tripel. Streng parsen: keine Restzeichen, negativen Zahlen oder Überläufe.

- Iterationen 50–100000.
- Bezugsmaß U = 1000000; 1000 <= SIZE <= U; 0 <= X,Y <= U-SIZE.
- `RENDER_BACKEND`: auto (Standard), gmp, long-double, perturb.
- `RENDER_STREAM=1`: Zeilenframes; sonst reines RGB-Vollbild in Rasterreihenfolge.
- `OMP_NUM_THREADS=4` als Betriebsstandard; keine gemeinsam beschreibbaren GMP-Variablen.
- Exit 0 nur bei Erfolg, 2 für ungültige CLI, 1 für Laufzeitfehler.
- stdout ausschließlich Binärdaten. stderr für Logs und genau benannte Statuszeilen.

Frame: vier Byte y als unsigned big-endian, danach 1920 RGB-Bytes. Insgesamt 1924 Byte, kein Zeilenumbruch. y=0 ist oben. Frames verschiedener Threads atomar unter einem gemeinsamen Ausgabelock schreiben und flushen. Pipe-Lesegrenzen sind keine Framegrenzen.

Zeilen dürfen außer Reihenfolge eintreffen. Ein GPU-Fehler nach Teilausgabe kann beim vollständigen GMP-Rückfall Zeilen erneut liefern: letzte Zeile gewinnt, Fortschritt zählt eindeutige y-Werte. Erfolg braucht alle 480 eindeutigen Zeilen, keinen Restframe und erfolgreichen Prozessabschluss.

Status auf stderr: `backend=NAME` vor erfolgreichem Ende. Bei FPU-Präzisionsrückfall vorher sofort `fallback=long-double-precision` mit Newline ausgeben und flushen. Zulässige NAME stehen in KERN.md. stdout niemals mit Logs vermischen.

## Koordinaten
Mit GMP-Präzision 128 + 20 * AnzahlZooms Bit initialisieren:
- left = -2.5; top = 1.3125; span = 3.5.
- Pro Tripel mit dem **alten** span: left += span*X/U; top -= span*(3/4)*Y/U; anschließend span *= SIZE/U.
- step = span/640; right = left+span; bottom = top-span*3/4.
- Pixelzentrum: cr = left+(px+0.5)*step; ci = top-(py+0.5)*step.
- Startbildgrenzen: left -2.5, right 1, top 1.3125, bottom -1.3125.

Testanker: Pfad [[250000,250000,500000]] ergibt left -1.625, right 0.125, top 0.65625, bottom -0.65625. Dessen Pixelabstand ist 1.75/640.

## Mandelbrot und Farbe
z beginnt bei 0. Für n von 0 bis limit-1: zuerst z = z*z+c, dann Abbruch falls |z|² > 4. Nicht >= und keine Quadratwurzel. Der Farbindex verwendet den **nullbasierten Schleifenindex n**, nicht n+1. Ohne Entkommen schwarz.

Bei Entkommen: c = (n*9) modulo 768; f = c modulo 256; k = ganzzahlig c/256.
- k=0: RGB (f, 0, 255-f)
- k=1: RGB (255-f, f, 0)
- k=2: RGB (0, 255-f, f)

Anker: n=0 -> (0,0,255), n=1 -> (9,0,246). Alle Backends müssen Pixelzentren, Zählung und Palette teilen.

## Interne Backendgrenze (Ziel)
Gemeinsame Header definieren Konstanten, RGB-Färbung und `row_ready(unsigned int y, const unsigned char *row)`; der Callback konsumiert die Daten synchron und behält den Pointer nicht.

GMP bietet `gmp_rows(left, top, step, limit, image, mask, first, rows, row_ready)` mit GMP-Quellwerten, Vollbild-RGB-Puffer und optionaler Vollbildmaske (Nichtnull = neu rechnen). Rückgabewert: Anzahl berechneter Pixel. Bei Maske bleiben andere Pixel unangetastet; Callback erst nach vollständiger Reparatur der Zeile.

Native Backends liefern Erfolg/Backendname oder kontrollierten Rückfall. Dispatcher besitzt den Vollbildpuffer und startet gegebenenfalls GMP. CPU-only muss ohne OpenCL-Header und -Bibliothek baubar bleiben.
