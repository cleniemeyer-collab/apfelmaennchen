#ifndef RENDER_GMP_H
#define RENDER_GMP_H

#include "render_common.h"
#include <gmp.h>

/* GMP-basierter Koordinatenzustand.
 * Präzision wird vor Initialisierung gesetzt (128 + 20*zoom_steps Bit).

typedef struct {
    mpz_t left;
    mpz_t top;
    mpz_t span;
    int     prec;
} gmp_coord_state_t;

void gmp_coord_init(gmp_coord_state_t *cs, int zoom_steps);
void gmp_coord_fini(gmp_coord_state_t *cs);
void gmp_coord_zoom(gmp_coord_state_t *cs, int64_t x, int64_t y, int64_t size);
double gmp_coord_step(const gmp_coord_state_t *cs);
void gmp_coord_final(const gmp_coord_state_t *cs, double *right, double *bottom);

#endif /* RENDER_GMP_H */
