#ifndef RENDER_GMP_H
#define RENDER_GMP_H
#include <gmp.h>
/* Scheduled workers yield between rows/blocks; standalone rendering is unchanged. */
void render_turn(void);
/* mask == NULL computes every pixel; otherwise only nonzero mask entries. */
unsigned int gmp_rows(mpf_srcptr left, mpf_srcptr top, mpf_srcptr step,
 unsigned long limit, unsigned char *image, const unsigned char *mask,
 unsigned int first, unsigned int rows, void (*row_ready)(unsigned int,const unsigned char *));
#endif
