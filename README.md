# narena

`narena` is a small C arena allocator distributed as a single STB-style header.
It groups related allocations and releases them together. The API includes
aligned allocation, zeroed allocation, reallocation, memory and string helpers,
reusable checkpoints, reset, and storage usage counters.

## STB-style header-only use

Define `NARENA_IMPLEMENTATION` in exactly one C file before including the
header. Include it normally in every other file:

```c
/* main.c */
#define NARENA_IMPLEMENTATION
#include "narena.h"

int main(void)
{
    NArena arena = {0};
    int *values = (int *)narena_calloc(&arena, 32, sizeof(*values));
    if (!values) {
        narena_destroy(&arena);
        return 1;
    }
    values[0] = 42;
    narena_destroy(&arena);
    return 0;
}
```

```sh
cc -std=c11 -Wall -Wextra -pedantic main.c -o app
```

## Separate compilation

For convenience, `narena.c` defines `NARENA_IMPLEMENTATION` and includes the
header. In this mode, do not define the implementation macro in another file:

```sh
cc -std=c11 -Wall -Wextra -pedantic main.c narena.c -o app
```

## Optional garbage collector mode

`NARENA_GARBAGE_COLLECTOR` creates a global arena named `__narena__`, renames the
user's `main`, and supplies a wrapper that initializes and destroys the arena
around the user's function. It also redirects `malloc`, `calloc`, `realloc`, and
`free` to the arena:

```c
/* gc_app.c: single translation unit; define these before the header. */
#define NARENA_IMPLEMENTATION
#define NARENA_GARBAGE_COLLECTOR
#include <stdio.h>
#include "narena.h"

int main(void)
{
    char *message = malloc(64);
    if (!message)
        return 1;
    snprintf(message, 64, "memory is reclaimed when main returns");
    puts(message);
    return 0;
}
```

```sh
cc -std=c11 -Wall -Wextra -pedantic gc_app.c -o gc_app
```

The mode accepts `main()` and `main(int argc, char **argv)` and passes command
line arguments through the wrapper. Include standard and third-party headers
**before** `narena.h`, because the allocation macros remain active after the
include. Use this mode in one application source file only; do not combine it
with `narena.c` or another `NARENA_IMPLEMENTATION` definition.

`free(ptr)` is a no-op in this mode. Memory is reclaimed when the user's `main`
returns, so do not use these macros for memory that must be freed individually
or pass arena pointers to the C library's `free`. `realloc(NULL, n)` and zero
sizes return `NULL`, matching the explicit `narena_*` API behavior.

## API

Initialize an arena with `{0}`. You can reuse it after `narena_reset`; calling
`narena_destroy` releases all regions and zeros the structure.

| Function | Behavior |
| --- | --- |
| `void *narena_alloc(NArena *, size_t)` | Allocates bytes aligned for `max_align_t`. A zero size returns `NULL`. |
| `void *narena_alloc_aligned(NArena *, size_t, size_t alignment)` | Allocates with a power-of-two alignment. Alignments below `max_align_t` are raised to `max_align_t`. |
| `void *narena_calloc(NArena *, size_t count, size_t size)` | Allocates and zeros `count * size` bytes; returns `NULL` for zero or overflow. |
| `void *narena_realloc(NArena *, void *, size_t)` | Resizes the latest allocation in place when possible; otherwise allocates and copies. On failure, the original allocation stays valid. |
| `void *narena_memdup(NArena *, const void *, size_t)` | Copies a byte range into the arena. |
| `char *narena_strdup(NArena *, const char *)` | Copies a string, including its terminator. |
| `char *narena_sprintf(NArena *, const char *fmt, ...)` | Formats a string inside the arena. |
| `size_t narena_bytes_used(const NArena *)` | Reports consumed region storage, including headers and alignment padding. |
| `size_t narena_bytes_reserved(const NArena *)` | Reports total region capacity reserved from the system allocator. |
| `void narena_save(NArena *)` | Replaces the arena's single checkpoint with its current state. |
| `void narena_restore(NArena *)` | Returns to the checkpoint; later regions remain allocated for reuse. The checkpoint stays valid. |
| `void narena_reset(NArena *)` | Invalidates allocations and the checkpoint while retaining regions for reuse. |
| `void narena_destroy(NArena *)` | Releases all regions and zeros the arena. |

Functions safely accept a `NULL` arena (`NULL` result or no action). Pointers
become invalid when their storage is rewound by `restore`, `reset`, or
`destroy`. Never pass arena pointers to the C library's `free`.

`narena_realloc` accepts pointers returned by the same arena. If the allocation
is not the latest in its region, it allocates a replacement and copies the
preserved bytes; the old block continues to consume space until reset, a
checkpoint restore that rewinds it, or destruction. A zero size returns `NULL`
and does not release the old block. Reallocation preserves at least the normal
`max_align_t` alignment; use `narena_alloc_aligned` again if a larger alignment
is required.

Each arena has one checkpoint. Saving again replaces it; checkpoints are not
nested. Restore does not return regions to the system allocator; it makes their
storage available for reuse. An arena must not be shallow-copied or used
concurrently without external synchronization.

The default region size is 4096 bytes. Override it by defining
`NARENA_DEFAULT_REGION_SIZE` before including the header. Large allocations get
a region large enough to hold the request and its alignment padding.

## Development

```sh
make tests
```

The example binaries are written to `build/tests/`. This project is licensed
under MIT; see [LICENSE](LICENSE).
