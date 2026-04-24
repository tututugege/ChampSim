#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { DATA_SIZE = 1 << 20, INDEX_SIZE = 1 << 18 };

static uint32_t lcg_next(uint32_t x) { return x * 1664525U + 1013904223U; }

int main(void)
{
  int* data = (int*)malloc(sizeof(*data) * DATA_SIZE);
  uint32_t* index = (uint32_t*)malloc(sizeof(*index) * INDEX_SIZE);
  if (data == NULL || index == NULL) {
    free(data);
    free(index);
    return 1;
  }

  for (size_t i = 0; i < DATA_SIZE; ++i) {
    data[i] = (int)(i * 7U + 11U);
  }

  uint32_t state = 1U;
  for (size_t i = 0; i < INDEX_SIZE; ++i) {
    state = lcg_next(state);
    index[i] = state % DATA_SIZE;
  }

  volatile uint64_t sum = 0;
  for (size_t round = 0; round < 64; ++round) {
    for (size_t i = 0; i < INDEX_SIZE; ++i) {
      sum += (uint64_t)data[index[i]];
    }
  }

  printf("gather sum=%llu\n", (unsigned long long)sum);
  free(index);
  free(data);
  return 0;
}
