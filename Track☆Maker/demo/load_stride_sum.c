#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { ARRAY_SIZE = 1 << 22, STRIDE = 16, ROUNDS = 256 };

int main(void)
{
  uint32_t* data = (uint32_t*)malloc(sizeof(*data) * ARRAY_SIZE);
  if (data == NULL) {
    return 1;
  }

  for (size_t i = 0; i < ARRAY_SIZE; ++i) {
    data[i] = (uint32_t)(i ^ 0x5a5a5a5aU);
  }

  uint64_t sum = 0;
  for (size_t round = 0; round < ROUNDS; ++round) {
    for (size_t i = 0; i < ARRAY_SIZE; i += STRIDE) {
      sum += data[i];
    }
  }

  printf("load_stride_sum=%llu\n", (unsigned long long)sum);
  free(data);
  return 0;
}
