# tpcas

TPCAS is a small C parser-combinator implementation of a typed propositional / first-order / higher-order expression grammar. It keeps a Pratt parser beside the combinator parser and verifies that both parsers produce alpha-equivalent ASTs for the same inputs. The surface grammar also includes a compact arithmetic layer (`+`, `-`, `*`, `/`, decimal literals, and unary `-`) so other C programs can embed TPCAS as a normal infix expression parser instead of using Lisp/S-expression input.

## Layout

```text
.
├── Makefile
├── scripts/
│   └── clean-tree.sh   # dry-run-first cleanup for the old flat tree
├── src/
│   ├── arena.{c,h}     # TPCAS arena facade backed by the DS allocator library
│   ├── ast.{c,h}       # AST, types, operators, alpha-equivalence, rewrites
│   ├── lex.{c,h}       # lexer used by the Pratt parser
│   ├── pratt.{c,h}     # Pratt parser baseline
│   ├── pc.{c,h}        # parser-combinator core
│   ├── combo.{c,h}     # grammar implemented with combinators
│   ├── print.{c,h}     # expression/type printers
│   └── main.c          # comparison test harness and single-expression driver
└── vendor/ds/
    └── lib/            # vendored subset of the custom data-structures library
```

All generated files are emitted under `build/`. The old flat source tree and root-level build products should be removed with the cleanup script below.

## Data-structures integration

TPCAS uses the custom DS library in two places:

1. `src/arena.c` preserves the existing `arena_t` API, but each arena chunk is backed by `ds_arena_t` and allocated through `ds_context_t`.
2. `src/pc.c` uses `ds_str_t` to build parser expectation messages instead of hand-rolling mutable string buffers.

Only the DS modules required by this project are vendored: `common`, `status`, `error`, `diagnostic`, `context`, `allocators`, and `str`.

## Build

```sh
make
```

The executable is `build/tpcas`.

Useful variants:

```sh
make MODE=release
make MODE=asan
make CC=clang
```

Build with Nix:

```sh
nix build
nix run . -- 'x * (28 - z) - y'
```

## Test

```sh
make test
```

The built-in suite runs 45 expressions through both parsers and checks AST equality, including the arithmetic extension.

## Parse one expression

```sh
make run ARGS='forall x:Nat. P(x) || !P(x)'
make run ARGS='x * (28 - z) - y'
# or:
./build/tpcas 'x * y - (8 / 3) * z'
```

## Clean generated files

```sh
make clean
```

This removes only `build/`.

## Clean up the old flat tree

Preview first:

```sh
scripts/clean-tree.sh
# or:
make prune-legacy
```

Apply once the preview looks right:

```sh
scripts/clean-tree.sh --apply
# or:
make prune-legacy CLEAN_APPLY=1
```

This removes known migration garbage such as root-level `*.o`, old `tpcas`/`tpcas3`/`test_rt` binaries, duplicated root-level `.c`/`.h` files, and exported archive/directories from earlier refactor passes. It preserves `src/`, `vendor/`, `README.md`, `CHANGES.md`, `Makefile`, and `scripts/`.
