# tpcas

TPCAS is a small C parser-combinator implementation of a typed propositional / first-order / higher-order expression grammar. It keeps a Pratt parser beside the combinator parser and verifies that both parsers produce alpha-equivalent ASTs for the same inputs.

> **Note:** a compact arithmetic layer (`+`, `-`, `*`, `/`, decimal literals, and unary `-`) is planned so other C programs can embed TPCAS as a normal infix expression parser, but it is **not yet implemented** in this tree. The lexer recognizes number tokens, but neither parser accepts the arithmetic operators yet, so inputs like `x * (28 - z)` currently fail. The examples below use the logical/quantifier grammar that is implemented.

## Layout

```text
.
├── Makefile
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

All generated files are emitted under `build/`.

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
nix run . -- 'forall x. P(x) && Q(x)'
```

## Test

```sh
make test
```

The built-in suite runs every fixture in `src/main.c` through both parsers and checks AST equality.

## Parse one expression

```sh
make run ARGS='forall x:Nat. P(x) || !P(x)'
make run ARGS='forall x. exists y. R(x, y)'
# or:
./build/tpcas 'A && B => C || D'
```

## Clean generated files

```sh
make clean
```

This removes only `build/`.
