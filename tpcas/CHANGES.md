# Changes

## ds str API rename

- Updated `src/pc.c` for ds's `ds_`-prefixed string API: `str_create`,
  `str_pushc`, `str_append_cstr`, `str_destroy` -> their `ds_str_*` forms.
  The `FUNC_str_cstr` / `FUNC_str_len` read-back macros are unchanged.

## ds include style

- Switched ds includes from `"lib/…"` to the umbrella `<ds.h>` in
  `src/arena.h`, `src/arena.c`, and `src/pc.c`. ds is expected to install
  `ds.h` at the top of its include dir; `DS_CFLAGS` still supplies the `-I`.

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
