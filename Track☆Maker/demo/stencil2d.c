#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { DIM = 512, ROUNDS = 32 };

static size_t idx(size_t row, size_t col) { return row * DIM + col; }

int main(void)
{
  double* a = (double*)malloc(sizeof(*a) * DIM * DIM);
  double* b = (double*)malloc(sizeof(*b) * DIM * DIM);
  if (a == NULL || b == NULL) {
    free(a);
    free(b);
    return 1;
  }

  for (size_t i = 0; i < DIM * DIM; ++i) {
    a[i] = (double)(i % 97U);
    b[i] = 0.0;
  }

  for (size_t round = 0; round < ROUNDS; ++round) {
    for (size_t r = 1; r + 1 < DIM; ++r) {
      for (size_t c = 1; c + 1 < DIM; ++c) {
        b[idx(r, c)] = 0.5 * a[idx(r, c)] + 0.125 * (a[idx(r - 1, c)] + a[idx(r + 1, c)] + a[idx(r, c - 1)] + a[idx(r, c + 1)]);
      }
    }

    double* tmp = a;
    a = b;
    b = tmp;
  }

  printf("stencil2d sample=%f\n", a[idx(DIM / 2, DIM / 2)]);
  free(b);
  free(a);
  return 0;
}
