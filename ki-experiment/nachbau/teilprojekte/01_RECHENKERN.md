# 01 — Rechenkern und Dispatcher

## Kontext / Dateibesitz
Lies KERN.md und vertraege/RENDERER.md. Zuständig: render.c, render_common.h/.c und zugehörige Tests. Keine HTTP- oder GPU-Implementierung.

## Kleine Schritte
A. Konstanten, Palette und strikte CLI-Validierung erstellen. Alle Tripel vor der Rechnung prüfen. Restzeichen, Überlauf und negative Werte ablehnen.
B. GMP-Koordinaten rekonstruieren. Präzision vor Initialisierung setzen. Die beiden Koordinatenanker aus dem Vertrag isoliert testen.
C. Vollbildpuffer und Zeilencallback implementieren. Frames atomar schreiben und flushen, Schreibfehler behandeln. Im Nicht-Streammodus ausschließlich das vollständige RGB-Bild ausgeben.
D. Nach Auftrag 02 GMP integrieren. Danach native Adapter einzeln ergänzen. CPU-only muss angefordertes auto/perturb durch GMP-Rückfall bedienen können. Ein Teststub ist keine fertige Berechnung.

## Grenzen
Dispatcher besitzt Bildspeicher und Koordinaten. Callback konsumiert Zeilendaten synchron. OpenCL-Includes nur bedingt einbinden. stderr für Diagnose, stdout nur für Bilddaten. Nach fehlgeschlagenem GPU-Versuch darf GMP bereits ausgegebene Zeilen ersetzen; finalen Backendnamen korrekt melden.

## Abnahme
- 0 und 80 Zoomschritte gültig; 81, ungültige Tripel und Überläufe abgelehnt.
- y=479 korrekt als big-endian; jeder Frame genau 1924 Byte.
- Nach 02: dekodierter Stream und Vollbild pixelgleich, Vollbild genau 921600 Byte.
- CPU-Build ohne OpenCL und mit Compilerwarnungen als Fehler.

Übergabe: Header-Signaturen, Besitzregeln und offene Integrationspunkte in STATUS.md.
