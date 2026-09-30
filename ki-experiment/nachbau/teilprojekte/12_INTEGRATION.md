# 12 — Gesamtabnahme

## Kontext / Dateibesitz
Lies KERN.md, STATUS.md und nur Verträge der gerade geprüften Funktion. Zuständig: Integrationstests und Abnahmebericht; keine allgemeine Umgestaltung. Jede folgende Gruppe darf eine eigene Sitzung sein.

## Teststrategie
Tests in tests/ nach Verantwortungsbereich aufteilen. Python-Standardbibliothek unittest für Server/Prozesse; Node-VM oder eingebauter Node-Testläufer optional für Frontend, ohne Node-Laufzeitabhängigkeit der Anwendung. Browser-Smoke-Test ergänzt Mocks. Echte GPU-Tests strikt von CPU-Tests trennen.

Konkrete Testbefehle erst festlegen, wenn die Testdateien existieren; dann in Makefile und STATUS dokumentieren. Jeder Prozess-/Netzwerktest hat eigenes Timeout und cleanup. Produktionslimit nicht auf Testwerte reduzieren.

## Gruppen
1. Numerik: Startgrenzen, ein Zoom, tiefer Zoom, Pixelzentren, n-Zählung, Palette; GMP gegen unabhängige Stichprobenreferenz. Ein/vier Threads pixelgleich.
2. Protokoll: geteilte Header/Zeilen/UTF-8, stderr-Druck, doppelte und ungeordnete y-Werte, fehlende Zeilen, Fehlerexit, kein done bei unvollständigem Bild.
3. Ressourcen: 503 bei belegtem Slot, Timeout 300 Sekunden als Konfiguration prüfen und Logik mit kurzem Testwert ausführen, Slot nach Fehler frei, kein Zombie.
4. Abbruch: echter HTTP-Disconnect vor erster Zeile und mitten im Bild; Folgeauftrag möglich; Frontend stellt vollständigen Snapshot wieder her.
5. UI: manuell 2500/100000, ungültige Eingaben, Zoom bei CSS-Skalierung, Zurück/Start, Preview, Fortschritt und sichtbarer Fallback.
6. Persistenz: Config kanonisch roundtrippen, Manipulation ablehnen, Palette pausiert importieren, unangewandte Eingaben nicht exportieren.
7. Ausgabe: zwölf Cacheplätze, keine Teilbilder, tatsächliche Backend-/Fallbackmetadaten, PNG mit sichtbarer Palette.
8. Native Backends: FPU-Auflösungsrückfall, GPU fehlt/FP32/FP64, Perturbationsreparaturen und Unterlauf. Keine unbewiesene universelle GMP-Pixelgleichheit voraussetzen.
9. Betrieb: CPU-Docker frisch bauen, Healthcheck, nichtroot/read-only. GPU separat, falls Hardware verfügbar. Originalprojekt nicht überschreiben und Portkollision vermeiden.

## Abnahmebericht
Pro Gruppe tatsächlichen Befehl, Ergebnis und ggf. Skip-Grund erfassen. Zusätzlich Compiler/Architektur, LDBL_MANT_DIG, GPU-Gerät/Treiber und Buildvariante dokumentieren. Leistung nur mit Ausschnitt, Iterationen und tatsächlichem Backend vergleichen.

Fertig bedeutet: alle CPU-Pflichtfälle bestanden; optionale Hardwarefälle entweder bestanden oder ausdrücklich ungeprüft. Keine Testzahlen oder Erfolgsmeldungen aus dem Original übernehmen. Bekannte Grenzen der Perturbationsheuristik und nativen Präzision in Nutzerdokumentation erhalten.
