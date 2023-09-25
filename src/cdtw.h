#include <stdlib.h>
#include <stdint.h>
#include "sigfish.h"

typedef struct Path
{
  int k;
  int *px;
  int *py;
} Path;


float std_dtw(float *x, float *y, int64_t n, int64_t m, float *cost, int64_t squared);
int path(float *cost, int64_t n, int64_t m, int64_t startx, int64_t starty, Path *p);
void subsequence(float *x, float *y, int64_t n, int64_t m, float *cost);
int subsequence_path(float *cost, int64_t n, int64_t m, int64_t starty, Path *p);

#ifdef TEST_SCALING
void _hw_sdtw(SIG_DTYPE *scaled_x, SIG_DTYPE *scaled_y, int64_t n, int64_t m, COST_DTYPE *scaled_cost);
void _sw_sdtw(SIG_DTYPE *scaled_x, SIG_DTYPE *scaled_y, int64_t n, int64_t m, COST_DTYPE *scaled_cost);
int path_DS(COST_DTYPE *cost, int64_t n, int64_t m, int64_t startx, int64_t starty, Path *p);
int subsequence_path_DS(COST_DTYPE *cost, int64_t n, int64_t m, int64_t starty, Path *p);
#endif