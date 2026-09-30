# Projekt-Prompt: Mandelbrot-Webanwendung

## Ziel
Eine Docker-basierte Webanwendung zur Darstellung der Mandelbrotmenge (Apfelmännchen). Der Server berechnet das Bild und überträgt fertige Zeilen fortlaufend an den Browser. Per Maus lässt sich ein Ausschnitt zum Hineinzoomen auswählen.

## Anforderungen aus dem bisherigen Gespräch
- Beliebig genaue Berechnung mit GMP für tiefe Zoomstufen.
- Optionale GPU-Berechnung per OpenCL, insbesondere für Intel Arc. Unterstützte Genauigkeit und passende Einstellungen sollen nachvollziehbar sein; eine pauschale Beschränkung auf FP32 ist nicht als gesicherte Eigenschaft vorauszusetzen.
- Zusätzliche Berechnungsvariante mit Referenzbahn (Perturbation).
- Zusätzliche CPU-Variante mit der Intel-FPU und `long double`.
- Automatischer Rückfall auf GMP, wenn die Genauigkeit nicht ausreicht; dieser Wechsel soll sichtbar angezeigt werden.
- Berechnungszeitlimit von fünf Minuten statt 45 Sekunden, da GMP deutlich länger benötigen kann.
- Iterationszahl zusätzlich manuell eingeben können, ausdrücklich auch Werte über 2000.
- Aktuelle Ansicht als Konfigurationsdatei exportieren und wieder importieren können, insbesondere Eckpunkte und Iterationszahl. Koordinaten müssen ihre Genauigkeit behalten.
- Konfigurationsdateien dürfen clientseitig gespeichert werden. Bei serverseitiger Ablage müssen sie außerhalb des Containers dauerhaft erhalten bleiben.
- Laufende Berechnungen abbrechen können, um anschließend Parameter anzupassen. Auch der zugehörige Serverprozess muss beendet werden.
- Nach Änderungen an C-Dateien neu kompilieren und bei Docker-Nutzung das Image neu bauen sowie den Container aktualisieren; ein bloßer Neustart übernimmt solche Quellcodeänderungen nicht.

## Architektur und relevante Dateien
- `render.c`: CPU-Renderer mit GMP beziehungsweise `long double`; Ausgabe der Bildzeilen über stdout.
- `perturb.c`, `perturb.cl`: Berechnung mit Referenzbahn.
- `gpu.c`, `gpu.h`, `mandelbrot.cl`: OpenCL-GPU-Anbindung.
- `server.py`: HTTP-Server, Start und Überwachung der Renderer-Prozesse sowie Streaming zum Browser.
- `app.js`, `index.html`, `style.css`: Weboberfläche, Bilddarstellung, Zoomauswahl, Ansichts-Cache, Konfigurationsimport/-export und Abbruch.
- `Dockerfile`, `Dockerfile.gpu`, `compose.yaml`, `compose.gpu.yaml`: Container-Build und Betrieb.
- `test_*.py`, `test_frontend.cjs`: Tests für Backend, Berechnungsvarianten, Konfiguration, Streaming und Frontend.

## Zuletzt überlieferter Stand
Laut Gesprächszusammenfassung war die Anwendung funktionsfähig und getestet. Zuletzt wurde die Abbruchfunktion umgesetzt:

- Das Frontend verwendet einen `AbortController`, aktiviert den Abbruchknopf während der Berechnung und stellt bei Abbruch die vorherige Ansicht wieder her.
- Der Server erkennt Verbindungsabbrüche über `selectors` und beendet den Renderer im `finally`-Block.
- Berichtet wurden 18 erfolgreiche Python-Unit-Tests, Frontend-VM-Tests und reale HTTP-Integrationstests sowie ein gebauter, gesunder Docker-Container.

Diese Angaben stammen aus dem bisherigen Gespräch und wurden für diese Zusammenfassung nicht erneut durch Testläufe oder eine Laufzeitprüfung verifiziert.

## Wichtige Hinweise für weitere Arbeiten
- Beim EOF einer Renderer-Pipe den Zähler `open_pipes` reduzieren, damit ein noch registrierter Client-Socket die Verarbeitung nicht endlos offen hält.
- Das Abbruchsignal an `fetch` weiterreichen und nach asynchronen Warteoperationen im Stream-Reader prüfen, damit ein Abbruch nicht bis zum nächsten Datenpaket warten muss.
- Frontend-Testumgebungen müssen `AbortController` bereitstellen und asynchrone Abbrüche simulieren können.
- Bestehende Nutzeränderungen, insbesondere in `render.c`, erhalten und nicht unbeabsichtigt überschreiben.

## Arbeitsauftrag für die Fortsetzung
Die bestehende Anwendung in `/root/apfelmaennchen/` gezielt weiterentwickeln. Vor Änderungen den aktuellen Code prüfen, vorhandene Berechnungsvarianten und Nutzeränderungen respektieren und betroffene Funktionen mit passenden Tests validieren. Container nur dann als aktualisiert melden, wenn Neubau und Bereitstellung tatsächlich erfolgt sind.
