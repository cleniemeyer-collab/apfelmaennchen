# Weiterentwicklung mit kleinem Kontext

## Passenden Auftrag finden
| Änderungswunsch | Primärer Auftrag | Zusätzlich prüfen |
|---|---|---|
| Layout, Zoom, Eingabefelder | 04 | HTTP-Vertrag |
| Palette, PNG, Cache | 07 | Renderer-Palette, Config |
| Timeout, Abbruch, parallele Anfragen | 03 / 05 | Prozessprotokoll, Frontendzustand |
| Export/Import | 06 | CONFIG-Vertrag und Versionspolitik |
| Genauigkeit oder Leistung | 02 / 08 / 09 / 10 | Pixelzentren, Iterationszählung |
| Neues Backend | 01 plus eigener neuer Auftrag | HTTP, Config, UI, Build, Tests |
| Container/Hostwechsel | 11 | Treiber, Rechte, Architektur |

## Änderungsprompt
Lies KERN.md, den aktuellen Abschnitt aus STATUS.md und Auftrag [ID]. Ziel: [konkrete Änderung]. Bewahre die bestehenden Schnittstellen und Nutzeränderungen. Lies nur die betroffenen Funktionen und Tests. Reproduziere den Fehler bzw. formuliere zuerst ein Abnahmekriterium. Implementiere einen kleinen Schritt, teste ihn und aktualisiere die Übergabe.

Die eckigen Felder sind vom Anwender auszufüllen, keine Dateipfade oder bereits erteilten Aufträge.

## Regeln für Erweiterungen
1. Zuerst reproduzierbarer Test, dann Implementierung.
2. Neuer Backendname erfordert abgestimmte Änderungen an Validierung, Dispatcher, Statusausgabe, UI, Cache und Config.
3. Änderungen der Bildgröße oder Zoomsemantik sind keine lokale Konstante: Verträge, Config-Version, Frameparser, Kernel und UI gemeinsam migrieren.
4. Config-Version 1 niemals still umdeuten. Freie Eckpunkte als Eingabe benötigen ein neues Format mit festgelegter Genauigkeit und Migration.
5. Numerische Optimierungen mit GMP-Referenz und expliziten Grenzfällen prüfen. Schnellere Laufzeit allein beweist keine Korrektheit.
6. Jede neue Funktion in ein eigenes kurzes Aufgabenblatt mit Abhängigkeiten, Dateibesitz und Abnahmetests aufteilen.
7. Nach C-Änderungen kompilieren; bei Docker das richtige Image neu bauen und Container neu erstellen. Ein Neustart allein genügt nicht.

## Hardware- und Sicherheitsgrenzen
GPU-Tests ohne GPU als übersprungen melden, nicht als bestanden. Container-Gerätegruppen und Treiberoptionen pro Host ermitteln. Standardziel ist ein vertrauenswürdiger lokaler Rechner bzw. LAN; öffentliche Bereitstellung erfordert zusätzlich TLS, Authentisierung und Request-Limits.
