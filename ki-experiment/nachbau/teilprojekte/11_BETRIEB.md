# 11 — Build, Docker und lokaler Betrieb

## Kontext / Dateibesitz
Lies KERN.md und die tatsächlich entstandenen Dateinamen/Signaturen. Zuständig: Makefile, Dockerfile, Dockerfile.gpu, compose.yaml, compose.gpu.yaml, .dockerignore und Betriebsanleitung.

## Kleine Schritte
A. Reproduzierbaren CPU-Build ab Auftrag 02 ermöglichen. C-Quellen nach Zielaufteilung kompilieren, GMP, libm und OpenMP verlinken. Flags: -O3 -Wall -Wextra -Werror -ffp-contract=off -fopenmp. Kein Fast-Math. Nach 08 FPU-Quelle aufnehmen.
B. Optionaler GPU-Build zusätzlich mit USE_OPENCL, gpu.c, perturb.c und OpenCL-Bibliothek. Beide .cl-Dateien als Laufzeitressourcen bereitstellen. Kernelpfad zuverlässig auflösen, nicht zufällig vom Aufrufverzeichnis abhängig machen.
C. CPU-Multistage-Image, beispielsweise Alpine 3.22: Build mit build-base/gmp-dev, Runtime python3/gmp/libgomp. GPU separat mit Debian Bookworm, GCC/GMP/OpenCL-Headern beim Build, OpenCL-ICD und passendem Intel-Treiber zur Laufzeit. Neue aufgeteilte Python-Module vollständig kopieren.
D. Nichtroot-Benutzer, read-only, cap_drop ALL, no-new-privileges. Standard vier Threads, OMP_DYNAMIC=FALSE, CPU-Grenze 4, pids_limit 64. CPU-Speicherbudget zunächst 128 MB, GPU 768 MB, anhand Tests überprüfen. GPU benötigt beschreibbaren temporären Treibercache, etwa tmpfs /tmp mit 128 MB.
E. /health als Healthcheck. Port 8080 für lokale Nutzung an 127.0.0.1 binden; LAN-Zugriff bewusst freigeben. Innerhalb Docker muss der HTTP-Server für Portweiterleitung an der Container-Netzwerkschnittstelle lauschen können, also konfigurierbare Bind-Adresse und im Container 0.0.0.0.

## GPU-Hostkonfiguration
/dev/dri/renderD128 nur nach Prüfung durchreichen; Gerätegruppe auf dem Zielhost ermitteln. Die ursprüngliche Gruppe 104 und Optionen NEOReadDebugKeys=1 / OverrideGpuAddressSpace=48 waren hostbezogen, keine allgemeingültigen Defaults. clinfo für Diagnose. Kein privilegierter Container erforderlich.

## Bedienbefehle nach Erstellung der Dateien
- CPU: docker compose up -d --build
- GPU: docker compose -f compose.yaml -f compose.gpu.yaml up -d --build
- GPU-Diagnose: docker compose -f compose.yaml -f compose.gpu.yaml exec -T apfelmaennchen clinfo
- Stoppen mit denselben Compose-Dateien und down.

Diese Befehle setzen den Servicenamen apfelmaennchen in der neuen Compose-Datei voraus. Original und Nachbau nicht gleichzeitig auf demselben Hostport starten; für parallele Nutzung den neuen Hostport ändern.

## Abnahme
CPU-Image bauen, /health prüfen, Bild rechnen, Abbruch und Config-Roundtrip testen. GPU separat prüfen oder Hardwarelücke melden. Nach C-Änderung neu bauen und Container neu erstellen: Restart allein kompiliert nicht. Keine Config-Dateien im Container. Builds können Pakete aus dem Netz laden; Anwendungsbetrieb benötigt keine externen Dienste.
