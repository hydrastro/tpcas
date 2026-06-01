# tpcas

TPCAS is a small C parser-combinator implementation of a typed propositional / first-order / higher-order expression grammar. It keeps a Pratt parser beside the combinator parser and verifies that both parsers produce alpha-equivalent ASTs for the same inputs. The surface grammar also includes a compact arithmetic layer (`+`, `-`, `*`, `/`, decimal literals, and unary `-`) so other C programs can embed TPCAS as a normal infix expression parser instead of using Lisp/S-expression input.

## Layout

```text
.
├── flake.nix           # Nix flake; pulls in the ds dependency
├── Makefile
├── src/
│   ├── arena.{c,h}     # TPCAS arena facade backed by the ds allocator library
│   ├── ast.{c,h}       # AST, types, operators, alpha-equivalence, rewrites
│   ├── lex.{c,h}       # lexer used by the Pratt parser
│   ├── pratt.{c,h}     # Pratt parser baseline
│   ├── pc.{c,h}        # parser-combinator core
│   ├── combo.{c,h}     # grammar implemented with combinators
│   └── print.{c,h}     # expression/type printers
├── app/
│   └── tpcas.c         # CLI: built-in suite + single-expression driver
└── test/
    └── cpp_smoke.cpp   # C++ ABI/linkage smoke test
```

All generated files are emitted under `build/`.

## Data-structures integration

TPCAS depends on the external [ds](https://github.com/hydrastro/ds) library in two places:

1. `src/arena.c` preserves the existing `arena_t` API, but each arena chunk is backed by `ds_arena_t` and allocated through `ds_context_t`.
2. `src/pc.c` uses `ds_str_t` to build parser expectation messages instead of hand-rolling mutable string buffers.

ds is no longer vendored. It is consumed as a flake input (`github:hydrastro/ds`) and linked as a normal dependency.

## Build (Nix)

The flake provides `ds` to both the package build and the dev shell, exporting `DS_CFLAGS` and `DS_LIBS` so `make` finds its headers and library automatically.

```sh
nix develop          # drops you in a shell with ds, make, clang-tools, gdb, valgrind
make                 # build build/tpcas and build/libtpcas.a

nix build            # build the package
nix run . -- 'x * (28 - z) - y'
```

## Build (without Nix)

Point `make` at an installed ds. Headers are included as `lib/…` (e.g. `lib/str.h`), so the include directory must contain a `lib/` subdirectory:

```sh
make DS_CFLAGS=-I/usr/local/include DS_LIBS='-L/usr/local/lib -lds'
```

The executable is `build/tpcas`.

Useful variants:

```sh
make MODE=release
make MODE=asan
make CC=clang
```

## Test

```sh
make test
```

The built-in suite runs 39 expressions through both parsers and checks AST equality, including the arithmetic extension. An optional C++ linkage check is available via `make check-cpp`.

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
