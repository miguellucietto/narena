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
} Region;

typedef struct NArena_checkpoint {
    Region *region;
    size_t offset;
    int valid;
} NArena_checkpoint;

typedef struct NArena {
    Region *head;
    Region *tail;
    Region *current;

    NArena_checkpoint checkpoint;
} NArena;

// Allocation
void *narena_alloc(NArena *arena, size_t amount);
void *narena_realloc(NArena *arena, void *ptr, size_t amount);

// Lifecycle 
void narena_destroy(NArena *arena);
void narena_reset(NArena *arena);

// Checkpoints 
void narena_save(NArena *arena);
void narena_restore(NArena *arena);

// Strings 
char *narena_strdup(NArena *arena, const char *src);
char *narena_sprintf(NArena *arena, const char *fmt, ...);

#endif /* NARENA_H */
