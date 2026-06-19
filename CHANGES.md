## Arithmetic parser completion

- Added public `OP_ADD`, `OP_SUB`, `OP_MUL`, and `OP_DIV` operator metadata.
- Added arithmetic precedence and unary minus to both the Pratt and combinator parsers.
- Added decimal and scientific-notation literals to both lexing paths.
- Added arithmetic regression cases and C++ API/AST-shape checks.
- Added `make check`, ignored generated build output, and made sanitizer linking work for the C++ smoke test.

# Changes

## main.c moved out of src/

- Moved the test harness / CLI driver from `src/main.c` to `app/tpcas.c`;
  `src/` is now library-only. Makefile and README updated to match.
- Added `clean.sh`: git-aware cleanup (dry run by default, `--apply` to act)
  that stages removals via `git rm` so they're reversible before commit.

## ds de-vendored

- Removed the vendored `vendor/ds/` subset; ds is now an external dependency.
- Added `ds` as a flake input (`github:hydrastro/ds`), provided as a `buildInput`
  to both the package build and the dev shell.
- Makefile now links against ds via overridable `DS_CFLAGS` / `DS_LIBS`
  (exported by the flake; settable by hand outside Nix).
- Removed migration scaffolding: `scripts/clean-tree.sh`, the `prune-legacy`
  and `distclean` make targets, the dead `run_tests.sh`, the `expl` note, and
  checked-in `build/` artifacts.
- Corrected the documented test count (39, not 45).

## Earlier refactor

- Moved the active TPCAS implementation into `src/`.
- Vendored the required subset of the custom DS library under `vendor/ds/lib/`.
- Reworked `arena_t` to use `ds_arena_t`/`ds_context_t` internally while preserving chunk-growth behavior.
- Reworked parser expectation-message construction to use `ds_str_t`.
- Replaced the Makefile with an out-of-tree build:
  - object files: `build/obj/...`
  - dependency files: `build/dep/...`
  - executable: `build/tpcas`
  - modes: `debug` default, `release`, and `asan`
- Added `scripts/clean-tree.sh` to safely remove old flat-tree garbage after previewing the deletion list.
- Expanded `.gitignore` for generated files and migration exports.
