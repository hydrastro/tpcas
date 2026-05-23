# tpcas2 — propositional / first-order / higher-order logic CAS

A rewrite of `tpcas` keeping the design ideas (table-driven operators,
direction-of-parsing, callback-traced rewrites) but on a sturdier
foundation: arena allocator, Pratt parser, unified AST.

## Build

```
make
./tpcas
```

## Syntax

```
PL    !A && B || C => D <=> E       # standard connectives
FOL   forall x. P(x)                # quantifier
      exists y. Q(x, y)
      forall x y z. P(x, y, z)      # multi-var sugar
      forall x:Nat. P(x)            # typed
HOL   \x. f(x)        lambda x. f(x)
      forall P:Nat -> Bool. P(0) || !P(0)
```

Precedence (highest binds tightest):

| operator | prec | assoc |
|----------|------|-------|
| `!`      | 90   | prefix |
| `&&`     | 70   | left  |
| `\|\|`     | 60   | left  |
| `=`      | 50   | none  |
| `=>`     | 40   | right |
| `<=>`    | 30   | none  |

Application `f(...)` has effective precedence 100. Binders extend as
far right as possible.

## Commands

```
parse <expr>     parse and pretty-print
tree  <expr>     show as indented tree
tokens <expr>    debug: tokenise only
eval  <expr>     three-valued PL evaluation
cnf   <expr>     convert to conjunctive normal form
beta  <expr>     beta-normalise (HOL)
help / quit
```

## Examples

```
> cnf (A => B) && (B => C)
(!A || B) && (!B || C)

> cnf !(forall x. P(x))
exists x. !P(x)

> beta (\f. \x. f(f(x)))(g)(a)
g(g(a))

> parse forall x:Nat. forall P:Nat -> Bool. P(x) || !P(x)
forall x:Nat. forall P:Nat -> Bool. P(x) || !P(x)
```

## Files

```
arena.{h,c}      bump allocator
ast.{h,c}        unified AST + operator table + alpha-eq + rewriters
lex.{h,c}        position-tracked tokenizer
parse.{h,c}      Pratt parser
print.{h,c}      precedence-aware pretty-printer (round-trips)
eval.{h,c}       three-valued PL evaluation
transform.{h,c}  CNF + capture-avoiding beta reduction
repl.{h,c}       REPL with caret diagnostics
main.c
```

## Design notes

**Pratt parser ≈ "direction-of-parsing" in one pass.** The original
tpcas iterated operators in precedence order and walked the flat
token list left-to-right or right-to-left based on associativity.
A Pratt parser does the same in one descent: left-associative
operators recurse with `rbp = precedence`, right-associative with
`rbp = precedence - 1`. Same idea, no list mutation.

**Function application is a postfix `(`** with binding power 100.
This is what makes `P(x, y)` (FOL predicates) and `f(x)` (HOL
application) fall out for free — the original tpcas couldn't
express juxtaposition.

**Rewrites are pure functions** `Node -> Maybe Node`. The visitor
pattern with callbacks from the original is preserved, but driven
by `ast_rewrite_fixpoint` so termination detection is automatic
(returns the same pointer when no rewrite fires).

**Capture-avoiding substitution**: when substituting under a binder
whose bound variable is free in the replacement, the binder is
alpha-renamed to a fresh name (`x_N`).

## Not done yet

- No type checker. Types parse and print but aren't enforced.
- No de Bruijn indices. Substitution uses names; capture avoidance
  is correct but freshness counter is global (not a problem for a
  REPL session).
- CNF distribution doesn't handle nested quantifiers smartly
  (no Skolemisation or prenex form).
- No proof / theorem-prover module yet.
