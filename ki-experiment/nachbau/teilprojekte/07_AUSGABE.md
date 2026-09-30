# 07 — Palette, Cache und PNG

## Kontext / Dateibesitz
Lies KERN.md, Palette in RENDERER.md und UI-Transaktion in CONFIG.md. Zuständig: Clientdarstellung und Tests, nicht die Berechnung.

## Kleine Schritte
A. Server-RGB verlustfrei auf ursprüngliche Palettenindizes abbilden. Die tatsächlich durch (n*9)%768 erreichbaren Farben sind eindeutig. Schwarz hat einen eigenen Sentinel und rotiert nie. Keine interpolierten Mischfarben als exakte Indizes behandeln.
B. Versatz 0–767: Palette[(Originalindex+offset)%768]. Animation nur auf Nutzeraktion mit 96 Einträgen/Sekunde; direction ±1 steuert den Versatz. Manuelles Schieben pausiert. Originalfarben stoppt und setzt Versatz 0. requestAnimationFrame verwenden, keine mehrfachen Animationsschleifen.
C. Vorschau über Nearest-Neighbour auf Indexdaten. Live-Zeilen und Cache mit derselben Palette anzeigen. Palette löst keine Serveranfrage aus.
D. Maximal zwölf vollständige Ansichten mit begrenzter Verdrängungsstrategie (z. B. LRU). Schlüssel aus kanonischem Pfad, Iterationen und angefordertem Backend. Tatsächlichen Backendnamen und Fallbackhinweis als Metadaten speichern. Zurück/Startbild nutzen Cache; Neu berechnen umgeht ihn. Teilbilder ausschließen.
E. PNG-Download zeigt aktuelle Palette. Bei Versatz 0 Original-PNG möglich, sonst aktuelles Canvas als PNG exportieren. Animationszustand für den Download konsistent aufnehmen, Objekt-URLs aufräumen.

## Abnahme
- Palette bei Versatz 0 exakt wie Serverbild; Schwarz bei jedem Versatz unverändert.
- Rotation benötigt keine Fraktalrechnung und keine Netzwerkanfrage.
- Cache trennt 250/2500 Iterationen und gmp/auto; älteste Ansicht wird bei Kapazitätsüberschreitung verdrängt.
- FPU→GMP-Hinweis bleibt bei Cachetreffer erhalten.
- Abgebrochene Ansichten sind weder Cachetreffer noch Downloadgrundlage.
- Import pausiert Animation, Export enthält sichtbare Palette und erfolgreich berechnete View.
