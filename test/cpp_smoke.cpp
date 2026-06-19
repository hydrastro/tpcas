#include "tpcas.h"
#include <cassert>
#include <cstdio>

int main() {
    arena_t arena;
    arena_init(&arena, 4096);

    parse_result_t result = parse("forall x. P(x) && Q(x)", &arena);
    assert(result.ok);
    assert(result.ast != nullptr);

    parse_result_t arithmetic = parse("-x + 2.5e-3 * y", &arena);
    assert(arithmetic.ok);
    assert(arithmetic.ast != nullptr);
    assert(ast_is_op_app(arithmetic.ast, &OP_ADD, 2));
    assert(ast_is_op_app(arithmetic.ast->app.args[0], &OP_SUB, 1));
    assert(ast_is_op_app(arithmetic.ast->app.args[1], &OP_MUL, 2));
    assert(op_lookup("+") == &OP_ADD);
    assert(op_lookup("-") == &OP_SUB);
    assert(op_lookup("*") == &OP_MUL);
    assert(op_lookup("/") == &OP_DIV);

    print_expr(stdout, result.ast);
    std::fputc('\n', stdout);

    arena_destroy(&arena);
    return 0;
}
