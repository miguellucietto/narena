# narena

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![Language: C](https://img.shields.io/badge/Language-C-blue.svg)

A lightweight, high-performance arena allocator for C. Allocate a large chunk of memory at the beginning of an operation and release all arena memory together when the operation is complete.

## Table of Contents

- [Features](#features)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Building](#building)
- [Contributing](#contributing)
- [License](#license)

## Features

✨ **Simple & Efficient** — Allocate from reusable regions with minimal overhead  
**Zero-Cost Abstraction** — Minimal overhead compared to manual management  
**Single Cleanup** — Free everything with a single call  
**Lightweight** — No external dependencies

## Quick Start

### Basic Usage

```c
#include "narena.h"

int main(void)
{
    NArena arena = {0};

    // Allocation can fail; check the returned pointer.
    int *array = narena_alloc(&arena, sizeof(int) * 128);
    if (!array) {
        narena_destroy(&arena);
        return 1;
    }

    // Use the allocation...

    // Free everything at once.
    narena_destroy(&arena);
    return 0;
}
```

## API Reference

### Core Functions

#### `narena_alloc()`
```c
void *narena_alloc(NArena *arena, size_t amount);
```
Allocates `amount` bytes from the arena. Returns `NULL` if `arena` is `NULL`, `amount` is zero, the size overflows, or an allocation fails. Always check the returned pointer before using it.

---

#### `narena_destroy()`
```c
void narena_destroy(NArena *arena);
```
Frees all memory allocated from the arena. Call this once when you're done.

---

### Convenience Functions

#### `narena_sprintf()`
```c
char *narena_sprintf(NArena *arena, const char *fmt, ...);
```
Allocates and returns a formatted string based on `fmt` and variadic arguments. Returns `NULL` if formatting or allocation fails.

**Example:**
```c
char *greeting = narena_sprintf(&arena, "Hello, %s!", "World");
```

---

#### `narena_strdup()`
```c
char *narena_strdup(NArena *arena, const char *src);
```
Allocates and returns a copy of the source string `src`. Returns `NULL` if an argument is invalid or allocation fails.

**Example:**
```c
char *copy = narena_strdup(&arena, "original");
```

---

#### `narena_realloc()`
```c
void *narena_realloc(NArena *arena, void *old, size_t amount);
```
Resizes an arena allocation to `amount` bytes and returns the resulting pointer. When possible, it resizes the most recent allocation in its region in place. Otherwise, it allocates a new block and copies the first `min(old_size, amount)` bytes. The old block is not freed when a new block is used; it remains part of the arena until reset or destruction. Returns `NULL` for invalid arguments or if allocation fails; in that case, the original allocation remains valid.

**Example:**
```c
void *new_ptr = narena_realloc(&arena, old_ptr, new_size);
if (!new_ptr) {
    // old_ptr remains valid
}
```

---

#### `narena_save() and narena_restore()`
```c
void narena_save(NArena *arena);
void narena_restore(NArena *arena);
```
You can make a checkpoint with `narena_save` and restore to that state with `narena_restore`.

**Example:**
```c
NArena arena = {0};
narena_save(&arena); // save the current arena state
for (int i = 0; i < 10; i++) {
    void *ptr = narena_alloc(&arena, 128);
    if (!ptr) {
        // Handle allocation failure.
        break;
    }
}
narena_restore(&arena); // restore the last saved state
```

## Examples

### String Manipulation

```c
#include "narena.h"
#include <stdio.h>

int main(void)
{
    NArena arena = {0};

    char *str1 = narena_strdup(&arena, "Hello");
    char *str2 = str1 ? narena_sprintf(&arena, "%s, %s!", str1, "World") : NULL;

    if (!str2) {
        narena_destroy(&arena);
        return 1;
    }

    printf("%s\n", str2);  // Output: Hello, World!

    narena_destroy(&arena);
    return 0;
}
```

### Multiple Allocations

```c
#include "narena.h"

int main(void)
{
    NArena arena = {0};

    int *integers = narena_alloc(&arena, sizeof(int) * 100);
    double *floats = narena_alloc(&arena, sizeof(double) * 50);
    char *buffer = narena_alloc(&arena, 1024);

    if (!integers || !floats || !buffer) {
        narena_destroy(&arena);
        return 1;
    }

    // Use all allocations...
    // No need to track individual pointers for cleanup!

    narena_destroy(&arena);
    return 0;
}
```

---

## Building

### Compilation

```bash
cc narena.c main.c -o main
```

### With Additional Flags

```bash
cc -Wall -Wextra -std=c11 narena.c main.c -o main
```

---

## Contributing

Contributions are welcome! Feel free to:
- Report bugs
- Suggest improvements
- Submit pull requests

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

**Made with ❤️ for C developers**
