#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { ARRAY_SIZE = 1 << 20, ROUNDS = 128 };

int main(void)
{
  uint64_t* data = (uint64_t*)malloc(sizeof(*data) * ARRAY_SIZE);
  if (data == NULL) {
    return 1;
  }

  for (size_t i = 0; i < ARRAY_SIZE; ++i) {
    data[i] = (uint64_t)(i * 13U + 7U);
  }

  uint64_t sum = 0;
  for (size_t round = 0; round < ROUNDS; ++round) {
    for (size_t i = 0; i < ARRAY_SIZE; ++i) {
      sum += data[i];
    }
  }

  printf("load_seq_sum=%llu\n", (unsigned long long)sum);
  free(data);
  return 0;
}
