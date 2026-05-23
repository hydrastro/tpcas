#define _POSIX_C_SOURCE 200809L
#include "repl.h"
#include "parse.h"
#include "print.h"
#include "eval.h"
#include "transform.h"
#include "lex.h"
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
 * Command dispatch. Replaces the regex maps with a simple table.
 * Each command takes the rest of the line (already stripped of
 * the leading command word and one space).
 *
 * The expression parser handles outer parens itself, so writing
 *   parse(A && B)
 *   parse A && B
 * are equivalent.
 * ============================================================ */

typedef struct {
    bool quit;
    arena_t *arena;
} repl_ctx_t;

static void report_error(const char *src, span_t span, const char *msg) {
    fprintf(stderr, "error: %s (line %zu, col %zu)\n", msg, span.line, span.col);
    /* find line bounds */
    size_t line_start = 0;
    for (size_t i = 0; i < span.start && src[i]; i++)
        if (src[i] == '\n') line_start = i + 1;
    size_t line_end = line_start;
    while (src[line_end] && src[line_end] != '\n') line_end++;
    fprintf(stderr, "  %.*s\n  ", (int)(line_end - line_start), src + line_start);
    for (size_t i = line_start; i < span.start; i++) fputc(' ', stderr);
    size_t span_len = span.end > span.start ? span.end - span.start : 1;
    for (size_t i = 0; i < span_len && i < 80; i++) fputc('^', stderr);
    fputc('\n', stderr);
}

/* shared parse helper */
static node_t *try_parse(repl_ctx_t *ctx, const char *src) {
    parse_result_t r = parse(src, ctx->arena);
    if (!r.ok) { report_error(src, r.err_span, r.err_msg); return NULL; }
    return r.ast;
}

/* ----- commands ------------------------------------------------ */

static int cmd_parse(repl_ctx_t *ctx, const char *arg) {
    node_t *n = try_parse(ctx, arg);
    if (!n) return 0;
    print_expr(stdout, n);
    putchar('\n');
    return 0;
}

static int cmd_tree(repl_ctx_t *ctx, const char *arg) {
    node_t *n = try_parse(ctx, arg);
    if (!n) return 0;
    print_expr_indent(stdout, n);
    return 0;
}

static int cmd_eval(repl_ctx_t *ctx, const char *arg) {
    node_t *n = try_parse(ctx, arg);
    if (!n) return 0;
    pl_value_t v = pl_eval(n, NULL, 0);
    printf("%s\n", pl_value_name(v));
    return 0;
}

static int cmd_cnf(repl_ctx_t *ctx, const char *arg) {
    node_t *n = try_parse(ctx, arg);
    if (!n) return 0;
    node_t *c = transform_cnf(ctx->arena, n);
    print_expr(stdout, c);
    putchar('\n');
    return 0;
}

static int cmd_beta(repl_ctx_t *ctx, const char *arg) {
    node_t *n = try_parse(ctx, arg);
    if (!n) return 0;
    node_t *r = transform_beta_normal(ctx->arena, n);
    print_expr(stdout, r);
    putchar('\n');
    return 0;
}

static int cmd_tokens(repl_ctx_t *ctx, const char *arg) {
    lexer_t l;
    lex_init(&l, arg, ctx->arena);
    for (;;) {
        token_t t = lex_next(&l);
        if (t.kind == TOK_EOF) { printf("EOF\n"); break; }
        if (t.kind == TOK_ERROR) {
            printf("ERROR at %zu:%zu: %s\n", t.span.line, t.span.col, t.err_msg);
            break;
        }
        const char *kind = "?";
        switch (t.kind) {
            case TOK_LPAREN:    kind = "LPAREN";    break;
            case TOK_RPAREN:    kind = "RPAREN";    break;
            case TOK_COMMA:     kind = "COMMA";     break;
            case TOK_DOT:       kind = "DOT";       break;
            case TOK_COLON:     kind = "COLON";     break;
            case TOK_ARROW_T:   kind = "ARROW_T";   break;
            case TOK_BACKSLASH: kind = "BACKSLASH"; break;
            case TOK_OP:        kind = "OP";        break;
            case TOK_IDENT:     kind = "IDENT";     break;
            case TOK_NUMBER:    kind = "NUMBER";    break;
            default: break;
        }
        printf("%-10s '%s'  @ %zu:%zu\n", kind, t.lexeme, t.span.line, t.span.col);
    }
    return 0;
}

static int cmd_help(repl_ctx_t *ctx, const char *arg) {
    (void)ctx; (void)arg;
    puts(
        "commands:\n"
        "  parse <expr>    parse and pretty-print\n"
        "  tree  <expr>    parse and show as indented tree\n"
        "  tokens <expr>   tokenise only (debug)\n"
        "  eval  <expr>    three-valued PL evaluation\n"
        "  cnf   <expr>    convert to conjunctive normal form\n"
        "  beta  <expr>    beta-normalise (HOL)\n"
        "  help            this text\n"
        "  quit / exit     leave\n"
        "\n"
        "syntax:\n"
        "  PL:    !A && B || C => D <=> E\n"
        "  FOL:   forall x. P(x) => exists y. Q(x, y)\n"
        "         forall x y z. P(x, y, z)               # multi-var sugar\n"
        "  HOL:   \\x. f(x)        or    lambda x. f(x)\n"
        "         forall P:Nat -> Bool. P(0) || !P(0)\n"
        "  comments start with #\n");
    return 0;
}

static int cmd_quit(repl_ctx_t *ctx, const char *arg) {
    (void)arg;
    ctx->quit = true;
    return 0;
}

typedef int (*cmd_fn)(repl_ctx_t *, const char *);
typedef struct { const char *name; cmd_fn fn; } cmd_t;

static const cmd_t COMMANDS[] = {
    {"parse",  cmd_parse},
    {"tree",   cmd_tree},
    {"tokens", cmd_tokens},
    {"eval",   cmd_eval},
    {"cnf",    cmd_cnf},
    {"beta",   cmd_beta},
    {"help",   cmd_help},
    {"quit",   cmd_quit},
    {"exit",   cmd_quit},
    {NULL, NULL}
};

/* Strip leading whitespace, read an identifier as the command, return rest. */
static const char *split_cmd(const char *line, char *buf, size_t bufsz) {
    while (*line && isspace((unsigned char)*line)) line++;
    size_t i = 0;
    while (*line && (isalnum((unsigned char)*line) || *line == '_') && i + 1 < bufsz)
        buf[i++] = *line++;
    buf[i] = '\0';
    while (*line && isspace((unsigned char)*line)) line++;
    return line;
}

static int dispatch(repl_ctx_t *ctx, const char *line) {
    char cmd[32];
    const char *rest = split_cmd(line, cmd, sizeof cmd);
    if (cmd[0] == '\0') return 0; /* blank line */

    for (const cmd_t *c = COMMANDS; c->name; c++) {
        if (strcmp(c->name, cmd) == 0) return c->fn(ctx, rest);
    }
    fprintf(stderr, "unknown command: '%s'   (try 'help')\n", cmd);
    return 0;
}

int repl_run(arena_t *arena) {
    repl_ctx_t ctx = { .quit = false, .arena = arena };
    char *line = NULL;
    size_t cap = 0;
    puts("tpcas — type 'help' for commands, 'quit' to leave.");
    while (!ctx.quit) {
        fputs("> ", stdout);
        fflush(stdout);
        ssize_t n = getline(&line, &cap, stdin);
        if (n < 0) { putchar('\n'); break; }
        if (n > 0 && line[n - 1] == '\n') line[n - 1] = '\0';
        dispatch(&ctx, line);
        /* Reset arena per command to keep memory bounded.
         * (Trade-off: any AST built earlier becomes invalid; the REPL
         * doesn't currently let you reference past results, so this is
         * safe and keeps the long-running session lightweight.) */
        arena_reset(arena);
    }
    free(line);
    return 0;
}
