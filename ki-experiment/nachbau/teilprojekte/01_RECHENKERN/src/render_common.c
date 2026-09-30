#include "render_common.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>

void palette_init(unsigned char pal[PALETTE_SIZE]) {
    (void)pal;
}

void palette_rgb(uint32_t n, unsigned char rgb[3]) {
    uint32_t c = (n * 9U) % 768U;
    uint32_t f = c % 256U;
    uint32_t k = c / 256U;

    switch (k) {
        case 0: /* RGB (f, 0, 255-f) */
            rgb[0] = (unsigned char)f;
            rgb[1] = 0;
            rgb[2] = (unsigned char)(255U - f);
            break;
        case 1: /* RGB (255-f, f, 0) */
            rgb[0] = (unsigned char)(255U - f);
            rgb[1] = (unsigned char)f;
            rgb[2] = 0;
            break;
        case 2: /* RGB (0, 255-f, f) */
            rgb[0] = 0;
            rgb[1] = (unsigned char)(255U - f);
            rgb[2] = (unsigned char)f;
            break;
        default:
            rgb[0] = 0;
            rgb[1] = 0;
            rgb[2] = 0;
            break;
    }
}

void frame_write_header(unsigned char buf[FRAME_HEADER_SIZE], unsigned int y) {
    buf[0] = (unsigned char)((y >> 24) & 0xFFU);
    buf[1] = (unsigned char)((y >> 16) & 0xFFU);
    buf[2] = (unsigned char)((y >> 8) & 0xFFU);
    buf[3] = (unsigned char)(y & 0xFFU);
}

int parse_cli(int argc, char *argv[], cli_args_t *out) {
    if (argc < 2) {
        return 2;
    }

    memset(out, 0, sizeof(*out));

    /* Iterationen parsen */
    {
        char *endptr;
        errno = 0;
        long val = strtol(argv[1], &endptr, 10);

        if (*endptr != '\0' || endptr == argv[1]) {
            return 2;
        }

        if (val < 0 || val > (long)ITER_MAX || errno == ERANGE) {
            return 2;
        }

        out->iterations = (uint64_t)val;

        if (out->iterations < ITER_MIN || out->iterations > ITER_MAX) {
            return 2;
        }
    }

    /* Zoom-Tripel parsen */
    {
        int i = 0;
        char *endptr;

        while (i < ZOOM_MAX_STEPS && (2 + i * 3 + 2) <= argc) {
            int base = 2 + i * 3;

            /* X parsen */
            errno = 0;
            long x_val = strtol(argv[base], &endptr, 10);
            if (*endptr != '\0' || endptr == argv[base]) {
                return 2;
            }

            /* Y parsen */
            errno = 0;
            long y_val = strtol(argv[base + 1], &endptr, 10);
            if (*endptr != '\0' || endptr == argv[base + 1]) {
                return 2;
            }

            /* SIZE parsen */
            errno = 0;
            long s_val = strtol(argv[base + 2], &endptr, 10);
            if (*endptr != '\0' || endptr == argv[base + 2]) {
                return 2;
            }
            if (s_val < COORD_SIZE_MIN || s_val > COORD_SIZE_MAX) {
                return 2;
            }

            /* X,Y ≤ U - SIZE prüfen */
            long max_xy = COORD_U - s_val;
            if (x_val < 0 || x_val > max_xy) {
                return 2;
            }
            if (y_val < 0 || y_val > max_xy) {
                return 2;
            }

            out->zoom[i].x = (int64_t)x_val;
            out->zoom[i].y = (int64_t)y_val;
            out->zoom[i].size = (int64_t)s_val;
            i++;
        }

        /* Restliche Argumente sind ungültig */
        if ((2 + i * 3) < argc) {
            return 2;
        }

        out->zoom_steps = i;
    }

    return 0;
}

void coord_init(coord_state_t *cs) {
    cs->left = COORD_LEFT_INIT;
    cs->top = COORD_TOP_INIT;
    cs->span = COORD_SPAN_INIT;
}

void coord_zoom(coord_state_t *cs, int64_t x, int64_t y, int64_t size) {
    double u = (double)COORD_U;
    cs->left += cs->span * ((double)x / u);
    cs->top -= cs->span * ((double)COORD_ASPECT_NUM / COORD_ASPECT_DEN) * ((double)y / u);
    cs->span *= ((double)size / u);
}

void coord_final(const coord_state_t *cs, double *step, double *right, double *bottom) {
    *step = cs->span / (double)IMG_WIDTH;
    *right = cs->left + cs->span;
    *bottom = cs->top - cs->span * ((double)COORD_ASPECT_NUM / COORD_ASPECT_DEN);
}

/* Mandelbrot-Iteration: z=0, n=0..limit-1, z=z*z+c, Abbruch |z|^2>4.
   Gibt nullbasierten Index zurück; limit wenn nicht entkommen. */
int mandelbrot_iter(double cr, double ci, uint32_t limit) {
    double zr = 0.0, zi = 0.0;
    uint32_t n;

    for (n = 0; n < limit; n++) {
        double zr_new = zr * zr - zi * zi + cr;
        double zi_new = 2.0 * zr * zi + ci;
        zr = zr_new;
        zi = zi_new;

        if (zr * zr + zi * zi > 4.0) {
            break;
        }
    }

    return (int)n;
}
