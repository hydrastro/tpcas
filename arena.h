#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

/* Bump allocator. All allocations live until arena_reset/arena_destroy.
 * Removes the per-node malloc/free dance and makes tree rewrites cheap:
 * just build new nodes and let the arena clean up. */

typedef struct arena_chunk arena_chunk_t;

typedef struct {
    arena_chunk_t *head;
    size_t chunk_size;
} arena_t;

void  arena_init(arena_t *a, size_t chunk_size);
void *arena_alloc(arena_t *a, size_t bytes);
void *arena_alloc_align(arena_t *a, size_t bytes, size_t align);
char *arena_strdup(arena_t *a, const char *s);
char *arena_strndup(arena_t *a, const char *s, size_t n);
void  arena_reset(arena_t *a);
void  arena_destroy(arena_t *a);

#endif
