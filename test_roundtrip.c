/* test_roundtrip.c — verify parse → print → parse gives alpha-eq AST */
#define _POSIX_C_SOURCE 200809L
#include "arena.h"
#include "parse.h"
#include "print.h"
#include "ast.h"
#include <stdio.h>
#include <string.h>

static const char *cases[] = {
    "A",
    "A && B",
    "!A",
    "A && B || C",
    "A && (B || C)",
    "(A => B) => C",
    "A => B => C",
    "A <=> B",
    "!(A && B)",
    "forall x. P(x)",
    "forall x y z. P(x, y, z)",
    "forall x. P(x) => exists y. Q(x, y)",
    "forall x:Nat. P(x)",
    "forall P:Nat -> Bool. forall x:Nat. P(x) || !P(x)",
    "\\x. f(x)",
    "\\f. \\x. f(f(x))",
    "(\\x. x)(a)",
    "f(g(x), h(y, z))",
    NULL
};

int main(void) {
    arena_t arena;
    arena_init(&arena, 65536);
    int pass = 0, fail = 0;
    for (int i = 0; cases[i]; i++) {
        parse_result_t a = parse(cases[i], &arena);
        if (!a.ok) {
            fprintf(stderr, "FAIL parse #1: %s  (%s)\n", cases[i], a.err_msg);
            fail++; continue;
        }
        char buf[1024];
        FILE *m = fmemopen(buf, sizeof buf, "w");
        print_expr(m, a.ast);
        fclose(m);
        parse_result_t b = parse(buf, &arena);
        if (!b.ok) {
            fprintf(stderr, "FAIL parse #2: '%s' -> '%s'  (%s)\n", cases[i], buf, b.err_msg);
            fail++; continue;
        }
        if (!ast_equal(a.ast, b.ast)) {
            fprintf(stderr, "FAIL alpha-eq:\n  in:  %s\n  out: %s\n", cases[i], buf);
            fail++; continue;
        }
        printf("  ok: %-50s -> %s\n", cases[i], buf);
        pass++;
    }
    printf("\n%d passed, %d failed\n", pass, fail);
    arena_destroy(&arena);
    return fail == 0 ? 0 : 1;
}
