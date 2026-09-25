# narena 

Arena allocator in C, the goal here is allocate a big chank of memory
in the begining of an operation and not need to worry if it is a NULL ptr
because someone have no memory left

## Example

```c
#include "narena.h"

NArena arena = {0};

int main(void)
{
    int *array = narena_alloc(&arena, sizeof(int) * 128);
    // You don't need to check if it is NULL
    
    char *buf = narena_alloc(&arena, 256);

    narena_destroy(&arena); // Everything is freed here
    return 0;
}

```

## Compilation

```console
cc narena.c main.c -o main
```

## Functions

* `narena_sprintf` (NArena *arena, const char *fmt, ...) -> char *
return an allocated and formated string corresponding to fmt and __VA_ARGS__

* `narena_strdup` (NArena *arena, const char *src) -> char *
allocate a copy of src and return it

* `narena_realloc` (NArena* arena, void *old, size_t amount) -> void *
return a copy of `amount` bytes from `old`

