# 03 — Hauptprogramm / HTTP

## Kontext / Dateibesitz
Lies KERN.md, vertraege/HTTP.md und Prozessschnittstelle in RENDERER.md. Zuständig: server.py, render_service.py, Servertests. Config-Routen folgen in 06.

## Kleine Schritte
A. ThreadingHTTPServer, statische Allowlist, /health und strikte JSON-Validierung. Maximal 16384 Byte, bool ist keine Iterationszahl. Gemeinsame Auftragsvalidierung für spätere Config-Funktion bereitstellen.
B. Renderer mit Argumentliste ohne Shell starten. Prozessumgebung pro Auftrag kopieren, GMP-Zwang beachten. Ein serverweiter nichtblockierender Semaphore; sonst 503. Immer im finally freigeben.
C. stdout/stderr parallel per selectors lesen. Teilframes puffern; y prüfen, Bild zusammensetzen und eindeutige Zeilen zählen. stderr begrenzen und Statuszeilen über Lesegrenzen hinweg erkennen. Pipes bei EOF abmelden und offenen Zähler reduzieren.
D. PNG mit Standardbibliothek: Signatur, IHDR für 640×480/8-bit RGB, Filterbyte 0 je Zeile, zlib-IDAT, IEND und CRCs. Klassische PNG-Route und NDJSON-Route möglichst mit gemeinsamer Prozesslogik.
E. Monotone Gesamtdeadline 300 Sekunden. notice früh weitergeben, done nur nach vollständigem Bild und Exit 0. Nach Streamstart Fehler als error, nicht durch Änderung des HTTP-Status. Disconnect-Behandlung in 05 vervollständigen.

## Abnahme mit Testrenderer
- Geteilte Frames, umgekehrte Reihenfolge und doppelte Zeilen funktionieren.
- Große stderr-Ausgabe blockiert stdout nicht.
- Fehlende Zeile, ungültiges y, Restframe oder Fehlerexit erzeugen kein done.
- Geteilte Fallback-Statuszeile wird erkannt.
- Timeout mit kurzem injiziertem Testwert prüfen; Produktion bleibt bei 300 Sekunden.
- Slot nach Fehler wieder frei; parallele Anfrage erhält 503.
- PNG dekodiert pixelgleich zum Stream.

Lokale Bindung bevorzugt 127.0.0.1. LAN-Freigabe bewusst konfigurieren; dies ist sicherer als die Standard-LAN-Freigabe des Originals.
