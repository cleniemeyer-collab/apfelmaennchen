# Gemeinsamer Kernkontext

## Produkt
Mandelbrot-Webviewer mit serverseitiger Rechnung. Python-Standardbibliothek als HTTP-Server, C mit GMP und OpenMP als Renderer, Vanilla-JavaScript/HTML/CSS als Client. Docker optional für lokalen Betrieb; GPU optional. Keine KI im Laufzeitpfad.

## Feste Vorgaben
- Bild: 640 × 480 RGB, Zoomrechtecke im Seitenverhältnis 4:3.
- Iterationen: ganze Zahlen 50–100000, Standard 250; manuelle Eingabe auch über 2000.
- Höchstens 80 Zoomschritte; Koordinaten als ganzzahliger Zoompfad, nicht als JS-double.
- Genauigkeit: 128 + 20 Bit pro Zoomschritt bei GMP.
- Rechenwege angefordert: auto, gmp, long-double, perturb.
- Tatsächliche Backends: gmp, cpu-long-double, opencl-fp32, opencl-fp64, opencl-perturb, opencl-perturb-gmp.
- Auto: direkte GPU, wenn verfügbar und ausreichend auflösend, sonst GMP. Kein automatischer Wechsel zu Perturbation oder FPU.
- Erzwungenes GMP auf Serverebene hat Vorrang. FPU-Präzisionsrückfall früh und dauerhaft anzeigen.
- Ein aktiver Bildauftrag pro Server, weitere HTTP 503. Zeitlimit 300 Sekunden, auch bei GMP; kein Neustart des Limits beim Rückfall.
- Fortschritt zeilenweise, Reihenfolge darf variieren. Abbruch beendet auch den Renderer und stellt den letzten vollständigen Zustand wieder her.
- Config-Export/-Import als clientseitige JSON-Datei, max. 16 KiB. Keine Ablage im Container.
- PNG-Download, Farbrotation, maximal zwölf vollständige Ansichten im Cache.

## Zielaufteilung im neuen Projekt
- `render.c`: CLI, Koordinaten, Backendauswahl und Ausgabe.
- `render_common.h`, `render_common.c`: gemeinsame Konstanten, Palette und Schnittstellen.
- `render_gmp.c`, `render_gmp.h`: GMP-Backend einschließlich Pixelreparatur.
- `render_long_double.c`: FPU-Backend.
- `gpu.c`, `gpu.h`, `mandelbrot.cl`: direkte GPU.
- `perturb.c`, `perturb.cl`: Referenzbahn und GPU-Abweichungen.
- `server.py`: HTTP und Ressourcenbegrenzung.
- `render_service.py`: Prozessüberwachung, Frames, PNG.
- `config_service.py`: Validierung und exakte Config-Koordinaten.
- `index.html`, `style.css`, `app.js`: Client. Erst bei Bedarf weiter aufteilen.
- `tests/`, `Makefile`, Dockerfiles und Compose-Dateien.

Diese Dateien existieren zu Beginn noch nicht. Öffentliche Wire-Formate werden durch die Dateien unter vertraege/ festgelegt. Interne Funktionen dürfen passend zum Code organisiert werden; Änderungen müssen im Status stehen.

## Numerische Grenzen
GMP ist die Referenz, aber ebenfalls endlich präzise. Schwarz bedeutet nicht entkommen innerhalb des Limits, nicht bewiesene Mengenzugehörigkeit. FPU-/GPU-Auflösungsprüfungen garantieren keine identischen Iterationszahlen nahe der Grenze. Perturbation ist experimentell und benötigt GMP-Korrekturen.
