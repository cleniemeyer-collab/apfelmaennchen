# 08 — Intel-CPU / long double

## Kontext / Dateibesitz
Lies KERN.md und RENDERER.md. Zuständig: render_long_double.c, Adapter im Dispatcher, Backendtests. HTTP/UI nur für noch fehlende Modus- oder Statusintegration ändern.

## Kleine Schritte
A. GMP-Werte direkt über ausreichend langen Dezimalstring nach strtold konvertieren. Nicht mpf_get_d verwenden: das würde die zusätzliche Präzision vorher verlieren. Konvertierungsfehler und Nichtendlichkeit behandeln.
B. LDBL_MANT_DIG und sizeof(long double) diagnostisch protokollieren. Auf x86-64 häufig 64 Mantissenbits bei 16 Byte Speicher, nicht automatisch 128-Bit-Präzision. Andere Architekturen nicht als gleich voraussetzen.
C. Vor Pixelrechnung Auflösung prüfen. l=left, t=top, s=step; scale=max(1,abs(l),abs(t),abs(l+640*s),abs(t-480*s)). Bei Nichtendlichkeit oder s <= 64*LDBL_EPSILON*scale Rückfall. Sofort fallback=long-double-precision auf stderr mit Flush; Dispatcher berechnet GMP.
D. Bei ausreichender Auflösung native Iterationsschleife mit long-double-Konstanten und derselben Reihenfolge wie GMP. Keine Quadratwurzel. OpenMP-Zeilenverteilung, gemeinsamer Callback. Erfolg meldet cpu-long-double, Rückfall gmp.

## Abnahme
- Flacher Ausschnitt nutzt cpu-long-double.
- Für die vorhandene Plattform konstruierter tiefer Ausschnitt triggert GMP; nicht feste Mantissenbreite voraussetzen.
- Streamhinweis erscheint vor GMP-Zeilen und bleibt in UI/Cache erhalten.
- Pixelzentrum-Konvertierung nicht durch double verkürzt.
- CPU-only-Build enthält dieses Backend.
- Vergleich stabiler Stichproben mit GMP; Unterschiede nahe der Grenze dokumentieren, keine universelle Pixelgleichheit behaupten.

Kein Fast-Math. Compilerflag -ffp-contract=off beibehalten. Die Auflösungsprüfung schützt Koordinatenabstände, nicht die gesamte chaotische Iteration. Auto-Modus bleibt unverändert und wählt nicht plötzlich FPU.
