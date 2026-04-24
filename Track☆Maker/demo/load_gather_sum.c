#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { DATA_SIZE = 1 << 20, INDEX_SIZE = 1 << 18, ROUNDS = 128 };

static uint32_t lcg_next(uint32_t x) { return x * 1103515245U + 12345U; }

int main(void)
{
  uint64_t* data = (uint64_t*)malloc(sizeof(*data) * DATA_SIZE);
  uint32_t* index = (uint32_t*)malloc(sizeof(*index) * INDEX_SIZE);
  if (data == NULL || index == NULL) {
    free(data);
    free(index);
    return 1;
  }

  for (size_t i = 0; i < DATA_SIZE; ++i) {
    data[i] = (uint64_t)(i * 9U + 1U);
  }

  uint32_t state = 1U;
  for (size_t i = 0; i < INDEX_SIZE; ++i) {
    state = lcg_next(state);
    index[i] = state % DATA_SIZE;
  }

  uint64_t sum = 0;
  for (size_t round = 0; round < ROUNDS; ++round) {
    for (size_t i = 0; i < INDEX_SIZE; ++i) {
      sum += data[index[i]];
    }
  }

  printf("load_gather_sum=%llu\n", (unsigned long long)sum);
  free(index);
  free(data);
  return 0;
}
