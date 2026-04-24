#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { ARRAY_SIZE = 1 << 20 };

static void init_array(int* data, size_t size)
{
  for (size_t i = 0; i < size; ++i) {
    data[i] = (int)(i * 3U + 1U);
  }
}

int main(void)
{
  int* data = (int*)malloc(sizeof(*data) * ARRAY_SIZE);
  if (data == NULL) {
    return 1;
  }

  init_array(data, ARRAY_SIZE);

  volatile uint64_t sum = 0;

  for (size_t round = 0; round < 64; ++round) {
    for (size_t i = 0; i < ARRAY_SIZE; ++i) {
      sum += (uint64_t)data[i];
    }

    for (size_t i = 0; i < ARRAY_SIZE; i += 8) {
      sum += (uint64_t)data[i];
    }
  }

  printf("seq_stride sum=%llu\n", (unsigned long long)sum);
  free(data);
  return 0;
}
