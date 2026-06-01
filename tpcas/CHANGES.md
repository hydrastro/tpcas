# Changes

## Cleanup pass

- Removed committed build output (`build/`) from version control and added a
  `.gitignore` covering build products, Nix `result` links, legacy flat-tree
  artifacts, and editor/OS cruft.
- Removed `expl` (an unrelated chat transcript that had been checked in).
- Removed the now-obsolete migration tooling: `scripts/clean-tree.sh`, the
  `prune-legacy` Makefile target, and the `CLEAN_APPLY` plumbing. The flat
  tree it cleaned no longer exists, so the tooling only invited confusion.
  `distclean` is retained as an alias for `clean`.
- Rewrote `run_tests.sh`, which still compiled the old flat tree (`parse.c`,
  `test_roundtrip.c`) and exercised REPL commands (`cnf`, `beta`, `eval`) that
  the current `main.c` does not implement. It now builds via `make` and runs
  the built-in parser-comparison suite plus a few single-expression checks.
- Fixed README drift: dropped the hard-coded "45 expressions" count (the suite
  has 39 fixtures) in favor of "every fixture in `src/main.c`", and removed the
  layout entry and two sections describing the deleted cleanup script.

## Prior refactor

- Moved the active TPCAS implementation into `src/`.
- Vendored the required subset of the custom DS library under `vendor/ds/lib/`.
- Reworked `arena_t` to use `ds_arena_t`/`ds_context_t` internally while
  preserving chunk-growth behavior.
- Reworked parser expectation-message construction to use `ds_str_t`.
- Replaced the Makefile with an out-of-tree build:
  - object files: `build/obj/...`
  - dependency files: `build/dep/...`
  - executable: `build/tpcas`
  - modes: `debug` default, `release`, and `asan`
