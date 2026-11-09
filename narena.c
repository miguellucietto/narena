#include "narena.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdalign.h>
#include <limits.h>

#define NARENA_ALLOCATION_MAGIC UINT64_C(0x4E4152454E414D47)

typedef union {
    struct {
        size_t size;
        uint64_t magic;
    } info;

    max_align_t alignment;
} AllocationHeader;


static int narena_add_overflow(size_t a, size_t b)
{
    return a > SIZE_MAX - b;
}

static size_t narena_align_forward(size_t offset, size_t alignment)
{
    size_t remainder = offset % alignment;

    if (remainder == 0)
        return offset;

    size_t padding = alignment - remainder;

    if (narena_add_overflow(offset, padding))
        return SIZE_MAX;

    return offset + padding;
}

static size_t narena_region_size_for(size_t amount)
{
    if (amount > NARENA_DEFAULT_REGION_SIZE)
        return amount;

    return NARENA_DEFAULT_REGION_SIZE;
}

static Region *narena_region_create(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    Region *region = malloc(sizeof(*region));

    if (!region)
        return NULL;

    region->data = malloc(capacity);

    if (!region->data) {
        free(region);
        return NULL;
    }

    region->next = NULL;
    region->size = capacity;
    region->offset = 0;

    return region;
}

static int narena_region_has_space(
    Region *region,
    size_t amount,
    size_t *aligned_offset
)
{
    const size_t alignment = alignof(max_align_t);

    size_t offset = narena_align_forward(
        region->offset,
        alignment
    );

    if (offset == SIZE_MAX)
        return 0;

    if (offset > region->size)
        return 0;

    if (amount > region->size - offset)
        return 0;

    *aligned_offset = offset;

    return 1;
}


void *narena_alloc(NArena *arena, size_t amount)
{
    if (!arena || amount == 0)
        return NULL;

    if (narena_add_overflow(sizeof(AllocationHeader), amount))
        return NULL;

    size_t total_size = sizeof(AllocationHeader) + amount;

    Region *region = arena->current;
    size_t aligned_offset = 0;


    while (region != NULL) {
        if (narena_region_has_space(
                region,
                total_size,
                &aligned_offset
            )) {
            break;
        }

        region = region->next;
    }

    // No existing region has enough space.
    // Create a new one.
    if (region == NULL) {
        size_t capacity = narena_region_size_for(total_size);

        region = narena_region_create(capacity);

        if (!region)
            return NULL;

        if (arena->tail) {
            arena->tail->next = region;
        } else {
            arena->head = region;
        }

        arena->tail = region;
        aligned_offset = 0;
    }

    arena->current = region;

    AllocationHeader *header =
        (AllocationHeader *)(region->data + aligned_offset);

    header->info.size = amount;
    header->info.magic = NARENA_ALLOCATION_MAGIC;

    void *memory = header + 1;

    region->offset = aligned_offset + total_size;

    return memory;
}

void *narena_realloc(NArena *arena, void *ptr, size_t amount)
{
    if (!arena || !ptr || amount == 0)
        return NULL;

    if (narena_add_overflow(sizeof(AllocationHeader), amount))
        return NULL;

    uintptr_t address = (uintptr_t)ptr;

    Region *owner = NULL;

 
    for (Region *region = arena->head;
         region != NULL;
         region = region->next) {

        uintptr_t start = (uintptr_t)region->data;

        if (region->size > UINTPTR_MAX - start)
            continue;

        uintptr_t end = start + region->size;

        if (address >= start + sizeof(AllocationHeader) && address < end) {
            owner = region;
            break;
        }
    }

    if (!owner)
        return NULL;

    size_t user_offset =
        (size_t)(address - (uintptr_t)owner->data);

    size_t header_offset =
        user_offset - sizeof(AllocationHeader);

    if (header_offset >= owner->offset)
        return NULL;

    AllocationHeader *header =
        (AllocationHeader *)(owner->data + header_offset);

    if (header->info.magic != NARENA_ALLOCATION_MAGIC)
        return NULL;

    size_t old_size = header->info.size;

    if (old_size > owner->offset - user_offset)
        return NULL;

    /*
     * Check whether this is the last allocation
     * in its region.
     */

    int is_last = user_offset + old_size == owner->offset;

    /*
     * Try to resize in place.
     */

    if (is_last) {
        size_t available = owner->size - user_offset;

        if (amount <= available) {
            owner->offset = user_offset + amount;
            header->info.size = amount;

            return ptr;
        }
    }

    /*
     * Otherwise allocate new memory and copy.
     */

    void *new_ptr = narena_alloc(arena, amount);

    if (!new_ptr)
        return NULL;

    size_t copy_size = old_size < amount
        ? old_size
        : amount;

    memcpy(new_ptr, ptr, copy_size);

    return new_ptr;
}


void narena_save(NArena *arena)
{
    if (!arena)
        return;

    NArena_checkpoint checkpoint = {
        .region = arena->current,
        .offset = arena->current
            ? arena->current->offset
            : 0,
        .valid = 1
    };

    arena->checkpoint = checkpoint;
}

void narena_restore(NArena *arena)
{
    if (!arena || !arena->checkpoint.valid)
        return;

    Region *checkpoint_region = arena->checkpoint.region;

    /*
     * A checkpoint created before the first allocation.
     */

    if (!checkpoint_region) {
        for (Region *region = arena->head;
             region != NULL;
             region = region->next) {

            region->offset = 0;
        }

        arena->current = arena->head;

        return;
    }

    /*
     * Restore the checkpoint region.
     */

    checkpoint_region->offset = arena->checkpoint.offset;

    /*
     * Invalidate every subsequent region.
     * Their memory remains allocated and reusable.
     */

    for (Region *region = checkpoint_region->next;
         region != NULL;
         region = region->next) {

        region->offset = 0;
    }

    arena->current = checkpoint_region;
}

void narena_reset(NArena *arena)
{
    if (!arena)
        return;

    for (Region *region = arena->head;
         region != NULL;
         region = region->next) {

        region->offset = 0;
    }

    arena->current = arena->head;

    arena->checkpoint.valid = 0;
}

void narena_destroy(NArena *arena)
{
    if (!arena)
        return;

    Region *region = arena->head;

    while (region != NULL) {
        Region *next = region->next;

        free(region->data);
        free(region);

        region = next;
    }

    arena->head = NULL;
    arena->tail = NULL;
    arena->current = NULL;

    arena->checkpoint = (NArena_checkpoint){0};
}

char *narena_strdup(NArena *arena, const char *src)
{
    if (!arena || !src)
        return NULL;

    size_t length = strlen(src);

    if (length == SIZE_MAX)
        return NULL;

    char *dest = narena_alloc(arena, length + 1);

    if (!dest)
        return NULL;

    memcpy(dest, src, length + 1);

    return dest;
}

char *narena_sprintf(
    NArena *arena,
    const char *fmt,
    ...
)
{
    if (!arena || !fmt)
        return NULL;

    va_list args;
    va_list copy;

    va_start(args, fmt);
    va_copy(copy, args);

    int needed = vsnprintf(NULL, 0, fmt, copy);

    va_end(copy);

    if (needed < 0) {
        va_end(args);
        return NULL;
    }

    if ((size_t)needed == SIZE_MAX) {
        va_end(args);
        return NULL;
    }

    char *str = narena_alloc(arena, (size_t)needed + 1);

    if (!str) {
        va_end(args);
        return NULL;
    }

    int written = vsnprintf(
        str,
        (size_t)needed + 1,
        fmt,
        args
    );

    va_end(args);

    if (written < 0 || written != needed)
        return NULL;

    return str;
}
