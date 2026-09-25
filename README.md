# narena

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![Language: C](https://img.shields.io/badge/Language-C-blue.svg)

A lightweight, high-performance arena allocator for C. Allocate a large chunk of memory at the beginning of an operation and eliminate the need for NULL pointer checks throughout your code.

## Table of Contents

- [Features](#features)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Building](#building)
- [Contributing](#contributing)
- [License](#license)

## Features

✨ **Simple & Efficient** — Pre-allocate memory once, use freely without NULL checks  
🎯 **Zero-Cost Abstraction** — Minimal overhead compared to manual management  
🧹 **Single Cleanup** — Free everything with a single call  
📦 **Lightweight** — No external dependencies  

## Quick Start

### Basic Usage

```c
#include "narena.h"

int main(void)
{
    NArena arena = {0};
    
    // Allocate memory from the arena
    int *array = narena_alloc(&arena, sizeof(int) * 128);
    char *buf = narena_alloc(&arena, 256);
    
    // Use your memory safely (no NULL checks needed!)
    // ...
    
    // Free everything at once
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
Allocates `amount` bytes from the arena. Returns a valid pointer (never NULL).

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
Allocates and returns a formatted string based on `fmt` and variadic arguments.

**Example:**
```c
char *greeting = narena_sprintf(&arena, "Hello, %s!", "World");
```

---

#### `narena_strdup()`
```c
char *narena_strdup(NArena *arena, const char *src);
```
Allocates and returns a copy of the source string `src`.

**Example:**
```c
char *copy = narena_strdup(&arena, "original");
```

---

#### `narena_realloc()`
```c
void *narena_realloc(NArena *arena, void *old, size_t amount);
```
Returns a copy of the first `amount` bytes from `old`, allocated in the arena.

**Example:**
```c
void *new_ptr = narena_realloc(&arena, old_ptr, new_size);
```

---

## Examples

### String Manipulation

```c
#include "narena.h"
#include <stdio.h>

int main(void)
{
    NArena arena = {0};
    
    char *str1 = narena_strdup(&arena, "Hello");
    char *str2 = narena_sprintf(&arena, "%s, %s!", str1, "World");
    
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
