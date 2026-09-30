# 09 — Direkte GPU-Berechnung

## Kontext / Dateibesitz
Lies KERN.md und RENDERER.md. Zuständig: gpu.c/.h, mandelbrot.cl, optionaler Dispatcheradapter und GPU-Tests. OpenCL 1.2 als kompatible Basis.

## Kleine Schritte
A. Plattformen und GPU-Gerät finden, ohne Vorhandensein vorauszusetzen. FP64-Fähigkeit abfragen; bei Unterstützung double, sonst float. Intel Arc nicht pauschal FP64 zuschreiben. Fehlende GPU bedeutet GMP-Rückfall, nicht kaputte CPU-Anwendung.
B. GMP-Koordinaten erst nach Rekonstruktion in native Werte konvertieren. Für direkte GPU epsilon aus gewähltem FP32/FP64; scale=max(1,abs(left),abs(top)). Nur bei endlichen Werten und step > 64*epsilon*scale verwenden. Diese Schwelle entspricht dem bisherigen direkten GPU-Pfad.
C. Kernel aus lokaler Datei laden, Buildfehler mit begrenztem Log melden. Je Pixel gleiche Zentren, Iterationszählung und Palette. Fused-Kontraktion deaktivieren, keine relaxed-math-Optionen.
D. Im Streammodus Blöcke von acht Zeilen rechnen und zurücklesen, dann einzeln callbacken. Ressourcen in allen Fehlerpfaden freigeben. Fehler startet GMP-Rückfall; gegebenenfalls bereits gesendete Zeilen werden ersetzt.
E. Erfolg meldet opencl-fp32 oder opencl-fp64. Auto verwendet diesen Pfad bei ausreichender Auflösung, sonst GMP; gmp erzwingt GMP.

## Abnahme
- CPU-Build braucht keine OpenCL-Header/Bibliotheken.
- Ohne Gerät, bei Kernel-Buildfehler und zu kleinem step kontrollierter Rückfall.
- Native Datentypen der Kernelargumente passen zur kompilierten Variante.
- Streaming und Vollbild ergeben dieselben GPU-Pixel.
- Hardwaretests getrennt markieren, ohne Gerät überspringen und als ungeprüft melden.
- GMP-Vergleich auf stabilen Punkten; Grenzabweichungen nicht als unmöglich deklarieren.

Keine hostbezogenen Gruppennummern oder Intel-Treiberoptionen im allgemeinen Code hardcodieren. Betriebsdetails gehören in Auftrag 11.
