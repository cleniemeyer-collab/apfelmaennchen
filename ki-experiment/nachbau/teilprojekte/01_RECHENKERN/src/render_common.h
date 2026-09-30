#ifndef RENDER_COMMON_H
#define RENDER_COMMON_H

#include <stdint.h>
#include <stddef.h>

/* Bildabmessungen */
#define IMG_WIDTH 640
#define IMG_HEIGHT 480
#define IMG_CHANNELS 3
#define IMG_ROW_BYTES (IMG_WIDTH * IMG_CHANNELS)
#define IMG_FULL_SIZE (IMG_ROW_BYTES * IMG_HEIGHT) /* 921600 */

/* Frame-Format für Streammodus: 4 Byte big-endian y + 1920 RGB Bytes */
#define FRAME_HEADER_SIZE 4
#define FRAME_ROW_TOTAL (FRAME_HEADER_SIZE + IMG_ROW_BYTES) /* 1924 */

/* Koordinaten-Anker */
#define COORD_LEFT_INIT -2.5
#define COORD_TOP_INIT 1.3125
#define COORD_SPAN_INIT 3.5
#define COORD_ASPECT_NUM 3
#define COORD_ASPECT_DEN 4
#define COORD_U 1000000

/* Zoom-Beschränkungen */
#define ZOOM_MAX_STEPS 80
#define ITER_MIN 50
#define ITER_MAX 100000
#define COORD_SIZE_MIN 1000
#define COORD_SIZE_MAX COORD_U

/* Backends */
#define BACKEND_AUTO "auto"
#define BACKEND_GMP "gmp"
#define BACKEND_LONG_DOUBLE "long-double"
#define BACKEND_PERTURB "perturb"

/* Farben: Palette mit 768 Einträgen (3 * 256) */
#define PALETTE_SIZE 768

/* row_ready Callback-Signatur */
typedef void (*row_callback_t)(unsigned int y, const unsigned char *row);

/* Koordinaten-Status (public für Tests) */
typedef struct {
    double left;
    double top;
    double span;
} coord_state_t;

void coord_init(coord_state_t *cs);
void coord_zoom(coord_state_t *cs, int64_t x, int64_t y, int64_t size);
void coord_final(const coord_state_t *cs, double *step, double *right, double *bottom);

/* Palette und RGB-Färbung */
void palette_init(unsigned char pal[PALETTE_SIZE]);
void palette_rgb(uint32_t n, unsigned char rgb[3]);

/* CLI-Validierung */
typedef struct {
    uint64_t iterations;
    int zoom_steps;
    struct {
        int64_t x, y, size;
    } zoom[ZOOM_MAX_STEPS];
} cli_args_t;

int parse_cli(int argc, char *argv[], cli_args_t *out);

/* Frame-Header big-endian schreiben */
void frame_write_header(unsigned char buf[FRAME_HEADER_SIZE], unsigned int y);

/* Mandelbrot-Iteration (Referenzalgorithmus aus Vertrag) */
int mandelbrot_iter(double cr, double ci, uint32_t limit);

#endif /* RENDER_COMMON_H */
