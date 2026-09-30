# Startprompt für die lokale KI

Du implementierst oder pflegst eine Mandelbrot-Webanwendung in kleinen, testbaren Schritten. Lies zuerst KERN.md und STATUS.md. Wähle den ersten offenen, freigegebenen Schritt, dessen Voraussetzungen erfüllt sind. Lies danach nur seinen Auftrag und die dort genannten Verträge. Fehlen dir diese Dateien, fordere genau diese an statt das gesamte Projekt.

## Arbeitsregeln
- Pro Sitzung ein begrenztes Ziel: etwa eine Funktion, ein Testfall oder ein Integrationsschritt.
- Verträge sind verbindlich. Andere Module nicht nebenbei umgestalten.
- Bereits vorhandenen Code zuerst gezielt lesen, Nutzeränderungen erhalten.
- Keine Browser-Fraktalrechnung, keine externen Webdienste, keine neuen Frameworks ohne Begründung.
- Keine Scheingenauigkeit: native Gleitkommazahlen ersetzen GMP bei tiefen Zooms nicht.
- Build und Tests tatsächlich ausführen, sofern Werkzeuge verfügbar sind. Sonst klar als nicht ausgeführt markieren.
- Keine automatischen Git-Commits oder Änderungen an der ursprünglichen Anwendung.

## Kontextbudget
Plane höchstens etwa die Hälfte des Kontextfensters für Eingaben ein; reserviere den Rest für Code, Tests und Antwort. Bei sehr kleinen Fenstern (z. B. 4k Token) nur KERN, aktuellen Statusauszug, einen Unterabschnitt des Auftrags und die benötigten Vertragsabschnitte laden. Auch diese Kombination kann zu groß sein: dann weiter teilen. Tokenzahl ist modellabhängig; Wortzahl ist nur eine Orientierung.

Keine kompletten Testlogs oder großen Quelldateien laden. Suche nach Funktionen, lies gezielte Abschnitte. Wenn der Kontext knapp wird: keinen neuen Schritt anfangen, sondern Übergabe schreiben.

## Ablauf je Sitzung
1. Nenne Ziel, betroffene Dateien und Abnahmetest kurz.
2. Implementiere nur diesen Schritt; temporäre Mocks ausdrücklich kennzeichnen.
3. Prüfe zuerst den betroffenen Test, danach die direkte Integration.
4. Aktualisiere STATUS.md: umgesetzt, offene Punkte, geänderte Schnittstellen, exakte Testbefehle und Ergebnisse.
5. Hinterlasse einen konkreten nächsten Schritt mit den dafür benötigten Dateien.

## Abschlussformat für die Übergabe
- Schritt / Ergebnis
- Geänderte Dateien und wichtige Symbole
- Tests: Befehl, Ergebnis, nicht geprüfte Voraussetzungen
- Bekannte Fehler oder Einschränkungen
- Nächster kleiner Auftrag und benötigter Kontext

Ein Auftrag ist erst fertig, wenn seine Abnahmekriterien erfüllt sind oder Abweichungen ausdrücklich dokumentiert und vom Nutzer akzeptiert wurden.
