# Changes

## Current refactor

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
