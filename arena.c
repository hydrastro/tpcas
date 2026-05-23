#include "arena.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct arena_chunk {
    arena_chunk_t *next;
    size_t used;
    size_t cap;
    char data[];
};

static arena_chunk_t *arena_new_chunk(size_t cap) {
    arena_chunk_t *c = malloc(sizeof(arena_chunk_t) + cap);
    if (!c) return NULL;
    c->next = NULL;
    c->used = 0;
    c->cap = cap;
    return c;
}

void arena_init(arena_t *a, size_t chunk_size) {
    a->chunk_size = chunk_size ? chunk_size : 4096;
    a->head = arena_new_chunk(a->chunk_size);
}

void *arena_alloc_align(arena_t *a, size_t bytes, size_t align) {
    if (align == 0) align = 1;
    arena_chunk_t *c = a->head;
    uintptr_t base = (uintptr_t)(c->data + c->used);
    uintptr_t aligned = (base + align - 1) & ~(uintptr_t)(align - 1);
    size_t pad = (size_t)(aligned - base);
    if (c->used + pad + bytes > c->cap) {
        size_t need = bytes + align;
        size_t cap = a->chunk_size > need ? a->chunk_size : need * 2;
        arena_chunk_t *nc = arena_new_chunk(cap);
        if (!nc) return NULL;
        nc->next = a->head;
        a->head = nc;
        c = nc;
        base = (uintptr_t)c->data;
        aligned = (base + align - 1) & ~(uintptr_t)(align - 1);
        pad = (size_t)(aligned - base);
    }
    c->used += pad + bytes;
    void *p = (void *)aligned;
    memset(p, 0, bytes);
    return p;
}

void *arena_alloc(arena_t *a, size_t bytes) {
    return arena_alloc_align(a, bytes, sizeof(void *));
}

char *arena_strndup(arena_t *a, const char *s, size_t n) {
    char *p = arena_alloc_align(a, n + 1, 1);
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

char *arena_strdup(arena_t *a, const char *s) {
    return arena_strndup(a, s, strlen(s));
}

void arena_reset(arena_t *a) {
    /* keep first chunk, free rest */
    if (!a->head) return;
    arena_chunk_t *c = a->head->next;
    while (c) {
        arena_chunk_t *next = c->next;
        free(c);
        c = next;
    }
    a->head->next = NULL;
    a->head->used = 0;
}

void arena_destroy(arena_t *a) {
    arena_chunk_t *c = a->head;
    while (c) {
        arena_chunk_t *next = c->next;
        free(c);
        c = next;
    }
    a->head = NULL;
}
