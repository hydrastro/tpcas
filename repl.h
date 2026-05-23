#ifndef REPL_H
#define REPL_H

#include "arena.h"

/* Top-level REPL loop.  Reads lines, dispatches to commands, prints results. */
int repl_run(arena_t *arena);

#endif
