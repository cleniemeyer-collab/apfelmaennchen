# 10 — GMP-Referenzbahn und GPU-Perturbation

## Kontext / Dateibesitz
Lies KERN.md, RENDERER.md und Schnittstellen aus 02/09. Zuständig: perturb.c, perturb.cl, Adapter und eigene Tests. Experimentell, nicht Standardauto.

## Schritt A: Referenzbahn separat testen
Bildmitte C=(left+320*step, top-240*step) mit GMP rekonstruieren. Z0=0, Z(n+1)=Zn²+C. Höchstens limit+1 Bahnwerte speichern. Nach |Z|²>256 abbrechen, damit die Referenz nicht unkontrolliert wächst. GPU-Darstellung FP32 mit real, imag und Fehlerabschätzung pro Eintrag.

FP32-Konstanten EPS=2^-23 und TINY=2^-126. Fehlerwert für Referenzeintrag: 8*EPS*(abs(real)+abs(imag))+32*TINY. Z0 und dessen Fehler mit Null initialisieren. Das ist eine praktische Abschätzung, kein formaler Intervallbeweis.

## Schritt B: GPU-Abweichung
Pixelabweichung dc=((px-319.5)*step, (239.5-py)*step). Keine Addition winziger Abstände zu großen absoluten FP32-Koordinaten. delta0=0; delta_next=2*Zn*delta+delta²+dc; tatsächliches z=Z(n+1)+delta_next. Nullbasierter Farbindex wie im Vertrag.

Ohne Skalierung unter step<1e-30 oder bei nichtendlichem FP32-step vollständiger GMP-Rückfall. Keine Mehrfachreferenzen oder Exponentenskalierung in dieser Stufe.

## Schritt C: Fehlerheuristik und Reparatur
Für Nachbau der bisherigen Heuristik L1(v)=abs(real)+abs(imag), d=L1(delta), z=L1(Zn), e=aktueller Fehler, r=Referenzfehler von Zn, D=L1(dc):
- dc_error=8*EPS*D+32*TINY.
- e_next=[2*(z+d)*e+e²+2*r*(d+e)+dc_error+32*EPS*(2*z*d+d²+D)+32*TINY]*1.00001.
- Nach delta-Update: total=e_next+Fehler(Zn+1)+4*EPS*(L1(Zn+1)+L1(delta_next))+32*TINY.
- norm=abs(actual)²; uncertainty=2*L1(actual)*total+2*total²+8*EPS*(norm+1).

Pixel zur GMP-Reparatur markieren, wenn:
- benötigter nächster Referenzwert fehlt;
- norm oder uncertainty nicht endlich ist;
- total>0.001;
- norm < 1e-6*abs(Zn+1)² (Auslöschung);
- Escape-Test unsicher: norm-uncertainty <= 4 und norm+uncertainty >= 4.

Wenn norm-uncertainty>4, Pixel entkommen. Wenn norm+uncertainty<4, weiteriterieren. Nach limit ohne Entkommen schwarz. Reparaturpixel müssen nicht vorher korrekt eingefärbt sein, dürfen aber vor Reparatur nicht ausgegeben werden.

## Schritt D: Blockintegration
Acht Zeilen GPU rechnen, RGB und Maske zurücklesen. Markierte Pixel über gmp_rows mit ursprünglichen GMP-Koordinaten reparieren. Erst danach vollständige Zeilen callbacken. Erfolg ohne Reparaturen: opencl-perturb; mit Reparaturen: opencl-perturb-gmp. Vollständiger Rückfall: gmp. Referenzlänge und Reparaturanzahl loggen, Speicher stets freigeben.

## Abnahme
- Referenzindizes und delta-Rekurrenz auf kurzen Bahnen gegen GMP prüfen.
- Zu kurze Bahn, Unterlaufbereich, nichtendliche Werte und mehrdeutiger Escape-Test führen zur Reparatur/zum Rückfall.
- Maske überschreibt keine unmarkierten Pixel; keine unreparierten Zeilen im Stream.
- Startbild, Randregion und tiefer Zoom mit GMP vergleichen; Reparaturquote/Laufzeit erfassen.
- Vergleich ausgewählter Bilder ist kein Beweis beliebiger Pixelgleichheit. Abweichungen untersuchen und dokumentieren.
- Ohne GPU Fallbacktests ausführbar; echte Kerneltests als hardwareabhängig kennzeichnen.

Bei hohem Reparaturanteil darf das Verfahren langsamer als GMP sein. Nicht durch Entfernen von Schutzprüfungen künstlich beschleunigen.
