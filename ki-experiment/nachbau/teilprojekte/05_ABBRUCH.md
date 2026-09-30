# 05 — Abbruch und Wiederherstellung

## Kontext / Dateibesitz
Lies KERN.md, HTTP.md und die Lebenszyklusfunktionen aus 03/04. Zuständig: diese Funktionen und Abbruchtests, keine globale Prozessabschaltung.

## Kleine Schritte
A. Berechnung-abbrechen-Knopf nur während aktivem Auftrag aktivieren. Pro Anfrage AbortController, signal an fetch weiterreichen. Nach asynchronen Warteoperationen im Reader Abbruch prüfen. AbortError von Netzwerkfehler unterscheiden.
B. Vollständigen alten Snapshot einschließlich Navigation, Palette und Hinweisen wiederherstellen; beim ersten Bild Canvas leeren. Teilbilder nicht cachen/exportieren. Eingaben wieder freigeben. Verspätete Antworten ignorieren.
C. Server überwacht Clientsocket zusätzlich zu Pipes. Requestbody vorher vollständig lesen, Connection: close setzen. EOF/Reset oder unerwartete zusätzliche Eingabe beendet diesen Auftrag. Abbruch muss auch vor der ersten Bildzeile erkannt werden.
D. finally: zugehörigen Renderer beenden, wait/reap, Pipes schließen, Slot freigeben. Niemals alle Prozesse namens render beenden. BrokenPipe beim Senden behandeln.

## Abnahme
- Abbruch vor erster Zeile und mitten im Bild.
- Dummy-Renderer ohne Ausgabe wird nach Disconnect zeitnah beendet; Integrationstest mit begrenzter Wartezeit, etwa fünf Sekunden.
- Anschließender Auftrag funktioniert ohne Warten auf die 300-Sekunden-Deadline.
- Keine Zombies; nur eigener Auftrag betroffen.
- Frontendtest mit AbortController und unterbrechbarer Pause übernimmt nach Abbruch keine weiteren Zeilen.
- Normaler Abschluss hängt trotz registriertem Socket nicht: offene Pipes bei EOF herunterzählen.

Ein Fetch-Mock beweist nicht die Serverprozessbeendigung. Zusätzlich echten lokalen HTTP-Test verwenden.
