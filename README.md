# tpcas — parser combinators in C, side-by-side with Pratt

This is a small, focused parser-combinator library in C, used to
re-parse the same `tpcas` grammar that tpcas2 parses with a Pratt
parser. Both parsers produce the same AST (alpha-comparable via
`ast_equal`), so we can directly compare them on the same inputs.

## Build & run

```
make
./tpcas3                       # run the 39-case test suite
./tpcas3 "forall x. P(x)"      # parse one expression with both
                               # parsers and print both ASTs
```

## File map

```
arena.{c,h}     bump allocator                    (shared with tpcas2)
ast.{c,h}       AST nodes, op table, ast_equal    (shared with tpcas2)
print.{c,h}     pretty-printer                    (shared with tpcas2)
lex.{c,h}       lexer used by the Pratt parser    (shared with tpcas2)
pratt.{c,h}     the Pratt parser                  (copy of parse.{c,h}, renamed)
pc.{c,h}        the combinator library            NEW
combo.{c,h}     tpcas grammar built with combinators  NEW
main.c          test driver: ast_equal on both parsers
```

## What's in `pc.{c,h}` (the library)

A parser is a `parser_fn` + closure-state pair packed into a
`parser_t`. Parsers are arena-allocated. They run against a mutable
`pstate_t` and return a `presult_t` carrying `ok`, `consumed`, and a
`void*` semantic value. Failures don't go in the result — they're
recorded in the state as "expected X" at the furthest position any
parser reached. `pc_run` formats the accumulated expectations into a
human-readable message.

The combinator set is deliberately small — enough to build the
tpcas grammar without a lot of one-shot helpers:

**Primitives:** `pc_satisfy`, `pc_char`, `pc_string`, `pc_take_while1`,
`pc_eof`, `pc_pure`, `pc_fail`.

**Combinators:** `pc_alt` / `pc_alts`, `pc_try`, `pc_label`,
`pc_seq_l` / `pc_seq_r` / `pc_pair`, `pc_many` / `pc_many1`,
`pc_sepby1`, `pc_between`, `pc_opt_or`, `pc_with_span`, `pc_map`.

**Expression chains:** `pc_chainl1`, `pc_chainr1`, `pc_chain_none`.
These are the heart of the precedence story — see below.

**Recursion:** `pc_ref` returns a placeholder parser; `pc_set` fills
it in later. Needed because `expr` recurses through `paren`, `binder`,
`app`, and `not`.

**For writing custom primitives:** `pc_note_err`, `pc_advance`,
`pc_snapshot` / `pc_restore_err`. `combo.c` uses these to implement
identifier-with-keyword-filter and the operator parsers without
tokenizing first.

### Design choices worth flagging

- **Parsec-style commit semantics.** `pc_alt(p1, p2)` only tries `p2`
  if `p1` failed *without consuming input*. To make a parser
  backtrackable, wrap it in `pc_try`. This avoids exponential
  backtracking and gives the standard "furthest reach" error model.
- **`pc_string` is atomic.** It never partially consumes — fails or
  matches the whole literal. Saves a `pc_try` at every use site.
- **Error accumulation.** At the furthest position reached, we keep
  every "expected X" that was noted there. `pc_many` and `pc_opt_or`
  snapshot/restore error state so their body's failures (e.g. the
  whitespace skip running out of whitespace) don't pollute the error.
- **`void *` everywhere.** Each parser produces an opaque value; the
  caller knows what each parser produces and casts in semantic
  actions. Type safety is by convention.

## The combinator grammar, level by level (`combo.c`)

The expression grammar reads top-down as one combinator per
precedence level, each wrapping the next:

```c
parser_t *and_expr = pc_chainl1   (a, not_expr, s_and, combine_binop, ctx_AND);
parser_t *or_expr  = pc_chainl1   (a, and_expr, s_or,  combine_binop, ctx_OR );
parser_t *eq_expr  = pc_chain_none(a, or_expr,  s_eq,  combine_binop, ctx_EQ );
parser_t *imp_expr = pc_chainr1   (a, eq_expr,  s_imp, combine_binop, ctx_IMP);
parser_t *iff_expr = pc_chain_none(a, imp_expr, s_iff, combine_binop, ctx_IFF);
```

Direction of parsing reads off the wrapping:
- `chainl1` ↔ left-associative
- `chainr1` ↔ right-associative
- `chain_none` ↔ non-associative (rejects `A op B op C`)

The atom layer is one `pc_alts`. Function application is a postfix
`atom (args)?` parsed via `pc_pair(atom, pc_opt_or(call_tail, NULL))`
with a `map_apply` that builds an `ast_app` when args were present.
Prefix `!` is a self-referential `pc_alt(map(seq_r(!, self), wrap), app)`.

## Pratt ↔ combinators, side by side

|                        | Pratt parser                  | Combinators                                   |
|------------------------|-------------------------------|-----------------------------------------------|
| code shape             | single loop, table-driven     | one parser per precedence level, stacked      |
| precedence             | `rbp` argument                | encoded by wrapping order                     |
| left-assoc             | `next_rbp = prec`             | `chainl1`                                     |
| right-assoc            | `next_rbp = prec - 1`         | `chainr1`                                     |
| non-assoc              | `next_rbp = prec + 1`         | `chain_none`                                  |
| function application   | postfix `(` at BP_APPLY=100   | `pair(atom, opt(call_tail))` + `map_apply`    |
| operator dispatch      | lookup in `op_info_t` table   | one combinator call per op, op metadata in ctx|
| forward references     | direct recursion              | `pc_ref` / `pc_set`                           |
| error reporting        | one-shot from current token   | furthest-position accumulation                |

### What the Pratt parser does better

- **One pass over the input.** Combinators visit each character
  multiple times (through layers of `chainl1`/`chainr1` that all
  match and discard).
- **No prefix-conflict pain.** Pratt tokenizes first with longest
  match, so `=` vs `=>` is settled before parsing. Combinators
  without a tokenizer need negative-lookahead helpers (see `p_punct`
  in `combo.c` for `"=", forbid_next=">"`).
- **More transparent control flow.** One function (`parse_expr`)
  with a precedence parameter; you can step it in a debugger and
  see exactly where you are.

### What the combinator parser does better

- **Grammar reads top-down.** Each level says what it is in one line.
  Adding a new operator level is one line plus a context struct.
- **Error messages list all alternatives.** "expected `!`, `true`,
  `false`, …, or `(`" rather than "expected an expression".
- **No tokenizer.** One layer instead of two; lexical rules are
  expressed in the same vocabulary as syntactic ones.
- **Composes.** `pc_chainl1`, `pc_sepby1`, `pc_between` etc. are
  reusable for any future grammar; the table-driven Pratt loop is
  bound to its op-info shape.

## The test harness (`main.c`)

39 inputs covering propositional core, FOL with multi-variable
binders, HOL with types, lambdas, and literals. For each one:

1. Parse with `parse(src, &arena)` (Pratt).
2. Parse with `combo_parse(src, &arena)` (combinators).
3. Compare ASTs with `ast_equal`.

Pass = both parsers succeed and the ASTs match. Run `./tpcas3
"YOUR EXPR"` to inspect one input with both parsers' output.

## Known limitations (deliberately left out)

- No error recovery — first failure halts parsing.
- No incremental / streaming parser; consumes the whole string.
- `pc_string` is implicitly atomic (re-checks from start on each
  call). Fine for short operator literals; not great for long ones.
- Error message merging at the same position is by union, not
  Parsec-style merge with separate "consumed" and "unconsumed"
  error tracking. The single-position model is enough for this
  grammar.
