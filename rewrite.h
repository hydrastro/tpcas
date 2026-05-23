#ifndef REWRITE_H
#define REWRITE_H

#include "ast.h"

/*
 * Generic rewrite framework.
 *
 * A rule takes a node and returns either NULL (rule didn't fire) or a new node
 * (rule fired -- the returned node replaces the input). The framework handles
 * traversal and fixpoint iteration. Specific rules like De Morgan, →-elim,
 * and ∨-over-∧ distribution are just predicates over node shapes.
 *
 * This replaces the original tpcas's hand-rolled recursive mutation in
 * pl_demorgan / pl_conjunct_disjunct / pl_to_boolean_basis. Those routines
 * had to redo traversal logic from scratch, mutated nodes in place (causing
 * double-frees), and lacked fixpoint detection. The new framework lets the
 * rules be pure: node -> maybe node.
 */

typedef ast_t *(*rule_t)(arena_t *a, ast_t *e);

/* Optional tracing callback. */
typedef void (*step_cb_t)(ast_t *before, ast_t *after, void *ud);

/* Bottom-up single pass: rewrite each subtree exactly once (children first),
 * then try the rule at the current node. Allocates fresh nodes when changed. */
ast_t *rewrite_bottomup(arena_t *a, ast_t *e, rule_t rule);

/* Apply rule bottom-up until fixpoint (no further change) or max_iters
 * iterations. Optionally calls cb after each whole-tree iteration. */
ast_t *rewrite_fix(arena_t *a, ast_t *e, rule_t rule,
                   step_cb_t cb, void *ud, int max_iters);

/* Compose: apply rules[0] to fixpoint, then rules[1], etc. */
ast_t *rewrite_pipeline(arena_t *a, ast_t *e, rule_t *rules, int nrules,
                        step_cb_t cb, void *ud, int max_iters);

/* ===== concrete propositional-logic rules ===== */

/* !!A          --> A
 * !(A && B)    --> (!A) || (!B)
 * !(A || B)    --> (!A) && (!B) */
ast_t *rule_demorgan(arena_t *a, ast_t *e);

/* A => B       --> (!A) || B */
ast_t *rule_implies_to_or(arena_t *a, ast_t *e);

/* A <=> B      --> (A => B) && (B => A) */
ast_t *rule_iff_expand(arena_t *a, ast_t *e);

/* A || (B && C) --> (A || B) && (A || C)
 * (A && B) || C --> (A || C) && (B || C) */
ast_t *rule_distribute_or_over_and(arena_t *a, ast_t *e);

/* ===== HOL: β-reduction (capture-avoiding skipped; see note in rewrite.c) ===== */

/* (\x. body)(arg)  --> body[x := arg]   (naive substitution) */
ast_t *rule_beta(arena_t *a, ast_t *e);

/* Compute the full CNF of a propositional formula. */
ast_t *to_cnf(arena_t *a, ast_t *e, step_cb_t cb, void *ud);

#endif
