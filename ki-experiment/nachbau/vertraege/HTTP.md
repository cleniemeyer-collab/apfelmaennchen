# Vertrag: HTTP und Streaming

## Routen
- GET /: index.html; GET /app.js und /style.css: feste statische Allowlist.
- GET /health: HTTP 200, Text ok; kein Rendererstart.
- POST /render/stream: NDJSON-Livestream.
- POST /render: PNG nach vollständiger Berechnung.
- POST /config/export und /config/import: siehe CONFIG.md.
- Unbekannte Routen: 404. Keine beliebigen Dateipfade ausliefern.

Render-JSON: `{"iterations":250,"path":[],"backend":"auto"}`.
Defaults bei fehlenden Feldern wie im Beispiel. Nur diese drei Schlüssel; Werte und Grenzen siehe KERN und RENDERER. JSON-Booleans sind keine gültigen Ganzzahlen. Kein NaN/Infinity. Maximal 16384 Byte Request-Body; ungültige Daten HTTP 400, belegter Render-Slot HTTP 503. Vor Prozessstart validieren. Keine Shell, nur Argumentliste.

## NDJSON
Content-Type application/x-ndjson; charset=utf-8. Ein JSON-Objekt pro Newline, nach jedem Ereignis flushen. Browser muss geteilte UTF-8-Zeichen und geteilte/zusammengefasste Zeilen korrekt puffern.

Ereignisse:
- notice: `{"type":"notice","reason":"long-double-precision","message":"FPU → GMP: Höhere Präzision nötig. Zeitlimit: 5 Minuten."}`
- row: Objekt mit type=row, y als Ganzzahl 0–479, rgb als Base64 von genau 1920 Byte.
- done: Objekt mit type=done, backend als tatsächlichem Backendnamen, seconds als Laufzeit in Sekunden, png als Base64 des fertigen PNG.
- error: Objekt mit type=error, message als lesbarer Fehlermeldung.

Die Beispiele mit beschreibenden Base64-Feldern sind Schemas, keine gültigen Bildfixtures. Für Tests echte Bytes kodieren. notice darf vor der ersten Zeile kommen. done genau einmal und erst bei vollständigem Bild sowie Exit 0. Streamende ohne done ist Fehler. Nach error kein done. Doppelte y-Werte ersetzen Zeilen, erhöhen aber nicht die Fortschrittszahl.

300-Sekunden-Gesamtdeadline ab Beginn des Renderauftrags. Bereits gestarteter Stream bleibt HTTP 200 und meldet Timeout per error; /render liefert HTTP 504. Allgemeine Renderfehler vor klassischer PNG-Antwort: HTTP 500. Keine Wiederholungsrechnung nach Timeout.

/render: image/png, X-Render-Backend und X-Render-Seconds. PNG muss 640×480, 8-bit RGB darstellen. PNG und Stream müssen dieselben finalen Pixel liefern.

## Lebenszyklus
Ein serverweiter Semaphore-Slot für genau einen Auftrag. Slot bei jedem Erfolg, Fehler, Timeout oder Abbruch freigeben. stdout/stderr gleichzeitig leeren; Diagnosepuffer begrenzen. Client-Disconnect während Livestream auch erkennen, wenn noch keine Zeile kommt. Renderer beenden, wait aufrufen, Pipes schließen.

Server-Umgebung RENDER_BACKEND=gmp erzwingt GMP für alle Anfragen; ansonsten entscheidet der Request, Standard auto. Prozessumgebung pro Anfrage kopieren, keine globalen Änderungen.

Cache-Control: no-store; X-Content-Type-Options: nosniff. CSP für eigene Skripte/Styles und Bild-Blobs. Stream nicht durch Proxy puffern lassen, Verbindung nach Antwort schließen. Lokaler Betrieb bevorzugt Bindung an Loopback; LAN-Freigabe bewusst konfigurieren.
