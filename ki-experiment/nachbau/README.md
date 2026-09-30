# Apfelmännchen lokal: Bauplan für eine lokale KI

Dieses eigenständige Dokumentationsprojekt bereitet die bestehende Anwendung für einen schrittweisen Nachbau und anschließende Weiterentwicklung mit kleinem KI-Kontextfenster auf. Der Ordnername entspricht dem Auftrag: `apflemännchen_lokal`.

**Noch keine neue Implementierung:** Hier liegen Arbeitsaufträge, verbindliche Schnittstellen und Abnahmekriterien. Das bestehende Projekt wird nicht verändert. Die Implementierung soll später in diesem Ordner entstehen. Eine lokale KI ist das Entwicklungswerkzeug; die Anwendung selbst benötigt keine KI, kein Modell und keinen API-Schlüssel.

## Einstieg
1. Der KI [START.md](START.md) geben.
2. Dazu [KERN.md](KERN.md), [STATUS.md](STATUS.md) und genau **einen** Auftrag laden.
3. Nur die im Auftrag genannten Verträge und betroffenen Quellcodedateien nachladen.
4. Einen kleinen Arbeitsschritt implementieren und testen; dann STATUS aktualisieren.
5. Für den nächsten Arbeitsschritt einen frischen Chat starten. Nicht sämtliche Dateien auf einmal laden.

## Reihenfolge und Abhängigkeiten
| ID | Teilprojekt | Voraussetzung |
|---|---|---|
| 01 | [Rechenkern und Prozessprotokoll](teilprojekte/01_RECHENKERN.md) | keine |
| 02 | [GMP-Berechnung](teilprojekte/02_GMP.md) | 01 |
| 03 | [Hauptprogramm / HTTP](teilprojekte/03_HAUPTPROGRAMM.md) | 01–02 |
| 04 | [Darstellung und Zoom](teilprojekte/04_DARSTELLUNG.md) | 03, zunächst Mock möglich |
| 05 | [Abbruch und Fehlerzustände](teilprojekte/05_ABBRUCH.md) | 03–04 |
| 06 | [Konfigurationsdateien](teilprojekte/06_KONFIGURATION.md) | 03–04 |
| 07 | [Palette, Cache und PNG](teilprojekte/07_AUSGABE.md) | 04–06 |
| 08 | [CPU-FPU / long double](teilprojekte/08_LONG_DOUBLE.md) | 01–05 |
| 09 | [Direkte OpenCL-Berechnung](teilprojekte/09_OPENCL.md) | 01–05 |
| 10 | [Referenzbahn / Perturbation](teilprojekte/10_REFERENZBAHN.md) | 02, 09 |
| 11 | [Build und Docker](teilprojekte/11_BETRIEB.md) | CPU-Build ab 02; vollständig nach 10 |
| 12 | [Gesamtabnahme](teilprojekte/12_INTEGRATION.md) | alle |

Nach 01–05 steht eine kleine, nutzbare CPU-Anwendung. GPU ist optional und darf CPU-Builds nicht blockieren. Jeder Auftrag enthält weitere kleine Schritte; nicht den ganzen Auftrag in einer Antwort erzwingen.

## Orientierung
- [KERN.md](KERN.md): kompakter gemeinsamer Projektkontext.
- [vertraege/RENDERER.md](vertraege/RENDERER.md): C-Prozess, Koordinaten und Farben.
- [vertraege/HTTP.md](vertraege/HTTP.md): Browser/Server-Schnittstelle.
- [vertraege/CONFIG.md](vertraege/CONFIG.md): dauerhaftes Dateiformat.
- [STATUS.md](STATUS.md): Übergabe zwischen KI-Sitzungen, bewusst noch offen.
- [WEITERENTWICKLUNG.md](WEITERENTWICKLUNG.md): spätere Änderungen ohne Vollkontext.

## Herkunft und Verlässlichkeit
Der Bauplan wurde anhand von README, Server, C-Renderer, GPU- und Perturbationscode sowie Docker-Konfiguration des vorhandenen Projekts `/root/apfelmaennchen` erstellt. Er ist ohne Zugriff auf dessen Quellcode verwendbar. Die neue Dateiaufteilung ist ein **Zielentwurf**, keine Behauptung über die Struktur des Originals. Bekannte Grenzen der numerischen Verfahren bleiben ausdrücklich erhalten. Frühere Testergebnisse des Originals sind keine Testergebnisse des Nachbaus.
