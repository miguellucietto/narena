#ifndef NARENA_H
#define NARENA_H

#include <assert.h>
#include <stddef.h>


#ifndef NARENA_DEFAULT_REGION_SIZE
#define NARENA_DEFAULT_REGION_SIZE (1024UL * 1024UL)
#endif

typedef struct Region {
  struct Region *next;
  unsigned char *data;
  size_t size;
  size_t offset;
  size_t old_offset;
} Region;

typedef struct NArena {
    Region *head;
    Region *tail;
} NArena;

// Arena can be initialized with Arena arena = {0};
void *narena_alloc(NArena *arena, size_t amount);
void *narena_realloc(NArena *arena, void *ptr, size_t amount);
void narena_destroy(NArena *arena);

char *narena_strdup(NArena *, const char *);
char *narena_sprintf(NArena *, const char *, ...);

#endif /* NARENA_H */

