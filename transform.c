#include "transform.h"
#include <stdio.h>
#include <string.h>

/* ============================================================
 * Small helpers: build common nodes.  Spans are inherited from
 * the input — good enough for error messages on rewritten terms.
 * ============================================================ */

static node_t *mk_op_const(arena_t *a, const op_info_t *op, span_t s) {
    return ast_const(a, op->name, op, s);
}
static node_t *mk_not(arena_t *a, node_t *x, span_t s) {
    return ast_app1(a, mk_op_const(a, &OP_NOT, s), x, s);
}
static node_t *mk_and(arena_t *a, node_t *x, node_t *y, span_t s) {
    return ast_app2(a, mk_op_const(a, &OP_AND, s), x, y, s);
}
static node_t *mk_or(arena_t *a, node_t *x, node_t *y, span_t s) {
    return ast_app2(a, mk_op_const(a, &OP_OR, s), x, y, s);
}
static node_t *mk_imp(arena_t *a, node_t *x, node_t *y, span_t s) {
    return ast_app2(a, mk_op_const(a, &OP_IMP, s), x, y, s);
}

/* ============================================================ */
/* CNF rewrites                                                  */
/* ============================================================ */

/* A <=> B   ⇒   (A => B) && (B => A) */
static node_t *rw_iff(arena_t *a, node_t *n, void *ctx) {
    (void)ctx;
    if (!ast_is_op_app(n, &OP_IFF, 2)) return NULL;
    node_t *A = n->app.args[0], *B = n->app.args[1];
    return mk_and(a, mk_imp(a, A, B, n->span), mk_imp(a, B, A, n->span), n->span);
}

/* A => B   ⇒   !A || B */
static node_t *rw_imp(arena_t *a, node_t *n, void *ctx) {
    (void)ctx;
    if (!ast_is_op_app(n, &OP_IMP, 2)) return NULL;
    node_t *A = n->app.args[0], *B = n->app.args[1];
    return mk_or(a, mk_not(a, A, n->span), B, n->span);
}

/* Negation normalisation rules */
static node_t *rw_push_not(arena_t *a, node_t *n, void *ctx) {
    (void)ctx;
    if (!ast_is_op_app(n, &OP_NOT, 1)) return NULL;
    node_t *inner = n->app.args[0];

    /* !!A => A */
    if (ast_is_op_app(inner, &OP_NOT, 1)) return inner->app.args[0];

    /* !(A && B) => !A || !B   ;   !(A || B) => !A && !B */
    if (ast_is_op_app(inner, &OP_AND, 2)) {
        return mk_or(a,
                     mk_not(a, inner->app.args[0], inner->span),
                     mk_not(a, inner->app.args[1], inner->span),
                     n->span);
    }
    if (ast_is_op_app(inner, &OP_OR, 2)) {
        return mk_and(a,
                      mk_not(a, inner->app.args[0], inner->span),
                      mk_not(a, inner->app.args[1], inner->span),
                      n->span);
    }

    /* !forall x. P => exists x. !P   ;   !exists x. P => forall x. !P */
    if (inner->kind == NODE_FORALL || inner->kind == NODE_EXISTS) {
        node_kind_t dual = (inner->kind == NODE_FORALL) ? NODE_EXISTS : NODE_FORALL;
        node_t *negated = mk_not(a, inner->bind.body, inner->bind.body->span);
        return ast_binder(a, dual, inner->bind.bvar, inner->bind.btype, negated, n->span);
    }
    return NULL;
}

/* A || (B && C)   ⇒   (A || B) && (A || C)
 * (A && B) || C   ⇒   (A || C) && (B || C)
 */
static node_t *rw_distribute_or(arena_t *a, node_t *n, void *ctx) {
    (void)ctx;
    if (!ast_is_op_app(n, &OP_OR, 2)) return NULL;
    node_t *L = n->app.args[0], *R = n->app.args[1];

    if (ast_is_op_app(R, &OP_AND, 2)) {
        node_t *B = R->app.args[0], *C = R->app.args[1];
        return mk_and(a, mk_or(a, L, B, n->span), mk_or(a, L, C, n->span), n->span);
    }
    if (ast_is_op_app(L, &OP_AND, 2)) {
        node_t *A2 = L->app.args[0], *B = L->app.args[1];
        return mk_and(a, mk_or(a, A2, R, n->span), mk_or(a, B, R, n->span), n->span);
    }
    return NULL;
}

node_t *transform_eliminate_iff(arena_t *a, node_t *n) {
    return ast_rewrite_fixpoint(a, n, rw_iff, NULL, 100);
}
node_t *transform_eliminate_imp(arena_t *a, node_t *n) {
    return ast_rewrite_fixpoint(a, n, rw_imp, NULL, 100);
}
node_t *transform_push_not(arena_t *a, node_t *n) {
    return ast_rewrite_fixpoint(a, n, rw_push_not, NULL, 100);
}
node_t *transform_distribute_or(arena_t *a, node_t *n) {
    return ast_rewrite_fixpoint(a, n, rw_distribute_or, NULL, 100);
}

node_t *transform_cnf(arena_t *a, node_t *n) {
    n = transform_eliminate_iff(a, n);
    n = transform_eliminate_imp(a, n);
    n = transform_push_not(a, n);
    n = transform_distribute_or(a, n);
    return n;
}

/* ============================================================ */
/* Substitution and β-reduction (capture-avoiding)               */
/*                                                              */
/* When substituting body[x := repl], at a binder \y. e:        */
/*   - if y == x:  the bound var shadows, return unchanged       */
/*   - if y is free in repl:  rename y → y_N to avoid capture    */
/*   - otherwise: recurse into e                                 */
/* ============================================================ */

static unsigned long g_fresh = 0;

static const char *fresh_name(arena_t *a, const char *base) {
    char buf[64];
    snprintf(buf, sizeof buf, "%s_%lu", base, ++g_fresh);
    return arena_strdup(a, buf);
}

node_t *subst(arena_t *a, node_t *body, const char *var, node_t *repl) {
    if (!body) return NULL;
    switch (body->kind) {
        case NODE_CONST: return body;
        case NODE_VAR:
            return strcmp(body->var.name, var) == 0 ? repl : body;
        case NODE_APP: {
            node_t *nh = subst(a, body->app.head, var, repl);
            bool changed = nh != body->app.head;
            node_t **na = NULL;
            if (body->app.argc) {
                na = arena_alloc(a, sizeof(node_t *) * body->app.argc);
                for (size_t i = 0; i < body->app.argc; i++) {
                    na[i] = subst(a, body->app.args[i], var, repl);
                    if (na[i] != body->app.args[i]) changed = true;
                }
            }
            if (!changed) return body;
            return ast_app(a, nh, body->app.argc, na, body->span);
        }
        case NODE_LAMBDA:
        case NODE_FORALL:
        case NODE_EXISTS: {
            if (strcmp(body->bind.bvar, var) == 0) return body; /* shadowed */
            if (ast_free_in(repl, body->bind.bvar)) {
                /* capture: alpha-rename the binder before substituting */
                const char *fresh = fresh_name(a, body->bind.bvar);
                node_t *fvar = ast_var(a, fresh, body->span);
                node_t *renamed = subst(a, body->bind.body, body->bind.bvar, fvar);
                node_t *done = subst(a, renamed, var, repl);
                return ast_binder(a, body->kind, fresh, body->bind.btype, done, body->span);
            }
            node_t *nb = subst(a, body->bind.body, var, repl);
            if (nb == body->bind.body) return body;
            return ast_binder(a, body->kind, body->bind.bvar, body->bind.btype, nb, body->span);
        }
    }
    return body;
}

/* (\x. body) arg1 arg2 ...    ⇒    body[x := arg1] arg2 ...
 *
 * For multi-arg applications we only consume the first arg per beta
 * step; the rewriter will revisit and consume more in the next pass.
 */
static node_t *rw_beta(arena_t *a, node_t *n, void *ctx) {
    (void)ctx;
    if (n->kind != NODE_APP) return NULL;
    if (n->app.head->kind != NODE_LAMBDA) return NULL;
    if (n->app.argc < 1) return NULL;

    node_t *lam = n->app.head;
    node_t *arg = n->app.args[0];
    node_t *reduced = subst(a, lam->bind.body, lam->bind.bvar, arg);

    if (n->app.argc == 1) return reduced;
    /* leftover args become a fresh application head */
    size_t rest = n->app.argc - 1;
    node_t **na = arena_alloc(a, sizeof(node_t *) * rest);
    for (size_t i = 0; i < rest; i++) na[i] = n->app.args[i + 1];
    return ast_app(a, reduced, rest, na, n->span);
}

node_t *transform_beta(arena_t *a, node_t *n) {
    return ast_rewrite_bottomup(a, n, rw_beta, NULL);
}
node_t *transform_beta_normal(arena_t *a, node_t *n) {
    return ast_rewrite_fixpoint(a, n, rw_beta, NULL, 1000);
}
