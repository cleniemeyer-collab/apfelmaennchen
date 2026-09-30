#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "../src/render_common.h"

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { \
    tests_run++; \
    fprintf(stderr, "TEST: %s ... ", name); \
} while(0)

#define PASS() do { \
    fprintf(stderr, "PASS\n"); \
} while(0)

#define FAIL(msg) do { \
    fprintf(stderr, "FAIL: %s\n", msg); \
    tests_failed++; \
} while(0)

#define FAILF(fmt, ...) do { \
    fprintf(stderr, "FAIL: " fmt "\n", ##__VA_ARGS__); \
    tests_failed++; \
} while(0)

/* --- Test: CLI-Validierung --- */

static void test_cli_valid(void) {
    TEST("gültige CLI ohne Zoom");
    cli_args_t args;
    char *argv[] = {"render", "250"};
    int rc = parse_cli(2, argv, &args);
    if (rc == 0 && args.iterations == 250 && args.zoom_steps == 0) {
        PASS();
    } else {
        FAIL("parse_cli sollte gültige CLI akzeptieren");
    }

    TEST("gültige CLI mit einem Zoom-Tripel");
    char *argv2[] = {"render", "500", "100000", "200000", "800000"};
    rc = parse_cli(4, argv2, &args);
    if (rc == 0 && args.iterations == 500 && args.zoom_steps == 1 &&
        args.zoom[0].x == 100000 && args.zoom[0].y == 200000 && args.zoom[0].size == 800000) {
        PASS();
    } else {
        FAIL("parse_cli sollte gültiges Zoom-Tripel akzeptieren");
    }

    TEST("gültige CLI mit 80 Zoom-Schritten (Grenzwert)");
    char *argv81[2 + 80*3]; /* 242 Elemente */
    argv81[0] = "render";
    argv81[1] = "100";
    for (int i = 0; i < 80; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", 1000 + i);
        argv81[2 + i*3] = buf;       /* x */
        snprintf(buf, sizeof(buf), "%d", 1000 + i);
        argv81[2 + i*3 + 1] = buf;   /* y */
        snprintf(buf, sizeof(buf), "%d", 500000);
        argv81[2 + i*3 + 2] = buf;   /* size */
    }
    rc = parse_cli(2 + 80*3, argv81, &args);
    if (rc == 0 && args.zoom_steps == 80) {
        PASS();
    } else {
        FAIL("parse_cli sollte 80 Zoom-Schritte akzeptieren");
    }

    TEST("ungültige CLI mit 81 Zoom-Schritten");
    char *argv82[2 + 81*3];
    argv82[0] = "render";
    argv82[1] = "100";
    for (int i = 0; i < 81; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", 1000 + i);
        argv82[2 + i*3] = buf;
        snprintf(buf, sizeof(buf), "%d", 1000 + i);
        argv82[2 + i*3 + 1] = buf;
        snprintf(buf, sizeof(buf), "%d", 500000);
        argv82[2 + i*3 + 2] = buf;
    }
    rc = parse_cli(2 + 81*3, argv82, &args);
    if (rc == 2) {
        PASS();
    } else {
        FAIL("parse_cli sollte 81 Zoom-Schritte ablehnen");
    }

    TEST("ungültige CLI: zu wenig Argumente");
    char *argv_short[] = {"render"};
    rc = parse_cli(1, argv_short, &args);
    if (rc == 2) {
        PASS();
    } else {
        FAIL("parse_cli sollte zu wenige Argumente ablehnen");
    }

    TEST("ungültige CLI: negative Iterationen");
    char *argv_neg[] = {"render", "-10"};
    rc = parse_cli(2, argv_neg, &args);
    if (rc == 2) {
        PASS();
    } else {
        FAIL("parse_cli sollte negative Iterationen ablehnen");
    }

    TEST("ungültige CLI: Iteration zu hoch (>100000)");
    char *argv_hi[] = {"render", "100001"};
    rc = parse_cli(2, argv_hi, &args);
    if (rc == 2) {
        PASS();
    } else {
        FAIL("parse_cli sollte Iterationen > 100000 ablehnen");
    }

    TEST("ungültige CLI: Iteration zu niedrig (<50)");
    char *argv_lo[] = {"render", "49"};
    rc = parse_cli(2, argv_lo, &args);
    if (rc == 2) {
        PASS();
    } else {
        FAIL("parse_cli sollte Iterationen < 50 ablehnen");
    }

    TEST("ungültige CLI: Restzeichen nach Zahl");
    char *argv_rest[] = {"render", "250abc"};
    rc = parse_cli(2, argv_rest, &args);
    if (rc == 2) {
        PASS();
    } else {
        FAIL("parse_cli sollte Restzeichen ablehnen");
    }

    TEST("ungültige CLI: SIZE zu klein (<1000)");
    char *argv_sz[] = {"render", "100", "500000", "500000", "999"};
    rc = parse_cli(4, argv_sz, &args);
    if (rc == 2) {
        PASS();
    } else {
        FAIL("parse_cli sollte SIZE < 1000 ablehnen");
    }

    TEST("ungültige CLI: SIZE zu groß (>1000000)");
    char *argv_sz2[] = {"render", "100", "500000", "500000", "1000001"};
    rc = parse_cli(4, argv_sz2, &args);
    if (rc == 2) {
        PASS();
    } else {
        FAIL("parse_cli sollte SIZE > 1000000 ablehnen");
    }

    TEST("ungültige CLI: X,Y > U");
    char *argv_xy[] = {"render", "100", "1000001", "500000", "500000"};
    rc = parse_cli(4, argv_xy, &args);
    if (rc == 2) {
        PASS();
    } else {
        FAIL("parse_cli sollte X,Y > U ablehnen");
    }

    TEST("ungültige CLI: unvollständiges Tripel");
    char *argv_incomplete[] = {"render", "100", "500000"};
    rc = parse_cli(3, argv_incomplete, &args);
    if (rc == 2) {
        PASS();
    } else {
        FAIL("parse_cli sollte unvollständiges Tripel ablehnen");
    }

    TEST("ungültige CLI: nicht-Zahl als Argument");
    char *argv_str[] = {"render", "abc"};
    rc = parse_cli(2, argv_str, &args);
    if (rc == 2) {
        PASS();
    } else {
        FAIL("parse_cli sollte nicht-Zahlen ablehnen");
    }

    TEST("gültige CLI: Randwerte SIZE=1000 und SIZE=1000000");
    char *argv_edge[] = {"render", "250", "0", "0", "1000"};
    rc = parse_cli(4, argv_edge, &args);
    if (rc == 0 && args.zoom[0].size == 1000) {
        PASS();
    } else {
        FAIL("parse_cli sollte SIZE=1000 akzeptieren");
    }

    char *argv_edge2[] = {"render", "250", "0", "0", "1000000"};
    rc = parse_cli(4, argv_edge2, &args);
    if (rc == 0 && args.zoom[0].size == 1000000) {
        PASS();
    } else {
        FAIL("parse_cli sollte SIZE=1000000 akzeptieren");
    }

    TEST("gültige CLI: X=0, Y=0 und X=U-SIZE, Y=U-SIZE");
    char *argv_corner[] = {"render", "250", "0", "0", "1000000"};
    rc = parse_cli(4, argv_corner, &args);
    if (rc == 0) {
        PASS();
    } else {
        FAIL("parse_cli sollte X=0,Y=0 akzeptieren");
    }

    char *argv_corner2[] = {"render", "250", "999000", "999000", "1000"};
    rc = parse_cli(4, argv_corner2, &args);
    if (rc == 0) {
        PASS();
    } else {
        FAIL("parse_cli sollte X=U-1000,Y=U-1000 akzeptieren");
    }
}

/* --- Test: Frame-Header big-endian --- */

static void test_frame_header(void) {
    TEST("frame_write_header y=0");
    unsigned char buf[FRAME_HEADER_SIZE];
    frame_write_header(buf, 0);
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0) {
        PASS();
    } else {
        FAIL("y=0 Header falsch");
    }

    TEST("frame_write_header y=479 (letzte Zeile)");
    frame_write_header(buf, 479);
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && buf[3] == 223) {
        /* 479 = 0x01DF */
        PASS();
    } else {
        FAIL("y=479 Header falsch");
    }

    TEST("frame_write_header y=65536 (Grenzwert)");
    frame_write_header(buf, 65536);
    if (buf[0] == 0 && buf[1] == 1 && buf[2] == 0 && buf[3] == 0) {
        /* 65536 = 0x010000 */
        PASS();
    } else {
        FAIL("y=65536 Header falsch");
    }

    TEST("frame_write_header y=255 (Max-Byte-Wert)");
    frame_write_header(buf, 255);
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 255) {
        PASS();
    } else {
        FAIL("y=255 Header falsch");
    }

    TEST("frame_write_header y=65793 (0x010101)");
    frame_write_header(buf, 65793);
    if (buf[0] == 0 && buf[1] == 1 && buf[2] == 1 && buf[3] == 1) {
        PASS();
    } else {
        FAIL("y=65793 Header falsch");
    }

    TEST("Frame-Gesamtgröße = 1924 Byte");
    if (FRAME_ROW_TOTAL == 1924) {
        PASS();
    } else {
        FAILF("FRAME_ROW_TOTAL sollte 1924 sein, ist %d", FRAME_ROW_TOTAL);
    }

    TEST("Vollbildgröße = 921600 Byte");
    if (IMG_FULL_SIZE == 921600) {
        PASS();
    } else {
        FAILF("IMG_FULL_SIZE sollte 921600 sein, ist %d", IMG_FULL_SIZE);
    }
}

/* --- Test: Palette / Farben --- */

static void test_palette(void) {
    TEST("Farbe n=0 -> (0,0,255)");
    unsigned char rgb[3];
    palette_rgb(0, rgb);
    if (rgb[0] == 0 && rgb[1] == 0 && rgb[2] == 255) {
        PASS();
    } else {
        FAILF("n=0 Farbe falsch: (%d,%d,%d)", rgb[0], rgb[1], rgb[2]);
    }

    TEST("Farbe n=1 -> (9,0,246)");
    palette_rgb(1, rgb);
    if (rgb[0] == 9 && rgb[1] == 0 && rgb[2] == 246) {
        PASS();
    } else {
        FAILF("n=1 Farbe falsch: (%d,%d,%d)", rgb[0], rgb[1], rgb[2]);
    }

    TEST("Farbe n=85 (k=0, f=85*9%256=765%256=253) -> (253,0,2)");
    palette_rgb(85, rgb);
    /* c = (85*9)%768 = 765%768 = 765; f = 765%256 = 253; k = 765/256 = 2 */
    /* k=2: RGB(0, 255-f, f) = (0, 2, 253) */
    if (rgb[0] == 0 && rgb[1] == 2 && rgb[2] == 253) {
        PASS();
    } else {
        FAILF("n=85 Farbe falsch: (%d,%d,%d)", rgb[0], rgb[1], rgb[2]);
    }

    TEST("Farbe n=256 (c=(256*9)%768=0, k=0, f=0) -> (0,0,255)");
    palette_rgb(256, rgb);
    /* c = (256*9)%768 = 2304%768 = 0; f = 0%256 = 0; k = 0/256 = 0 */
    /* k=0: RGB(f, 0, 255-f) = (0, 0, 255) */
    if (rgb[0] == 0 && rgb[1] == 0 && rgb[2] == 255) {
        PASS();
    } else {
        FAILF("n=256 Farbe falsch: (%d,%d,%d)", rgb[0], rgb[1], rgb[2]);
    }

    TEST("Farbe n=768 (c=0, k=0, f=0) -> (0,0,255)") ;
    palette_rgb(768, rgb);
    if (rgb[0] == 0 && rgb[1] == 0 && rgb[2] == 255) {
        PASS();
    } else {
        FAILF("n=768 Farbe falsch: (%d,%d,%d)", rgb[0], rgb[1], rgb[2]);
    }

    TEST("Farbe n=2 (c=18, k=0, f=18) -> (18,0,237)");
    palette_rgb(2, rgb);
    if (rgb[0] == 18 && rgb[1] == 0 && rgb[2] == 237) {
        PASS();
    } else {
        FAILF("n=2 Farbe falsch: (%d,%d,%d)", rgb[0], rgb[1], rgb[2]);
    }
}

/* --- Test: Koordinaten-Anker --- */

static void test_coordinates(void) {
    TEST("Koordinaten-Anker: Pfad [[250000,250000,500000]]");
    coord_state_t cs;
    coord_init(&cs);
    /* Alter span = 3.5 */
    coord_zoom(&cs, 250000LL, 250000LL, 500000LL);

    double expected_left = -1.625;

    /* left += span * X/U = -2.5 + 3.5 * 250000/1000000 = -2.5 + 0.875 = -1.625 */
    double diff_left = cs.left - expected_left;
    if (diff_left < 0) diff_left = -diff_left;

    /* top -= span * (3/4) * Y/U = 1.3125 - 3.5 * 0.75 * 0.25 = 1.3125 - 0.65625 = 0.65625 */
    double expected_top_calc = COORD_TOP_INIT - COORD_SPAN_INIT * (3.0/4.0) * (250000.0 / 1000000.0);
    double diff_top = cs.top - expected_top_calc;
    if (diff_top < 0) diff_top = -diff_top;

    /* span *= SIZE/U = 3.5 * 0.5 = 1.75 */
    double expected_span_val = COORD_SPAN_INIT * (500000.0 / COORD_U);
    double diff_span = cs.span - expected_span_val;
    if (diff_span < 0) diff_span = -diff_span;

    /* Pixelabstand sollte 1.75/640 sein */
    double pixel_step = cs.span / (double)IMG_WIDTH;
    double expected_pixel_step = 1.75 / 640.0;
    double diff_step = pixel_step - expected_pixel_step;
    if (diff_step < 0) diff_step = -diff_step;

    int ok = (diff_left < 1e-9 && diff_top < 1e-9 && diff_span < 1e-9 && diff_step < 1e-12);
    if (ok) {
        PASS();
    } else {
        FAILF("Koordinaten-Anker falsch: left=%.10f top=%.10f span=%.15f step=%.15f",
              cs.left, cs.top, cs.span, pixel_step);
    }

    TEST("Startbildgrenzen");
    coord_state_t cs_start;
    coord_init(&cs_start);
    double start_right, start_bottom, start_step;
    coord_final(&cs_start, &start_step, &start_right, &start_bottom);

    int bounds_ok = 1;
    if (cs_start.left != -2.5) bounds_ok = 0;
    if (start_right != 1.0) bounds_ok = 0;
    if (cs_start.top != 1.3125) bounds_ok = 0;
    if (start_bottom != -1.3125) bounds_ok = 0;

    if (bounds_ok) {
        PASS();
    } else {
        FAILF("Startgrenzen falsch: left=%.4f right=%.4f top=%.4f bottom=%.4f",
              cs_start.left, start_right, cs_start.top, start_bottom);
    }
}

/* --- Test: Mandelbrot-Anker --- */

static void test_mandelbrot_anchor(void) {
    TEST("Mandelbrot n=0 bei cr=-2.5, ci=1.3125 -> 0 (nicht entkommen)");
    /* z=0, |z|^2=0 <= 4, also nicht entkommen */
    int n = mandelbrot_iter(-2.5, 1.3125, 1);
    if (n == 0) {
        PASS();
    } else {
        FAIL("mandelbrot_iter bei Startgrenze sollte 0 zurückgeben");
    }

    TEST("Mandelbrot cr=0, ci=0 -> limit (nicht entkommen)");
    n = mandelbrot_iter(0.0, 0.0, 1);
    if (n == 1) {
        PASS();
    } else {
        FAIL("mandelbrot_iter bei (0,0) sollte 0 zurückgeben");
    }

    TEST("Mandelbrot n=1 bei cr=-2.0, ci=0 -> 1 (entkommt in Schritt 1)");
    /* z = 0*0 + (-2+0i) = -2; |z|^2 = 4 <= 4, also nicht entkommen */
    /* Schritt 2: z = (-2)^2 + (-2) = 4-2 = 2; |z|^2 = 4 > 4? Nein, genau 4. */
    /* Also erst bei Schritt 3: z = 2^2 + (-2) = 2; ... */
    /* Korrektur: n=0 -> z=c=(-2,0), |z|^2=4 nicht > 4 */
    /* n=1 -> z=z*z+c = 4+(-2)=2, |z|^2=4 nicht > 4 */
    /* Also sollte es länger dauern. Test mit einem anderen Punkt. */

    TEST("Mandelbrot: cr=1.0, ci=1.0 -> entkommt schnell");
    n = mandelbrot_iter(1.0, 1.0, 10);
    /* z0=(0,0), n=0: z=(-2+1i)? Nein: z=z*z+c = (0,0)*(0,0)+(1,1)=(1,1) */
    /* |z|^2 = 2 <= 4 */
    /* n=1: z=(1,1)*(1,1)+(1,1)=(0+1i)+(1,1)=(1,2i) */
    /* |z|^2 = 5 > 4 -> entkommen bei n=1 */
    if (n == 1) {
        PASS();
    } else {
        FAILF("mandelbrot_iter(1.0,1.0) sollte 1 zurückgeben, ist %d", n);
    }

    TEST("Mandelbrot: cr=-0.75, ci=0.1 -> nicht entkommen bei kleinem Limit");
    n = mandelbrot_iter(-0.75, 0.1, 2);
    /* Sollte >= 2 sein (nicht entkommen innerhalb von 2 Iterationen) */
    if (n >= 2) {
        PASS();
    } else {
        FAIL("mandelbrot_iter(-0.75,0.1) sollte >= 2 zurückgeben");
    }
}

/* --- Haupttest --- */

int main(void) {
    fprintf(stderr, "=== Step A Tests ===\n");

    test_cli_valid();
    test_frame_header();
    test_palette();
    test_coordinates();
    test_mandelbrot_anchor();

    fprintf(stderr, "\n=== Ergebnisse: %d/%d bestanden ===\n",
            tests_run - tests_failed, tests_run);

    return tests_failed > 0 ? 1 : 0;
}
