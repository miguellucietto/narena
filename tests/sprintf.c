#include <stdio.h>
#include "../narena.h"

int main(void) {

  NArena arena = {0};

  puts(narena_sprintf(&arena, "%s", "SUCCESS: arena sprintf"));

  narena_destroy(&arena);
  return 0;
}
