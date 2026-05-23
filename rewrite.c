#include "rewrite.h"
#include <string.h>
#include <stdio.h>

/* ===== generic traversal ===== */

ast_t *rewrite_bottomup(arena_t *a, ast_t *e, rule_t rule) {
    if (!e) return NULL;
    ast_t *out = e;
    switch (e->kind) {
    case AST_VAR:
    case AST_CONST:
        break;
    case AST_OP: {
        bool changed = false;
        ast_t **new_args = arena_alloc(a, sizeof(ast_t *) * (e->u.op.nargs ? e->u.op.nargs : 1));
        for (int i = 0; i < e->u.op.nargs; i++) {
            new_args[i] = rewrite_bottomup(a, e->u.op.args[i], rule);
            if (new_args[i] != e->u.op.args[i]) changed = true;
        }
        if (changed) out = ast_op(a, e->pos, e->u.op.op, new_args, e->u.op.nargs);
        break;
    }
    case AST_APP: {
        bool changed = false;
        ast_t *new_head = rewrite_bottomup(a, e->u.app.head, rule);
        if (new_head != e->u.app.head) changed = true;
        ast_t **new_args = arena_alloc(a, sizeof(ast_t *) * (e->u.app.nargs ? e->u.app.nargs : 1));
        for (int i = 0; i < e->u.app.nargs; i++) {
            new_args[i] = rewrite_bottomup(a, e->u.app.args[i], rule);
            if (new_args[i] != e->u.app.args[i]) changed = true;
        }
        if (changed) out = ast_app(a, e->pos, new_head, new_args, e->u.app.nargs);
        break;
    }
    case AST_LAMBDA:
    case AST_FORALL:
    case AST_EXISTS: {
        ast_t *new_body = rewrite_bottomup(a, e->u.binder.body, rule);
        if (new_body != e->u.binder.body) {
            out = ast_binder(a, e->pos, e->kind, e->u.binder.bv,
                             e->u.binder.bv_type, new_body);
        }
        break;
    }
    }

    /* now apply rule at this (possibly rewritten) node */
    ast_t *fired = rule(a, out);
    if (fired) return fired;
    return out;
}

ast_t *rewrite_fix(arena_t *a, ast_t *e, rule_t rule,
                   step_cb_t cb, void *ud, int max_iters) {
    if (max_iters <= 0) max_iters = 100;
    ast_t *prev = e;
    for (int i = 0; i < max_iters; i++) {
        ast_t *next = rewrite_bottomup(a, prev, rule);
        if (ast_eq(next, prev)) return next;
        if (cb) cb(prev, next, ud);
        prev = next;
    }
    return prev;
}

ast_t *rewrite_pipeline(arena_t *a, ast_t *e, rule_t *rules, int nrules,
                        step_cb_t cb, void *ud, int max_iters) {
    ast_t *cur = e;
    for (int i = 0; i < nrules; i++) {
        cur = rewrite_fix(a, cur, rules[i], cb, ud, max_iters);
    }
    return cur;
}

/* ===== shape helpers ===== */

static bool is_op(ast_t *e, op_id_t id) {
    return e && e->kind == AST_OP && e->u.op.op->id == id;
}

static ast_t *mk_not(arena_t *a, ast_t *x) {
    ast_t *args[1] = { x };
    return ast_op(a, x->pos, op_lookup_by_id(OP_NOT), args, 1);
}

static ast_t *mk_bin(arena_t *a, op_id_t id, ast_t *l, ast_t *r) {
    ast_t *args[2] = { l, r };
    return ast_op(a, l->pos, op_lookup_by_id(id), args, 2);
}

/* ===== concrete rules ===== */

ast_t *rule_demorgan(arena_t *a, ast_t *e) {
    if (!is_op(e, OP_NOT)) return NULL;
    ast_t *x = e->u.op.args[0];
    /* !!A -> A */
    if (is_op(x, OP_NOT)) return x->u.op.args[0];
    /* !(A && B) -> (!A) || (!B) */
    if (is_op(x, OP_AND)) {
        return mk_bin(a, OP_OR,
                      mk_not(a, x->u.op.args[0]),
                      mk_not(a, x->u.op.args[1]));
    }
    /* !(A || B) -> (!A) && (!B) */
    if (is_op(x, OP_OR)) {
        return mk_bin(a, OP_AND,
                      mk_not(a, x->u.op.args[0]),
                      mk_not(a, x->u.op.args[1]));
    }
    return NULL;
}

ast_t *rule_implies_to_or(arena_t *a, ast_t *e) {
    if (!is_op(e, OP_IMPLIES)) return NULL;
    return mk_bin(a, OP_OR, mk_not(a, e->u.op.args[0]), e->u.op.args[1]);
}

ast_t *rule_iff_expand(arena_t *a, ast_t *e) {
    if (!is_op(e, OP_IFF)) return NULL;
    ast_t *l = e->u.op.args[0];
    ast_t *r = e->u.op.args[1];
    /* A <=> B  ===  (A => B) && (B => A) */
    return mk_bin(a, OP_AND,
                  mk_bin(a, OP_IMPLIES, ast_dup(a, l), ast_dup(a, r)),
                  mk_bin(a, OP_IMPLIES, ast_dup(a, r), ast_dup(a, l)));
}

ast_t *rule_distribute_or_over_and(arena_t *a, ast_t *e) {
    if (!is_op(e, OP_OR)) return NULL;
    ast_t *l = e->u.op.args[0];
    ast_t *r = e->u.op.args[1];
    /* A || (B && C) -> (A || B) && (A || C) */
    if (is_op(r, OP_AND)) {
        return mk_bin(a, OP_AND,
                      mk_bin(a, OP_OR, ast_dup(a, l), r->u.op.args[0]),
                      mk_bin(a, OP_OR, ast_dup(a, l), r->u.op.args[1]));
    }
    /* (A && B) || C -> (A || C) && (B || C) */
    if (is_op(l, OP_AND)) {
        return mk_bin(a, OP_AND,
                      mk_bin(a, OP_OR, l->u.op.args[0], ast_dup(a, r)),
                      mk_bin(a, OP_OR, l->u.op.args[1], ast_dup(a, r)));
    }
    return NULL;
}

ast_t *to_cnf(arena_t *a, ast_t *e, step_cb_t cb, void *ud) {
    rule_t rules[] = {
        rule_iff_expand,        /* eliminate <=> */
        rule_implies_to_or,     /* eliminate => */
        rule_demorgan,          /* push ! inward */
        rule_distribute_or_over_and, /* distribute */
    };
    return rewrite_pipeline(a, e, rules, sizeof(rules)/sizeof(*rules), cb, ud, 200);
}

/* ===== HOL β-reduction (naive: no capture avoidance) =====
 *
 * NOTE: This is intentionally naive. Real β-reduction must α-rename bound
 * variables to avoid capture, e.g.  (\x. \y. x) y  must rename the inner y.
 * For closed terms this is fine; for open terms with capture potential
 * the result is wrong. A proper implementation requires fresh-name
 * generation and alpha equivalence -- deferred.
 */

static ast_t *subst(arena_t *a, ast_t *e, const char *x, ast_t *val) {
    if (!e) return NULL;
    switch (e->kind) {
    case AST_VAR:
        if (strcmp(e->u.var.name, x) == 0) return ast_dup(a, val);
        return e;
    case AST_CONST:
        return e;
    case AST_OP: {
        ast_t **args = arena_alloc(a, sizeof(ast_t *) * (e->u.op.nargs ? e->u.op.nargs : 1));
        bool changed = false;
        for (int i = 0; i < e->u.op.nargs; i++) {
            args[i] = subst(a, e->u.op.args[i], x, val);
            if (args[i] != e->u.op.args[i]) changed = true;
        }
        return changed ? ast_op(a, e->pos, e->u.op.op, args, e->u.op.nargs) : e;
    }
    case AST_APP: {
        ast_t *nh = subst(a, e->u.app.head, x, val);
        ast_t **args = arena_alloc(a, sizeof(ast_t *) * (e->u.app.nargs ? e->u.app.nargs : 1));
        bool changed = (nh != e->u.app.head);
        for (int i = 0; i < e->u.app.nargs; i++) {
            args[i] = subst(a, e->u.app.args[i], x, val);
            if (args[i] != e->u.app.args[i]) changed = true;
        }
        return changed ? ast_app(a, e->pos, nh, args, e->u.app.nargs) : e;
    }
    case AST_LAMBDA:
    case AST_FORALL:
    case AST_EXISTS:
        /* Stop substituting inside a binder whose bound variable shadows x */
        if (strcmp(e->u.binder.bv, x) == 0) return e;
        {
            ast_t *nb = subst(a, e->u.binder.body, x, val);
            if (nb == e->u.binder.body) return e;
            return ast_binder(a, e->pos, e->kind, e->u.binder.bv,
                              e->u.binder.bv_type, nb);
        }
    }
    return e;
}

ast_t *rule_beta(arena_t *a, ast_t *e) {
    if (!e || e->kind != AST_APP) return NULL;
    if (e->u.app.head->kind != AST_LAMBDA) return NULL;
    if (e->u.app.nargs == 0) return NULL;
    /* β: (\x. body)(arg, rest...) -> body[x := arg](rest...) */
    ast_t *lam = e->u.app.head;
    ast_t *arg = e->u.app.args[0];
    ast_t *reduced = subst(a, lam->u.binder.body, lam->u.binder.bv, arg);
    if (e->u.app.nargs == 1) return reduced;
    /* curry: still have args left */
    int rest = e->u.app.nargs - 1;
    ast_t **new_args = arena_alloc(a, sizeof(ast_t *) * rest);
    for (int i = 0; i < rest; i++) new_args[i] = e->u.app.args[i + 1];
    return ast_app(a, e->pos, reduced, new_args, rest);
}
