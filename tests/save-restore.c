#include <stdio.h>
#include "../narena.h"

int main(void) {

  NArena arena = {0};

  char *buf = narena_alloc(&arena, 256);
  int *nums = narena_alloc(&arena, sizeof(int) * 128);
  nums[0] = 20;
  buf[0] = '\0';

  narena_save(&arena);
  for (size_t i = 0; i < 100; i++) {

    char *something = narena_alloc(&arena, 900);
    float *other_thing = narena_alloc(&arena, 120);
    other_thing[0] = 1.23f;
    something[0] = '\0';
    
    narena_restore(&arena);
  }

  puts("SUCCESS: arena save-restore");
  
  narena_destroy(&arena);
  return 0;
}
