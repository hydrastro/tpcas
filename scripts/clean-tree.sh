#!/usr/bin/env sh
set -eu

# Clean a TPCAS checkout after the migration from a flat source tree to src/.
#
# Default mode is dry-run. Pass --apply to actually remove files.
# This script intentionally removes only known generated files, old root-level
# source duplicates, and previously exported refactor archives/directories.

apply=0
root="."

usage() {
    cat <<USAGE
Usage: $0 [--apply] [--root DIR]

Without --apply, prints what would be removed.

Removes:
  - build outputs: build/, *.o, tpcas, tpcas3, test_rt
  - old flat-tree source/header duplicates at repo root
  - temporary/exported refactor archives and directories

Preserves:
  - src/, vendor/, README.md, CHANGES.md, Makefile, scripts/
USAGE
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --apply)
            apply=1
            shift
            ;;
        --root)
            [ "$#" -ge 2 ] || { echo "error: --root needs a directory" >&2; exit 2; }
            root=$2
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "error: unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

[ -d "$root" ] || { echo "error: not a directory: $root" >&2; exit 2; }
cd "$root"

# Refuse to run somewhere that does not look like this project.
[ -d src ] || { echo "error: src/ not found; refusing to clean" >&2; exit 2; }
[ -f Makefile ] || { echo "error: Makefile not found; refusing to clean" >&2; exit 2; }

remove_path() {
    path=$1
    [ -e "$path" ] || return 0
    if [ "$apply" -eq 1 ]; then
        rm -rf -- "$path"
        printf 'removed %s\n' "$path"
    else
        printf 'would remove %s\n' "$path"
    fi
}

# Build products and binaries.
remove_path build
remove_path tpcas
remove_path tpcas3
remove_path test_rt

# Old root-level object files.
for f in ./*.o; do
    [ -e "$f" ] || continue
    remove_path "${f#./}"
done

# Old flat source tree duplicates. These should now live under src/.
for f in \
    arena.c arena.h \
    ast.c ast.h \
    combo.c combo.h \
    eval.c eval.h \
    fol.h \
    lex.c lex.h \
    main.c \
    parse.c parse.h \
    pc.c pc.h \
    pl.c pl.h \
    pratt.c pratt.h \
    print.c print.h \
    repl.c repl.h \
    rewrite.c rewrite.h \
    test_roundtrip.c \
    transform.c transform.h
 do
    remove_path "$f"
done

# Export artifacts from earlier ChatGPT/refactor passes.
remove_path src.zip
remove_path tpcas_ds_refactor.zip
remove_path tpcas_ds_refactor

if [ "$apply" -eq 0 ]; then
    echo
    echo "Dry run only. Re-run with --apply to delete these paths."
fi
