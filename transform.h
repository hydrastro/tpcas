#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "ast.h"
#include "arena.h"

/* ============================================================
 * Each rewrite is a node-to-node function. The CNF pipeline is
 * the staged composition you had — but each stage is now a pure
 * rewrite, driven by ast_rewrite_fixpoint so we get termination
 * detection for free.
 * ============================================================ */

/* A <=> B   ⇒  (A => B) && (B => A) */
node_t *transform_eliminate_iff(arena_t *a, node_t *n);

/* A => B    ⇒  !A || B */
node_t *transform_eliminate_imp(arena_t *a, node_t *n);

/* !!A => A, !(A && B) => !A || !B, !(A || B) => !A && !B,
 * !forall x. P => exists x. !P, !exists x. P => forall x. !P */
node_t *transform_push_not(arena_t *a, node_t *n);

/* A || (B && C) ⇒ (A || B) && (A || C), and symmetric */
node_t *transform_distribute_or(arena_t *a, node_t *n);

/* Full CNF: iff → imp → push-not → distribute, each to fixpoint. */
node_t *transform_cnf(arena_t *a, node_t *n);

/* Capture-avoiding β-reduction:  (\x. body) arg  ⇒  body[x := arg] */
node_t *transform_beta(arena_t *a, node_t *n);
node_t *transform_beta_normal(arena_t *a, node_t *n);  /* fixpoint */

/* Capture-avoiding substitution: body[var := repl] */
node_t *subst(arena_t *a, node_t *body, const char *var, node_t *repl);

#endif
