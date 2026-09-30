# Übergabestatus

## Stand
Nur der Bauplan wurde erstellt. Keine Nachbau-Implementierung, keine ausgeführten Anwendungs-Tests, kein neu gebauter Container. Das Original bleibt unverändert.

| Auftrag | Zustand |
|---|---|
| 01 Rechenkern | Schritt A abgeschlossen |
| 02 GMP | offen |
| 03 Hauptprogramm | offen |
| 04 Darstellung | offen |
| 05 Abbruch | offen |
| 06 Konfiguration | offen |
| 07 Ausgabe | offen |
| 08 long double | offen |
| 09 OpenCL | offen |
| 10 Referenzbahn | offen |
| 11 Betrieb | offen |
| 12 Integration | offen |

## Nächste Sitzung
Auftrag 01, Schritt B – GMP-Koordinaten rekonstruieren. Erforderlicher Kontext: KERN.md, 01_RECHENKERN.md, RENDERER.md.

## Je Sitzung ersetzen oder knapp ergänzen
- Aktueller Schritt: Auftrag 01, Schritt A – abgeschlossen
- Umgesetzt / relevante Dateien und Symbole:
  - teilprojekte/01_RECHENKERN/src/render_common.h: Konstanten (IMG_*, FRAME_*, COORD_*), Palette, CLI-Typen, öffentliche Deklarationen für coord_init/coord_zoom/coord_final/palette_rgb/frame_write_header/mandelbrot_iter/parse_cli
  - teilprojekte/01_RECHENKERN/src/render_common.c: palette_rgb (Vertragspalette), frame_write_header (big-endian y), parse_cli (strikt: Restzeichen, Überlauf, X,Y≤U-SIZE, Tripel-Vollständigkeit), coord_init/coord_zoom/coord_final
  - teilprojekte/01_RECHENKERN/src/render.c: Bereinigt – private Kopien entfernt, <unistd.h> hinzugefügt, Inline-Mandelbrot-Iteration für CLI-Validierungstestbarkeit
  - teilprojekte/01_RECHENKERN/tests/test_step_a.c: 36 Tests (CLI, Frame-Header, Palette, Koordinaten, Mandelbrot)
- Tests mit Befehlen und Ergebnissen:
  - `gcc -Wall -Wextra -Werror -std=c11 -o tests/test_step_a tests/test_step_a.c src/render_common.c && ./tests/test_step_a` → 36/36 PASS
  - `gcc -Wall -Wextra -Werror -std=c11 -c src/render.c` → warnfrei
- Nicht ausgeführt / Grund: Schritt B noch nicht begonnen
- Blocker und bekannte Abweichungen: Keine
- Getroffene Schnittstellenentscheidungen:
  - parse_cli gibt Exit-Codes zurück (0=OK, 2=CLI-Fehler)
  - frame_write_header schreibt big-endian unsigned int y
  - Palette folgt Vertrag: c=(n*9)%768, k=c/256, f=c%256 mit drei Farbketten
- Nächster Schritt / dafür nötige Dateien:
  - Auftrag 01, Schritt B – GMP-Koordinaten rekonstruieren
  - Benötigte Dateien: KERN.md, teilprojekte/01_RECHENKERN.md, vertraege/RENDERER.md

Nur den aktuellen Stand und wichtige Entscheidungen behalten, kein unendliches Chatprotokoll. Größere Entscheidungen bei Bedarf in eine eigene kurze Markdown-Datei auslagern.
