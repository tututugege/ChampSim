#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { NODE_COUNT = 1 << 18 };

struct node {
  struct node* next;
  uint64_t value;
};

static void shuffle(size_t* order, size_t size)
{
  uint64_t state = 0x12345678abcdefULL;

  for (size_t i = size - 1; i > 0; --i) {
    state = state * 6364136223846793005ULL + 1ULL;
    size_t j = (size_t)(state % (i + 1));
    size_t tmp = order[i];
    order[i] = order[j];
    order[j] = tmp;
  }
}

int main(void)
{
  struct node* nodes = (struct node*)malloc(sizeof(*nodes) * NODE_COUNT);
  size_t* order = (size_t*)malloc(sizeof(*order) * NODE_COUNT);
  if (nodes == NULL || order == NULL) {
    free(nodes);
    free(order);
    return 1;
  }

  for (size_t i = 0; i < NODE_COUNT; ++i) {
    order[i] = i;
    nodes[i].value = (uint64_t)(i ^ 0x5a5a5a5aU);
    nodes[i].next = NULL;
  }

  shuffle(order, NODE_COUNT);

  for (size_t i = 0; i + 1 < NODE_COUNT; ++i) {
    nodes[order[i]].next = &nodes[order[i + 1]];
  }
  nodes[order[NODE_COUNT - 1]].next = &nodes[order[0]];

  struct node* cur = &nodes[order[0]];
  volatile uint64_t sum = 0;

  for (size_t i = 0; i < NODE_COUNT * 32ULL; ++i) {
    sum += cur->value;
    cur = cur->next;
  }

  printf("pointer_chase sum=%llu\n", (unsigned long long)sum);
  free(order);
  free(nodes);
  return 0;
}
