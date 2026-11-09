#include <stdio.h>
#include "../narena.h"

int main(void) {

  NArena arena = {0};

  char *buf = narena_alloc(&arena, 256);
  int *nums = narena_alloc(&arena, sizeof(int) * 128);
  nums[0] = 20;
  buf[0] = '\0';

  puts("SUCCESS: arena allocation");

  narena_destroy(&arena);
  return 0;
}
