# 06 — Konfiguration speichern/laden

## Kontext / Dateibesitz
Lies KERN.md und vertraege/CONFIG.md. Zuständig: config_service.py, zwei HTTP-Routen, UI-Funktionen und Tests.

## Kleine Schritte
A. make_config mit gemeinsamer View-Validierung und Decimal-Koordinaten erstellen. Kanonisches Schema einhalten, keine float-Zwischenwerte.
B. Import auf Schema, Typen, Größe und rekonstruierte Grenzen prüfen. Python-Gleichheit reicht für Typprüfung nicht: True == 1. Config-Routen benötigen keinen Renderer-Slot.
C. Clientdownload als Blob aus letzter erfolgreicher Ansicht und aktueller Palette. Blob-URL danach freigeben. Unverwendete UI-Eingaben nicht exportieren. Ohne erfolgreiches Bild Export deaktivieren.
D. Datei auswählen, Größe vorab prüfen, serverseitig validieren. Danach Cache oder Renderer nutzen. Neue Ansicht/Palette erst bei Erfolg übernehmen, Animation pausieren. Bei Fehler/Abbruch alten Zustand erhalten.

## Abnahme
- Vertragsbeispiel und tiefer Pfad mit 80 Schritten roundtrippen ohne Präzisionsverlust.
- Manipulierte Grenzen, bool als int, falsche Bildgröße, zusätzliche Schlüssel, unbekannte Version und übergroße Datei abgelehnt.
- Import mit 50000 Iterationen synchronisiert Eingabefelder.
- Unangewandte Werte erscheinen nicht im Export.
- Gültiger Import mit anschließendem Renderfehler verändert alten Zustand nicht dauerhaft.

Keine Serverablage und keine Config-Volumes nötig. Die Downloads bleiben beim Anwender und überleben Containerneubauten. Im UI darauf hinweisen, dass Version 1 keine unabhängig editierbaren Eckpunkte unterstützt.
