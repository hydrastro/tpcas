#include "arena.h"
#include "repl.h"
#include <stdlib.h>

int main(void) {
    arena_t arena;
    arena_init(&arena, 65536);
    int rc = repl_run(&arena);
    arena_destroy(&arena);
    return rc;
}
