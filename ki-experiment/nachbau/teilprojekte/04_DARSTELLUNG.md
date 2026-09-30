# 04 — Darstellung, Eingaben und Zoom

## Kontext / Dateibesitz
Lies KERN.md, vertraege/HTTP.md und Zoomformeln in RENDERER.md. Zuständig: index.html, style.css, app.js und Frontendtests.

## Kleine Schritte
A. Canvas 640×480, Status, Backendauswahl, Iterations-Presets und manuelles Feld 50–100000. Neu berechnen, Zurück und Startbild. Labels und verständliche Fehler. Presets und manuelles Feld synchronisieren, auch bei Werten außerhalb der Presetliste.
B. Streamingclient mit TextDecoder im Streamingmodus und NDJSON-Puffer. HTTP-Fehler behandeln, Base64-Zeilenlänge prüfen, nach y zeichnen. Eindeutige Zeilen zählen. Fallbackhinweis separat vom Fortschritt halten. Erst done bedeutet Erfolg; EOF davor ist Fehler.
C. Pointer-Auswahl bei CSS-Skalierung in Canvas-Koordinaten umrechnen. Rechteck 4:3, auf Bild begrenzt. X=U*x/640, Y=U*y/480, SIZE=U*Breite/640. Nach Rundung gültige Integergrenzen sichern. Zu kleine Auswahl ablehnen, maximal 80 Schritte.
D. Erfolgreiche Ansicht getrennt von unverwendeten Eingaben speichern. Navigation transaktional behandeln. Vorschau darf vorhandene Pixel skalieren, aber keine Fraktale berechnen.

## Zustandsmodell
idle, rendering, success, error/aborted. Jeder Auftrag hat Kennung, alten vollständigen Snapshot und später AbortController. Veraltete Antworten dürfen neue Zustände nicht überschreiben. Während Rechnung Parameter sperren; Abbruch bleibt bedienbar. Nach Abbruch Eingaben wieder freigeben.

## Abnahme
- 2500 und 100000 werden tatsächlich als Iterationen gesendet; leere Werte, Bruchzahlen und Werte außerhalb der Grenzen abgelehnt.
- Geteilte UTF-8-/NDJSON-Daten funktionieren.
- Rückwärts aufgezogenes Rechteck und CSS-Skalierung ergeben gültige Tripel.
- Backendanzeige verwendet done.backend, nicht den angeforderten Modus.
- Netzwerkfehler erhalten letzte vollständige Ansicht.

Palette, Cache und PNG folgen in 07; Abbruch in 05.
