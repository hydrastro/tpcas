#include "tpcas.h"
#include <cassert>
#include <cstdio>

int main() {
    arena_t arena;
    arena_init(&arena, 4096);

    parse_result_t result = parse("forall x. P(x) && Q(x)", &arena);
    assert(result.ok);
    assert(result.ast != nullptr);

    print_expr(stdout, result.ast);
    std::fputc('\n', stdout);

    arena_destroy(&arena);
    return 0;
}
