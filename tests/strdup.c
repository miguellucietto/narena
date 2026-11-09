#include <stdio.h>
#include "../narena.h"

int main(void) {

  NArena arena = {0};

  puts(narena_strdup(&arena, "SUCCESS: arena strdup"));
  
  narena_destroy(&arena);
  return 0;
}
