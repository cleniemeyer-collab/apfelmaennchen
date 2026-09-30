# 02 — GMP-Backend

## Kontext / Dateibesitz
Lies KERN.md, vertraege/RENDERER.md und die Header aus 01. Zuständig: render_gmp.c/.h und Backendtests.

## Kleine Schritte
A. Einzelpixel mit mpf berechnen. Initial x=y=xx=yy=0. Pro Iteration zuerst t=x*y und y=2*t+ci, dann x=xx-yy+cr, danach xx=x*x und yy=y*y. Abbruch bei xx+yy > 4. Alte Quadrate weiterverwenden: drei mpf-Multiplikationen statt fünf. Nullbasierten Schleifenindex einfärben.
B. Zeilen und Vollbild ergänzen. Pixelzentren mit step*(2*p+1)/2 bilden, keine double-Zwischenwerte.
C. OpenMP mit dynamischer Zeilenverteilung. Jeder Thread besitzt eigene temporäre GMP-Werte. Koordinaten nur lesend teilen. Callback nach vollständiger Zeile.
D. Maskierte Nachberechnung: Vollbildmaske, Zeilenbereich first/rows, nur markierte Pixel ersetzen. Andere Pixel erhalten; Callback liefert die gesamte reparierte Zeile. Rückgabe zählt berechnete Pixel. Schnittstelle für Auftrag 10.

## Abnahme
- c=0 entkommt nicht; c=3 entkommt mit n=0; c=2 entkommt erst mit n=1, weil Gleichheit mit 4 nicht genügt.
- Optimierte Schleife gegen unabhängige langsame GMP-Referenz auf Stichproben prüfen.
- Ein und vier Threads ergeben dieselben Bildbytes.
- Leere Maske verändert nichts; einzelne Markierung verändert nur diesen Pixel.
- Alle GMP-Werte und Speicher werden freigegeben.

Keine zusätzlichen Innenraumtests oder Fast-Math-Optimierungen ohne gesonderte Tests. GMP ist die endlich präzise Referenz, kein Beweis unendlicher Genauigkeit.
