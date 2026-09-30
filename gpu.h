#ifndef GPU_H
#define GPU_H
#include <gmp.h>
const char *gpu_perturb(mpf_srcptr left, mpf_srcptr top, mpf_srcptr step, unsigned int limit,
 unsigned char *image, void (*row_ready)(unsigned int, const unsigned char *));
const char *gpu_render(double left, double top, double step, unsigned int limit, unsigned char *image, void (*row_ready)(unsigned int, const unsigned char *));
#endif
