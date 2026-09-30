# Experiment: Entwicklung durch eine lokale KI

Ziel dieses Teilprojekts ist zu untersuchen, wie weit eine lokale KI eine
Mandelbrot-Webanwendung anhand kleiner, überprüfbarer Arbeitsaufträge selbst
entwickeln kann. Die Anwendung benötigt zur Laufzeit keine KI.

Die funktionierende [Anwendung im Hauptverzeichnis](../README.md) ist ein
separater Vergleichsstand. Sie ist **kein Nachweis**, dass die lokale KI den
Nachbau eigenständig abgeschlossen hat. Die Aufgaben wurden mit KI-Assistenz
vorbereitet; Modell, Einstellungen und menschliche Eingriffe einzelner
Versuche sind in den übernommenen Dateien nicht vollständig dokumentiert.

## Einstieg für Mitleser

- [Aufgabenübersicht mit zwölf Teilprojekten](nachbau/README.md)
- [Startprompt für die lokale KI](nachbau/START.md)
- [Gemeinsamer Kernkontext](nachbau/KERN.md)
- [Überlieferter Arbeitsstatus](nachbau/STATUS.md)
- [Hinweise zur Weiterentwicklung](nachbau/WEITERENTWICKLUNG.md)

Pro Sitzung nur den gemeinsamen Kontext, den relevanten Statusabschnitt und
einen kleinen Arbeitsschritt samt benötigten Verträgen laden. Der Bauplan
ist bewusst für kleine Kontextfenster gedacht. Wer einen unabhängigen Nachbau
untersuchen möchte, sollte der lokalen KI zunächst nicht den fertigen
Quellcode der Hauptanwendung als Lösung bereitstellen.

## Aufgaben

| Nr. | Arbeitsauftrag |
|---|---|
| 01 | [Rechenkern und Dispatcher](nachbau/teilprojekte/01_RECHENKERN.md) |
| 02 | [GMP-Backend](nachbau/teilprojekte/02_GMP.md) |
| 03 | [Hauptprogramm / HTTP](nachbau/teilprojekte/03_HAUPTPROGRAMM.md) |
| 04 | [Darstellung und Zoom](nachbau/teilprojekte/04_DARSTELLUNG.md) |
| 05 | [Abbruch und Fehlerzustände](nachbau/teilprojekte/05_ABBRUCH.md) |
| 06 | [Konfiguration](nachbau/teilprojekte/06_KONFIGURATION.md) |
| 07 | [Palette, Cache und PNG](nachbau/teilprojekte/07_AUSGABE.md) |
| 08 | [CPU-FPU / long double](nachbau/teilprojekte/08_LONG_DOUBLE.md) |
| 09 | [Direkte OpenCL-Berechnung](nachbau/teilprojekte/09_OPENCL.md) |
| 10 | [GMP-Referenzbahn und GPU](nachbau/teilprojekte/10_REFERENZBAHN.md) |
| 11 | [Build, Docker und Betrieb](nachbau/teilprojekte/11_BETRIEB.md) |
| 12 | [Gesamtabnahme](nachbau/teilprojekte/12_INTEGRATION.md) |

Die Abhängigkeiten und Abnahmekriterien stehen in den ursprünglichen
Aufgabenblättern. Schnittstellenverträge:
[Renderer](nachbau/vertraege/RENDERER.md),
[HTTP](nachbau/vertraege/HTTP.md),
[Konfiguration](nachbau/vertraege/CONFIG.md).

## Herkunft und Grenzen des übernommenen Stands

Am 30. September 2026 wurden aus dem lokalen Verzeichnis
`/root/apflemännchen_lokal` **20 Markdown-Dateien und fünf C-/Header-Dateien**
unter `nachbau/` übernommen. Diese 25 Dateien wurden bytegleich kopiert; das
Quellverzeichnis blieb unverändert. Die kompilierte Datei
`teilprojekte/01_RECHENKERN/tests/test_step_a` wurde nicht übernommen.
Der erste Implementierungsversuch ist unter
[nachbau/teilprojekte/01_RECHENKERN/](nachbau/teilprojekte/01_RECHENKERN/)
einsehbar. Dies ist ein übernommener Zwischenstand, keine fertige Anwendung.

Die ursprünglichen Texte bleiben als Vergleichsbasis erhalten:

- Der Bauplan sieht noch einen aktiven Bildauftrag und `auto` (GPU mit
  GMP-Fallback) als Standard vor. Die Hauptanwendung wurde inzwischen auf
  mehrere Aufträge mit Round-Robin und FPU mit GMP-Fallback als Standard
  erweitert. Diese Erweiterungen sind nicht nachträglich in die ursprünglichen
  Abnahmekriterien hineingeschrieben worden.
- `STATUS.md` nennt einleitend noch keine Implementierung, meldet weiter unten
  aber Schritt A als abgeschlossen und 36 bestandene Tests. Die vorhandenen
  Quelldateien wurden mit übernommen; **die damaligen Testergebnisse wurden
  bei diesem Import nicht erneut geprüft**. Der Text ist ein überlieferter
  Statusbericht, keine unabhängige Bestätigung.
- Der Import rekonstruiert keine frühere Git-Historie und enthält kein
  vollständiges Gesprächsprotokoll.

## Eigene Versuche nachvollziehbar dokumentieren

Für einen neuen Versuch einen eigenen Branch oder Fork verwenden und mindestens
festhalten: Ausgangscommit, Modell und Version, Quantisierung, Hardware,
Kontextfenster, Agent/Werkzeuge, genauen Arbeitsauftrag, Änderungen, ausgeführte
Testbefehle und Ergebnisse sowie menschliche oder andere KI-Eingriffe.
Nicht ausgeführte Tests und Abweichungen ausdrücklich nennen. Ergebnisse
können im Forum oder in GitHub Issues diskutiert werden.

## Lizenz

Eigener Projektcode und eigene Dokumentation einschließlich dieses Teilprojekts
stehen, soweit nicht anders gekennzeichnet, unter **GPL-3.0-or-later**.
Siehe [LICENSE](../LICENSE) und die
[Hinweise zu Fremdbibliotheken](../README.md#lizenz).
