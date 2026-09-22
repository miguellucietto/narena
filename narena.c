#include "narena.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>


static size_t narena_align_forward(size_t offset, size_t alignment)
{
  assert(alignment > 0);

  size_t remainder = offset % alignment;

  if (remainder == 0)
    return offset;

  size_t padding = alignment - remainder;

  assert(offset <= SIZE_MAX - padding);
  return offset + padding;
}

static Region *narena_region_create(size_t capacity)
{
  assert(capacity > 0);

  Region *region = malloc(sizeof(*region));
  assert(region != NULL && "Could not allocate Region");

  region->data = malloc(capacity);
  assert(region->data != NULL && "Could not allocate Region data");

  region->next = NULL;
  region->size = capacity;
  region->offset = 0;

  return region;
}

static size_t narena_region_size_for(size_t amount)
{
  if (amount > NARENA_DEFAULT_REGION_SIZE)
    return amount;

  return NARENA_DEFAULT_REGION_SIZE;
}

void *narena_alloc(NArena *arena, size_t amount)
{
  assert(arena != NULL);
  assert(amount > 0);

  const size_t alignment = _Alignof(max_align_t);

  if (arena->head == NULL) {
    size_t region_size = narena_region_size_for(amount);

    arena->head = narena_region_create(region_size);
    arena->tail = arena->head;
  }

  assert(arena->tail != NULL);
  assert(arena->tail->offset <= arena->tail->size);

  size_t aligned_offset = narena_align_forward(arena->tail->offset, alignment);

  _Bool needs_new_region = aligned_offset > arena->tail->size || amount > arena->tail->size - aligned_offset;

  if (needs_new_region) {
    size_t region_size = narena_region_size_for(amount);
    Region *region = narena_region_create(region_size);

    arena->tail->next = region;
    arena->tail = region;

    aligned_offset = 0;
  }

  void *memory = arena->tail->data + aligned_offset;

  arena->tail->old_offset = aligned_offset;
  arena->tail->offset = aligned_offset + amount;

  return memory;
}

void *narena_realloc(NArena *arena, void *ptr, size_t amount)
{
  assert(arena != NULL && ptr != NULL && amount > 0);

  if (arena->tail->data + arena->tail->old_offset == ptr) {
    arena->tail->offset = arena->tail->old_offset;
    ptr = narena_alloc(arena, amount);
    return ptr;
  }

  void *new_ptr = narena_alloc(arena, amount);
  memcpy(new_ptr, ptr, amount);

  return new_ptr;
}

void narena_destroy(NArena *arena)
{
  if (arena == NULL)
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
}


char *narena_strdup(NArena *arena, const char *src)
{
  assert(arena != NULL);
  if (!src) return NULL;

  size_t len = strlen(src) + 1;

  char *dest = narena_alloc(arena, len);

  memcpy(dest, src, len);

  return dest;
}

char *narena_sprintf(NArena *arena, const char *fmt, ...) {

  assert(arena != NULL && fmt != NULL);
  va_list args;
    
  va_start(args, fmt);
  int needed = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  assert(needed > 0);
  
  char *str = narena_alloc(arena, needed + 1);

  va_start(args, fmt);
  vsnprintf(str, needed + 1, fmt, args);
  va_end(args);

  return str;
}
