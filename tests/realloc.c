#include <stdio.h>
#include "../narena.h"

int main(void) {

  NArena arena = {0};

  char *buf = narena_alloc(&arena, 256);
  int *nums = narena_alloc(&arena, sizeof(int) * 128);

  buf = narena_realloc(&arena, buf, 512);
  nums = narena_realloc(&arena, nums, sizeof(int) * 256);

  puts("SUCCESS: arena reallocation");

  narena_destroy(&arena);
  return 0;
}
