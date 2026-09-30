#include "render_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>







/* --- Hauptprogramm --- */

int main(int argc, char *argv[]) {
    cli_args_t args;
    int rc = parse_cli(argc, argv, &args);
    if (rc != 0) {
        fprintf(stderr, "error: invalid CLI arguments\n");
        return 2;
    }

    /* Backend wählen */
    const char *backend = BACKEND_AUTO;
    const char *env_backend = getenv("RENDER_BACKEND");
    if (env_backend != NULL && env_backend[0] != '\0') {
        backend = env_backend;
    }

    fprintf(stderr, "backend=%s\n", backend);

    /* Stream-Modus prüfen */
    int stream_mode = 0;
    const char *env_stream = getenv("RENDER_STREAM");
    if (env_stream != NULL && env_stream[0] == '1') {
        stream_mode = 1;
    }

    fprintf(stderr, "stream=%d\n", stream_mode);

    /* Koordinaten initialisieren */
    coord_state_t cs;
    coord_init(&cs);

    /* Zoom-Schritte anwenden (alter span pro Schritt) */
    for (int i = 0; i < args.zoom_steps; i++) {
        coord_zoom(&cs, args.zoom[i].x, args.zoom[i].y, args.zoom[i].size);
    }

    double step, right, bottom;
    coord_final(&cs, &step, &right, &bottom);

    fprintf(stderr, "coord: left=%.10f top=%.10f span=%.15f\n", cs.left, cs.top, cs.span);
    fprintf(stderr, "coord: right=%.10f bottom=%.10f step=%.15f\n", right, bottom, step);

    /* Render-Status (Placeholder für GMP/GPU) */
    uint8_t image[IMG_FULL_SIZE];
    memset(image, 0, IMG_FULL_SIZE);

    /* --- Referenzberechnung (Placeholder für GMP/GPU) --- */
    fprintf(stderr, "computing: iter=%u width=%d height=%d\n",
            (unsigned int)args.iterations, IMG_WIDTH, IMG_HEIGHT);

    for (int py = 0; py < IMG_HEIGHT; py++) {
        double ci = cs.top - (py + 0.5) * step;

        for (int px = 0; px < IMG_WIDTH; px++) {
            double cr = cs.left + (px + 0.5) * step;

            double zr = 0.0, zi = 0.0;
            uint32_t n;
            for (n = 0; n < (uint32_t)args.iterations; n++) {
                double zr_new = zr * zr - zi * zi + cr;
                double zi_new = 2.0 * zr * zi + ci;
                zr = zr_new;
                zi = zi_new;
                if (zr * zr + zi * zi > 4.0) {
                    break;
                }
            }

            int idx = (py * IMG_WIDTH + px) * IMG_CHANNELS;
            if (n < (uint32_t)args.iterations) {
                palette_rgb(n, &image[idx]);
            } else {
                image[idx] = 0;
                image[idx + 1] = 0;
                image[idx + 2] = 0;
            }
        }

        /* Frame-Ausgabe im Stream-Modus */
        if (stream_mode) {
            unsigned char header[FRAME_HEADER_SIZE];
            frame_write_header(header, (unsigned int)py);
            if (write(STDOUT_FILENO, header, FRAME_HEADER_SIZE) != FRAME_HEADER_SIZE ||
                write(STDOUT_FILENO, &image[py * IMG_ROW_BYTES], IMG_ROW_BYTES) != IMG_ROW_BYTES) {
                fprintf(stderr, "error: frame output failed for y=%u\n", py);
            }
        }
    }

    /* Nicht-Stream-Modus: Vollbild ausgeben */
    if (!stream_mode) {
        if (write(STDOUT_FILENO, image, IMG_FULL_SIZE) != IMG_FULL_SIZE) {
            fprintf(stderr, "error: full image output failed\n");
            return 1;
        }
    }

    fprintf(stderr, "done: backend=%s\n", backend);
    return 0;
}
