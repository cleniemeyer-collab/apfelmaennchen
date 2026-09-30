# Apfelmännchen mit GNU MP

CPU-Variante: `docker compose up -d --build`.
Intel-GPU-Variante (auf diesem Server aktiv):
`docker compose -f compose.yaml -f compose.gpu.yaml up -d --build`.
Danach http://localhost:8080 öffnen.
Von anderen Rechnern: `http://SERVER-IP:8080`. Stoppen: `docker compose down`.
Der externe Port ist in compose.yaml links vom Doppelpunkt einstellbar.

Mit gedrückter Maustaste ein Rechteck aufziehen und loslassen. Die Auswahl
behält das Seitenverhältnis 4:3 bei. Zurück und Startbild navigieren durch die
Ansichten. Iterationen lassen sich per Vorauswahl oder zusätzlich im Feld
„Manuell“ als ganze Zahl von 50 bis 100000 eingeben. Beide Felder bleiben
synchron, auch beim Config-Import. „Neu berechnen“ oder Enter im Zahlenfeld
wendet den Wert an; ungültige Werte ändern das bisherige Bild nicht.
PNG speichern lädt das aktuelle Bild mit der gewählten Farbrotation herunter.
Beim Zoom wird der bisherige Ausschnitt sofort als vergrößerte Vorschau
angezeigt. Fertige Zeilen werden sofort darüber gezeichnet; ein Zähler zeigt
Zeilenanzahl und Prozentfortschritt. Nach Abschluss steht das vollständige PNG
für Download und Bildcache bereit.
Der Browser speichert bis zu zwölf Ansichten (nach Zoom-Pfad, Iterationen und
Rechenweg). Zurück/Startbild verwenden diesen Cache; „Neu berechnen“ umgeht ihn.
Bei einem Fehler wird die letzte erfolgreiche Ansicht wiederhergestellt.
Mit „Berechnung abbrechen“ lässt sich ein laufender Auftrag jederzeit stoppen.
Der Browser schließt seinen Bilddatenstrom; der Server überwacht die Verbindung
auch zwischen Bildzeilen und beendet den zugehörigen Renderer. Die letzte
vollständige Ansicht bleibt erhalten (beim ersten Bild wird die Fläche geleert).
Unvollständige Bilder gelangen nicht in den Cache. Anschließend lassen sich
Iterationen und Rechenweg wieder ändern und mit „Neu berechnen“ anwenden.
Ein Abbruch betrifft nur den eigenen Auftrag, nicht die Aufträge anderer Clients.
Die Vorschau skaliert nur vorhandene Pixel, sie berechnet keine Fraktale.
Neue Rasterpunkte müssen weiterhin serverseitig berechnet werden.

## Konfiguration speichern und laden

„Config speichern“ lädt `apfelmaennchen-config.json` auf den Client herunter.
Gespeichert werden der zuletzt erfolgreich berechnete Zoompfad, die zugehörige
Iterationszahl und Rechenvariante, exakte Eckkoordinaten als Dezimalstrings,
Bildgröße sowie Palettenversatz und -richtung. Noch nicht angewandte Änderungen
in den Rechen-Eingabefeldern werden nicht versehentlich statt des Bildes gespeichert.

„Config laden“ öffnet eine lokale JSON-Datei, validiert sie serverseitig und
berechnet die Ansicht neu (oder verwendet den Bildcache). Die Palette wird
wiederhergestellt, die Animation bleibt pausiert. Bei ungültigen Dateien oder
fehlgeschlagener Berechnung bleibt die bisherige Ansicht erhalten. Dateien
sind auf 16 KiB begrenzt. Es werden keine Dateien im Container gespeichert;
Downloads bleiben auch nach Container-Neustarts oder Neubauten erhalten.

Formatversion 1 verwendet den Zoompfad als maßgebliche Positionsangabe. Die
vier Grenzen `left`, `right`, `top`, `bottom` werden mit exakter Dezimalrechnung
abgeleitet und beim Import geprüft, nicht als JavaScript-Fließkommazahlen
verarbeitet. Ein unabhängiges Editieren nur der Eckpunkte wird daher abgelehnt.
Iterationszahl, Rechenweg und Palette können innerhalb ihrer Grenzen geändert
werden. Endpunkte: `POST /config/export` und `POST /config/import`.

## Farbrotation

Der Regler verschiebt die zyklische Palette um 0–767 Einträge. Jeder Pixel
bekommt die Farbe `Palette[(ursprünglicher Index + Versatz) mod 768]`.
Die Farben laufen dadurch entlang der Iterationsbänder: Blau → Rot → Grün
oder, mit umgekehrter Richtung, Blau → Grün → Rot. Das erzeugt den Eindruck
wandernder Wellen, statt nur die Farbtöne per CSS-Filter zu verändern.

„Rotation starten“ animiert mit 96 Paletteneinträgen pro Sekunde; erneut
klicken pausiert. Die Richtung lässt sich auch während der Animation ändern.
Manuelles Verschieben pausiert ebenfalls. „Originalfarben“ stoppt die
Rotation und setzt den Versatz auf 0. Die Animation startet nie automatisch.

Die Serverpalette benutzt `(Iteration * 9) mod 768`. Die dabei erreichbaren
RGB-Farben sind eindeutig; der Browser rekonstruiert daraus verlustfrei die
ursprünglichen Palettenindizes. Schwarz besitzt einen eigenen, unveränderlichen
Eintrag. Vorschau, Live-Zeilen und gecachte Ansichten verwenden dieselbe
Palette. Die Zoom-Vorschau skaliert Indizes mit Nearest-Neighbour, damit keine
Mischfarben entstehen. Es gibt keine neue Serveranfrage oder Fraktalrechnung.
Beim PNG-Download wird die aktuell dargestellte Palette eingefroren; bei
Versatz 0 bleibt es beim Original-PNG. Canvas-Farbfilter sind nicht mehr nötig.

## Architektur

- Kleiner Python-Standardbibliothek-Webserver, keine Frameworks, kein Node.
- C-Renderer mit dynamisch verlinkter GMP; Alpine installiert GMP über seine
  signierten Pakete (gmp-dev beim Build, gmp im Laufzeit-Image).
- Zoom-Koordinaten werden zunächst immer mit GMP mpf rekonstruiert.
  Der CPU-Rechenweg verwendet GMP auch für sämtliche Iterationen.
  Der optionale GPU-Rechenweg wandelt erst danach in FP32/FP64 um.
  JavaScript berechnet keine Mandelbrot-Iterationen.
- Der Browser sendet nur normierte Auswahlrechtecke als Ganzzahlen. Der Server
  rekonstruiert den gesamten Zoom-Pfad mit 128 + 20 Bit pro Zoom-Schritt.
  Das vermeidet Präzisionsverlust durch Koordinatenübertragung als double.
- 640×480 Pixel, 50–100000 Iterationen, maximal 80 Zoom-Schritte.
- z beginnt bei 0; |z|² > 4 beendet die Iteration. Schwarz bedeutet lediglich,
  dass der Punkt innerhalb des Iterationslimits nicht entkommen ist.
- Standardmäßig bis zu vier aktive Bildaufträge verschiedener Clients.
  Ein gemeinsamer Round-Robin-Scheduler verteilt die Rechenzeit auf beide
  Bild-Endpunkte; erst bei belegten Rechenplätzen erhalten weitere HTTP 503.
  Nach 5 Minuten (300 Sekunden, einschließlich Wartezeiten zwischen Rechenschritten)
  wird der jeweilige Renderer beendet (Fehlerereignis im Live-Stream,
  HTTP 504 bei der klassischen PNG-Schnittstelle). Weniger Iterationen wählen,
  wenn tiefe Ausschnitte zu aufwendig werden.
- Multi-Stage-Alpine-Image, Benutzer ohne Root-Rechte, Read-only-Dateisystem,
  keine Linux-Capabilities, 128 MB RAM und vier CPU-Kerne als Obergrenze.

## Rechenwege und Standardauswahl

Die Auswahl steht in dieser Reihenfolge:

1. **FPU → GMP automatisch** (Standard): native CPU-`long double`-Berechnung,
   bei unzureichender Koordinatenauflösung automatisch GMP.
2. **GPU → GMP automatisch**: OpenCL, bei fehlender GPU oder unzureichender
   Präzision automatisch GMP.
3. **Immer GMP (präzise)**: alle Iterationen mit GMP auf der CPU.
4. **GMP-Referenzbahn + GPU**: experimentelle GPU-Perturbation mit GMP-Korrekturen.

Ohne `backend` verwendet die HTTP-API ebenfalls `long-double`. Bestehende
Config-Dateien behalten ihre explizite Auswahl; `auto` bezeichnet weiterhin
GPU mit GMP-Fallback. `RENDER_BACKEND=gmp` erzwingt serverseitig weiterhin GMP.

## Intel-GPU und automatische Präzisionswahl

`Dockerfile.gpu` verwendet Debian Bookworm mit Intel NEO OpenCL, der
CPU-Dockerfile bleibt auf Alpine. Das GPU-Image ist wegen Treiber und
Compilerbibliotheken rund 424 MB groß; CPU rund 50 MB. Beide laufen ohne Root.
`compose.gpu.yaml` reicht nur den Render-Knoten `/dev/dri/renderD128` durch,
setzt 768 MB RAM als Obergrenze und erlaubt einen temporären Treibercache.
Die Gruppe 104 entspricht auf diesem Host der Geräte-Gruppe. Auf anderen
Hosts mit `stat -c '%g' /dev/dri/renderD128` prüfen und `group_add` anpassen.

Die Arc A310 auf diesem i5-3470-Host benötigt mit dem verwendeten Treiber
`NEOReadDebugKeys=1` und `OverrideGpuAddressSpace=48`, um erkannt zu werden.
Diese hostbezogenen Einstellungen stehen im GPU-Compose-Override. Diagnose:
`docker compose exec -T apfelmaennchen clinfo`.

Im Modus „GPU → GMP automatisch“ wird ein verfügbares OpenCL-GPU-Gerät
verwendet, sofern der Pixelabstand mehr als 64 Maschinen-Epsilons multipliziert
mit max(1, |linker Rand|, |oberer Rand|) beträgt. Unterstützt die GPU FP64,
wird dieses verwendet, sonst FP32 (wie auf dieser Arc A310). Bei tieferen
Zooms oder OpenCL-Fehlern folgt automatisch die parallele GMP-Berechnung.
Ein Prozess-Zeitlimit führt zu einer Fehlermeldung, nicht zu einem erneuten
Rechenversuch.

Diese konservative Schwelle schützt die Koordinatenauflösung, garantiert
aber keine mit GMP identischen Iterationsergebnisse: nahe der Fraktalgrenze
können einzelne Pixel wegen Rundungsfehlern abweichen. Für Präzisionsvergleiche
„Immer GMP (präzise)“ wählen und neu berechnen. Der aktive Rechenweg erscheint
unter dem Bild und im Abschlussereignis des Streams (bei `/render` im
HTTP-Header `X-Render-Backend`). `RENDER_BACKEND=gmp`
im Container erzwingt GMP auch bei automatischer Auswahl im Frontend.

Für Änderungen/Neustarts der GPU-Variante immer beide Compose-Dateien angeben;
ein CPU-Build mit nur `compose.yaml` schaltet zurück auf die CPU-Variante.

## CPU / long double (FPU)

Diese zusätzliche Auswahl berechnet die Pixel auf vier CPU-Threads mit nativen
`long double`-Operationen, ohne GMP-Aufrufe in der Iterationsschleife. Auf dem
hier verwendeten x86-64-System bietet das x87-Format 64 Bit Mantisse (etwa
19 Dezimalstellen). Der 80-Bit-Wert belegt inklusive Padding 16 Byte Speicher;
das ist keine 128-Bit-Gleitkommapräzision. Andere Architekturen können ein
anderes `long double` verwenden; Mantissenbits und Speichergröße werden geloggt.

Zoomkoordinaten entstehen weiterhin zuerst mit GMP und werden über einen
Dezimalstring direkt nach `long double` konvertiert, ohne den Umweg über
`double`. Der Pixelabstand muss größer als 64 `LDBL_EPSILON` multipliziert mit
der Koordinatenskala sein. Andernfalls wird automatisch GMP genutzt und auch
als aktiver Rechenweg angezeigt. Bereits beim Umschalten erscheint im
Live-Stream „FPU → GMP“ mit dem Grund (höhere Präzision für diesen Zoom).
Der Hinweis bleibt während des Zeilenfortschritts, nach Abschluss und beim
Abruf aus dem Bildcache sichtbar. Diese Auflösungsprüfung verhindert keine
Rundungsabweichungen der Iteration nahe der Fraktalgrenze. Für maximale
Präzision weiterhin „Immer GMP“ wählen. Es wird keine Quadratwurzel gezogen.

Die Variante unterstützt Live-Zeilen, Palette, Bildcache und Config-Dateien.
API-Auswahl: `backend: "long-double"`; Ergebnis-Backend: `cpu-long-double`
oder `gmp` beim Rückfall. Sie benötigt keine GPU und ist auch im Alpine-Image
enthalten. Diese Variante ist jetzt die Standardauswahl.

## GMP-Referenzbahn + GPU (experimentell)

Die zusätzliche Auswahl `perturb` berechnet eine Referenzbahn für die Bildmitte
mit GMP. Auf der GPU werden nur die Abweichungen iteriert:
`delta_next = 2 * Z * delta + delta² + delta_c`. Dadurch müssen sehr kleine
Pixelabstände nicht zu großen FP32-Absolutkoordinaten addiert werden.

Fehlerabschätzungen, Auslöschung, Überläufe oder eine zu früh endende
Referenzbahn markieren Pixel zur vollständigen GMP-Nachberechnung. Zeilen
werden erst nach diesen Korrekturen übertragen. Unterhalb eines Pixelabstands
von `1e-30` oder bei fehlendem OpenCL fällt die Variante ganz auf GMP zurück.
Sie verwendet noch keine Exponentenskalierung oder mehrere Referenzbahnen.

Die Prüfungen sind eine praktische Absicherung, kein mathematischer Beweis
identischer Ergebnisse für beliebige Ausschnitte. Bei vielen Korrekturen kann
sie langsamer als direktes GMP sein. Die bisherigen Vergleiche für Startbild,
einen Randausschnitt und einen tiefen Zoom waren pixelgleich; die vollständige
Referenzbahn-Testsuite wurde vom Anwender unterbrochen. Die Variante bleibt
explizit auswählbar und wird nicht automatisch aktiviert. Logs enthalten
Referenzlänge und Korrekturanzahl.

## Mehrere CPU-Kerne

Bei HTTP-Aufträgen verteilt der C-Renderer die Pixel einer Zeile mit OpenMP
auf mehrere Threads. Jeder Thread besitzt eigene GMP-Rechenvariablen;
Bildkoordinaten werden nur lesend geteilt. So nutzt auch eine einzelne Zeile
mehrere Kerne. Ohne Server-Scheduling verteilt der eigenständig aufgerufene
Renderer weiterhin ganze Zeilen dynamisch auf die Threads; diese können
außer Reihenfolge eintreffen.

## Mehrere Clients und faire Rechenzeit

Jeder Auftrag besitzt einen eigenen Renderer, Bildpuffer und Datenstrom.
Vor jeder CPU/FPU-Zeile beziehungsweise jedem GPU-Block aus acht Zeilen
fordert der Renderer über eine interne Pipe eine Rechenfreigabe an. Der
Server erteilt sie in FIFO-/Round-Robin-Reihenfolge: Client 1, Client 2,
wieder Client 1 usw. GMP-Pixelkorrekturen gehören zum jeweiligen GPU-Block.
Auch die Initialisierung inklusive Referenzbahn wird als eigener Rechenschritt
freigegeben. Es rechnet höchstens ein solcher Schritt gleichzeitig;
wartende OpenMP-Threads schlafen, statt Rechenzeit zu verbrauchen.

Dies ist kooperatives Scheduling, keine feste Zeitscheibe: Eine besonders
teure Zeile, GPU-Kompilierung oder Referenzbahn kann andere Aufträge bis zum
nächsten Wechsel verzögern. Abbruch und Zeitlimit werden trotzdem auch während
des Wartens überwacht und beenden nur den betroffenen Renderer. Ein langsamer
Netzwerkclient wird durch das Socket-Zeitlimit begrenzt.

`MAX_RENDER_JOBS` in `compose.yaml` begrenzt aktive Aufträge (Standard 4,
zulässig 1–16). Darüber hinaus antwortet der Server mit HTTP 503, statt
unbegrenzt Prozesse und GPU-Kontexte anzulegen. Höhere Werte benötigen mehr
RAM und gegebenenfalls höhere Container-Prozesslimits. `/render/stream` und
`/render` teilen sich denselben Scheduler; die bisherigen Antwortformate
bleiben kompatibel.

Die GMP-Schleife übernimmt die Quadrate aus dem Abbruchtest in die nächste
Iteration: drei statt fünf `mpf_mul`-Aufrufe pro Iteration, ohne Änderung der
Präzision oder des Abbruchtests. Es wird keine Quadratwurzel berechnet.

Standardmäßig sind vier Threads eingestellt. In `compose.yaml` lassen sich
`environment.OMP_NUM_THREADS` (Thread-Anzahl) und `cpus` (CPU-Obergrenze)
ändern, beispielsweise beide auf 2. Anschließend `docker compose up -d`
ausführen. Beim ersten Update ist `docker compose up -d --build` nötig.
Mehr Threads als verfügbare CPU-Kerne bringen normalerweise keinen Vorteil.
Die Beschleunigung hängt vom Bildausschnitt und der übrigen Serverlast ab.

Für ein vertrauenswürdiges LAN gedacht. Vor Veröffentlichung im Internet
Reverse-Proxy mit TLS, Zugriffsschutz und Request-Limits vorschalten.
Port 8080 wird standardmäßig an allen Host-Netzwerkschnittstellen geöffnet.
Keine externen Skripte, Fonts, Analyse-Dienste oder API-Schlüssel erforderlich.

## Live-Zeilenübertragung

Das Frontend nutzt `POST /render/stream` mit denselben JSON-Aufträgen wie
`POST /render`. Die Antwort ist ein NDJSON-Stream:

- `notice`: Sofortiger Hinweis `message` mit maschinenlesbarem `reason`, etwa
  `long-double-precision` beim automatischen FPU→GMP-Rückfall.
- `row`: Zeilennummer `y` und Base64-kodierte RGB-Bytes in `rgb`.
- `done`: Rechenweg `backend`, Laufzeit `seconds` und fertiges PNG in `png`
  (Base64). Erst dieses Ereignis erlaubt die Aufnahme in den Bildcache.
- `error`: Fehlermeldung `message`; das Frontend stellt die letzte vollständige
  Ansicht wieder her. Auch ein vorzeitig beendeter Stream gilt als Fehler.

GMP liefert einzelne fertige Zeilen; OpenCL berechnet Blöcke aus acht Zeilen.
Der Server leitet die Ergebnisse ohne Vollbild-Pufferung vor der ersten
Ausgabe weiter und erstellt parallel das finale PNG. Der Browser zeichnet
nur die gelieferten Farbwerte und berechnet weiterhin keine Fraktalpunkte.
Die bestehende PNG-Schnittstelle `/render` bleibt kompatibel.

Verbindungsabbrüche beenden den zugehörigen Renderer und geben dessen
Rechenplatz sowie eine eventuell gehaltene Rechenfreigabe frei. Der Stream benötigt durch unkomprimierte RGB-Zeilen plus
Base64 rund 1,3 MB pro Bild, zusätzlich zum abschließenden PNG. Hinter einem
Reverse-Proxy muss Response-Buffering deaktiviert sein; der Server setzt
hierfür auch `X-Accel-Buffering: no`. Netzwerk und Browser können mehrere
Zeilen zu einem sichtbaren Update zusammenfassen. Proxy-/Gateway-Zeitlimits
müssen mindestens fünf Minuten plus Übertragungsreserve zulassen. Das
300-Sekunden-Rechenlimit gilt für alle Rechenwege und beide Bild-Endpunkte;
die kurzen Socket-Limits schützen weiterhin gegen blockierte Netzwerk-I/O.

## Tests

`python3 -m unittest -v test_server test_stream_unit test_config` prüft Validierung,
exakten Config-Roundtrip (auch 80 Zoomschritte),
PNG-Kodierung, Stream-Framing, Zeitlimit und Aufräumen von Renderer-Prozessen,
Round-Robin-Fairness, parallele HTTP-Aufträge, Kapazitätsgrenze sowie Abbruch
wartender Clients ohne Beeinträchtigung anderer Aufträge.
`python3 test_stream_http.py` prüft die echte Live-Ausgabe für CPU und GPU,
Pixelgleichheit zur PNG-Schnittstelle, Parallelaufträge und Verbindungsabbruch.
`python3 -m unittest -v test_scheduler_render` prüft mit gebautem `./render`
Pixelgleichheit zum eigenständigen Renderer mit einem und vier Threads,
FPU→GMP-Rückfall, vier aktive Clients (gemischte und reine GPU-Aufträge),
HTTP/PNG-Kompatibilität und Abbruch. Ohne Renderer wird diese Suite übersprungen.
`python3 test_http.py` prüft den laufenden Container einschließlich Startbild,
Zoom, Zurück-Reproduzierbarkeit und ungültiger Eingaben.
`python3 test_parallel.py` vergleicht im laufenden Container die Pixeldaten
mit einem und vier Threads für Startbild, Zoom und tiefen Zoom. Das Startbild
wird zusätzlich gegen die Prüfsumme des ursprünglichen Renderers geprüft.
Der Test gibt auch Laufzeiten aus (einschließlich Docker-Aufruf).
`python3 test_optimization.py` prüft Startbild und Zoom bei höheren
Iterationslimits gegen die Prüfsummen vor der Schleifenoptimierung und
misst jeweils den Median aus drei Durchläufen.
`python3 test_gpu.py` benötigt die GPU-Variante und prüft reale GPU-Ausführung,
GMP-Umschaltung bei tiefem Zoom, fehlenden Treiber und HTTP-Rechenwegauswahl.
`python3 test_long_double.py` prüft native Präzision, Thread-Reproduzierbarkeit,
Vergleich zu GMP, Tiefzoom-Rückfall, HTTP/Streaming und Config-Roundtrip.
`python3 test_config_http.py` prüft Export/Import über HTTP einschließlich
Bildvergleich. `python3 test_perturb.py` ist die längere Referenzbahn-Testsuite.
`node test_frontend.cjs` prüft Config-Bedienung, Live-Zeilen, geteilte Netzwerkpakete, Fortschritt,
Vorschau, Cache, Fehler-Rücknahme und Freigabe alter Bilder mit Browser-API-Mocks. Node ist nur für diesen Test nötig, nicht
im Anwendungscontainer. Ein visueller Browsertest wird dadurch nicht ersetzt.

## Quellen

- https://gmplib.org/#DOWNLOAD – GNU MP (Upstream-Version 6.3.0 zum Erstellungszeitpunkt)
- https://www.mathematische-basteleien.de/apfelmaennchen.htm – mathematische Beschreibung

GMP unterliegt LGPLv3 oder GPLv2; bei Weitergabe gelten die entsprechenden
Lizenz- und Quellcodepflichten. GMP bleibt dynamisch verlinkt und austauschbar.
