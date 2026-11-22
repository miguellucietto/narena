/*
 * narena.h - a small, reusable arena allocator for C.
 *
 * Normal use:
 *   #define NARENA_IMPLEMENTATION in exactly one C file before including this
 *   header. Include the header normally in all other files.
 *
 * Garbage collector mode:
 *   Define NARENA_IMPLEMENTATION and NARENA_GARBAGE_COLLECTOR before including
 *   this header in the file that defines the user's main(). See README.md.
 */
#ifndef NARENA_H_INCLUDED
#define NARENA_H_INCLUDED

#include <stddef.h>

#ifndef NARENA_DEFAULT_REGION_SIZE
#define NARENA_DEFAULT_REGION_SIZE ((size_t)4096)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NArenaRegion NArenaRegion;

typedef struct {
    NArenaRegion *region;
    size_t offset;
    int valid;
} NArenaCheckpoint;

typedef struct NArena {
    NArenaRegion *head;
    NArenaRegion *tail;
    NArenaRegion *current;
    NArenaCheckpoint checkpoint;
} NArena;

void *narena_alloc(NArena *arena, size_t amount);
void *narena_calloc(NArena *arena, size_t count, size_t amount);
void *narena_realloc(NArena *arena, void *ptr, size_t amount);
char *narena_strdup(NArena *arena, const char *src);
char *narena_sprintf(NArena *arena, const char *fmt, ...);
void narena_save(NArena *arena);
void narena_restore(NArena *arena);
void narena_reset(NArena *arena);
void narena_destroy(NArena *arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef NARENA_IMPLEMENTATION

#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define NARENA_ALLOCATION_MAGIC UINT64_C(0x4E4152454E414D47)

typedef union {
    struct {
        size_t size;
        uint64_t magic;
    } info;
    max_align_t alignment;
} NArenaAllocationHeader;

struct NArenaRegion {
    unsigned char *data;
    size_t size;
    size_t offset;
    NArenaRegion *next;
};

static int narena_size_add_overflow(size_t a, size_t b)
{
    return a > SIZE_MAX - b;
}

static size_t narena_align_offset(size_t offset)
{
    const size_t alignment = alignof(max_align_t);
    const size_t remainder = offset % alignment;
    if (remainder == 0)
        return offset;
    const size_t padding = alignment - remainder;
    return narena_size_add_overflow(offset, padding) ? SIZE_MAX : offset + padding;
}

static NArenaRegion *narena_region_create(size_t capacity)
{
    NArenaRegion *region;
    if (capacity == 0)
        return NULL;
    region = (NArenaRegion *)malloc(sizeof(*region));
    if (!region)
        return NULL;
    region->data = (unsigned char *)malloc(capacity);
    if (!region->data) {
        free(region);
        return NULL;
    }
    region->size = capacity;
    region->offset = 0;
    region->next = NULL;
    return region;
}

static int narena_region_find_space(NArenaRegion *region, size_t amount,
                                    size_t *offset_out)
{
    const size_t offset = narena_align_offset(region->offset);
    if (offset == SIZE_MAX || offset > region->size ||
        amount > region->size - offset)
        return 0;
    *offset_out = offset;
    return 1;
}

void *narena_alloc(NArena *arena, size_t amount)
{
    NArenaRegion *region;
    size_t total, offset = 0;
    if (!arena || amount == 0 ||
        narena_size_add_overflow(sizeof(NArenaAllocationHeader), amount))
        return NULL;
    total = sizeof(NArenaAllocationHeader) + amount;
    region = arena->current;
    while (region && !narena_region_find_space(region, total, &offset))
        region = region->next;
    if (!region) {
        size_t capacity = total > NARENA_DEFAULT_REGION_SIZE
            ? total : NARENA_DEFAULT_REGION_SIZE;
        region = narena_region_create(capacity);
        if (!region)
            return NULL;
        if (arena->tail)
            arena->tail->next = region;
        else
            arena->head = region;
        arena->tail = region;
        offset = 0;
    }
    arena->current = region;
    {
        NArenaAllocationHeader *header =
            (NArenaAllocationHeader *)(void *)(region->data + offset);
        header->info.size = amount;
        header->info.magic = NARENA_ALLOCATION_MAGIC;
        region->offset = offset + total;
        return (void *)(header + 1);
    }
}

void *narena_calloc(NArena *arena, size_t count, size_t amount)
{
    size_t total;
    void *ptr;
    if (count != 0 && amount > SIZE_MAX / count)
        return NULL;
    total = count * amount;
    if (total == 0)
        return NULL;
    ptr = narena_alloc(arena, total);
    if (ptr)
        memset(ptr, 0, total);
    return ptr;
}

void *narena_realloc(NArena *arena, void *ptr, size_t amount)
{
    NArenaRegion *owner = NULL;
    uintptr_t address;
    size_t user_offset, header_offset, old_size, copy_size;
    if (!arena || !ptr || amount == 0 ||
        narena_size_add_overflow(sizeof(NArenaAllocationHeader), amount))
        return NULL;
    address = (uintptr_t)ptr;
    for (NArenaRegion *region = arena->head; region; region = region->next) {
        uintptr_t start = (uintptr_t)region->data;
        if (region->size > UINTPTR_MAX - start)
            continue;
        if (address >= start + sizeof(NArenaAllocationHeader) &&
            address < start + region->size) {
            owner = region;
            break;
        }
    }
    if (!owner)
        return NULL;
    user_offset = (size_t)(address - (uintptr_t)owner->data);
    header_offset = user_offset - sizeof(NArenaAllocationHeader);
    if (header_offset >= owner->offset)
        return NULL;
    {
        NArenaAllocationHeader *header =
            (NArenaAllocationHeader *)(void *)(owner->data + header_offset);
        if (header->info.magic != NARENA_ALLOCATION_MAGIC)
            return NULL;
        old_size = header->info.size;
        if (old_size > owner->offset - user_offset)
            return NULL;
        if (user_offset + old_size == owner->offset &&
            amount <= owner->size - user_offset) {
            owner->offset = user_offset + amount;
            header->info.size = amount;
            return ptr;
        }
    }
    copy_size = old_size < amount ? old_size : amount;
    {
        void *new_ptr = narena_alloc(arena, amount);
        if (!new_ptr)
            return NULL;
        memcpy(new_ptr, ptr, copy_size);
        return new_ptr;
    }
}

void narena_save(NArena *arena)
{
    if (!arena)
        return;
    arena->checkpoint.region = arena->current;
    arena->checkpoint.offset = arena->current ? arena->current->offset : 0;
    arena->checkpoint.valid = 1;
}

void narena_restore(NArena *arena)
{
    NArenaRegion *region;
    if (!arena || !arena->checkpoint.valid)
        return;
    if (!arena->checkpoint.region) {
        for (region = arena->head; region; region = region->next)
            region->offset = 0;
        arena->current = arena->head;
        return;
    }
    arena->checkpoint.region->offset = arena->checkpoint.offset;
    for (region = arena->checkpoint.region->next; region; region = region->next)
        region->offset = 0;
    arena->current = arena->checkpoint.region;
}

void narena_reset(NArena *arena)
{
    NArenaRegion *region;
    if (!arena)
        return;
    for (region = arena->head; region; region = region->next)
        region->offset = 0;
    arena->current = arena->head;
    arena->checkpoint.valid = 0;
}

void narena_destroy(NArena *arena)
{
    NArenaRegion *region;
    if (!arena)
        return;
    region = arena->head;
    while (region) {
        NArenaRegion *next = region->next;
        free(region->data);
        free(region);
        region = next;
    }
    memset(arena, 0, sizeof(*arena));
}

char *narena_strdup(NArena *arena, const char *src)
{
    size_t length;
    char *copy;
    if (!arena || !src)
        return NULL;
    length = strlen(src);
    if (length == SIZE_MAX)
        return NULL;
    copy = (char *)narena_alloc(arena, length + 1);
    if (copy)
        memcpy(copy, src, length + 1);
    return copy;
}

char *narena_sprintf(NArena *arena, const char *fmt, ...)
{
    va_list args, copy;
    int needed, written;
    char *str;
    if (!arena || !fmt)
        return NULL;
    va_start(args, fmt);
    va_copy(copy, args);
    needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed < 0 || (size_t)needed == SIZE_MAX) {
        va_end(args);
        return NULL;
    }
    str = (char *)narena_alloc(arena, (size_t)needed + 1);
    if (!str) {
        va_end(args);
        return NULL;
    }
    written = vsnprintf(str, (size_t)needed + 1, fmt, args);
    va_end(args);
    return written == needed ? str : NULL;
}

#undef NARENA_ALLOCATION_MAGIC
#endif /* NARENA_IMPLEMENTATION */

#if defined(NARENA_GARBAGE_COLLECTOR)
/* GC mode is intentionally a single-translation-unit application mode. */
#ifndef NARENA_IMPLEMENTATION
#error "NARENA_GARBAGE_COLLECTOR requires NARENA_IMPLEMENTATION in this file"
#endif
static NArena __narena__;
#define main(...) __narena_user_main__(int argc, char **argv)
#undef malloc
#undef calloc
#undef realloc
#undef free
#define malloc(amount) narena_alloc(&__narena__, (amount))
#define calloc(count, amount) narena_calloc(&__narena__, (count), (amount))
#define realloc(ptr, amount) narena_realloc(&__narena__, (ptr), (amount))
#define free(ptr) ((void)(ptr))
#undef main
int __narena_user_main__(int argc, char **argv);
int main(int argc, char **argv)
{
    int result = __narena_user_main__(argc, argv);
    narena_destroy(&__narena__);
    return result;
}
#define main(...) __narena_user_main__(int argc, char **argv)
#endif /* NARENA_GARBAGE_COLLECTOR */

#endif /* NARENA_H_INCLUDED */
