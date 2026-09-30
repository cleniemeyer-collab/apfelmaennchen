# Vertrag: Konfiguration Version 1

Export: POST /config/export mit Objekt aus view (Renderauftrag nach HTTP.md) und palette (offset, direction). Der Server ergänzt Defaults und rekonstruiert exakte Grenzen. Import: POST /config/import mit vollständiger Datei; Antwort ist die validierte vollständige Config. Import selbst startet noch keinen Renderer, das Frontend fordert anschließend das Bild an.

Gültige Startbilddatei:
```json
{
  "format": "apfelmaennchen",
  "version": 1,
  "view": {"iterations": 250, "backend": "auto", "path": []},
  "bounds": {"left": "-2.5", "right": "1.0", "top": "1.3125", "bottom": "-1.3125"},
  "image_size": {"width": 640, "height": 480},
  "palette": {"offset": 0, "direction": 1}
}
```

## Verbindliche Regeln
- Maximal 16384 UTF-8-Byte; Client prüft Dateigröße, Server prüft Body unabhängig.
- Version exakt ganzzahlig 1, format exakt apfelmaennchen.
- Nur die dokumentierten Schlüssel; vollständige importierte Config erforderlich.
- Iterationen, Zoompfad und Backend wie Renderauftrag.
- Palette: offset ganzzahlig 0–767, direction ganzzahlig -1 oder 1. Keine bool-Werte als int akzeptieren; auch Bildgröße und Version strikt prüfen.
- bounds sind Strings, nie JS Number; image_size fest 640×480.
- Python Decimal mit lokaler Präzision 1024, Startwerte aus Strings. Zoomrekonstruktion wie RENDERER.md; keine float-Zwischenwerte. Formatierung mit format(value, "f").
- Exportiert werden kanonische Grenzen. Import rekonstruiert die gesamte erwartete Config und verlangt kanonische Übereinstimmung, einschließlich Grenzstrings. Nicht nur numerisch ähnliche Grenzen akzeptieren.
- Version 1 definiert den Zoompfad als maßgeblich. Nur Eckpunkte editieren ist ungültig. Freie Koordinateneingabe wäre eine eigene zukünftige Formatversion.
- Dateiname als Download: apfelmaennchen-config.json. Keine Serverdateien nötig.

## UI-Transaktion
Export nimmt die letzte **erfolgreich berechnete** Ansicht, nicht unverwendete Eingabewerte. Palette entspricht der Anzeige. Import validieren, dann Bild aus Cache oder neu berechnen. Erst bei Erfolg neue Ansicht und Palette übernehmen; Animation pausieren. Bei ungültiger Datei, Renderfehler oder Abbruch alten vollständigen Zustand erhalten.

## Prüffälle
Startdatei und tiefer Pfad bleiben im Export/Import-Roundtrip identisch. Manipulierte bounds, unbekannte Version, falsche Schlüssel, float/bool statt int, falsche Bildgröße und übergroße Datei ablehnen. Veränderte Iterationen oder Backend sind innerhalb der erlaubten Werte zulässig. Speichern/Laden funktioniert nach Containerneubau, da die Datei beim Benutzer liegt.
