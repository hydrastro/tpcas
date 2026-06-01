#!/bin/sh
# Smoke-test tpcas against the current src/ tree.
#
# The built-in suite (build/tpcas with no args) runs every fixture in
# main.c through BOTH the Pratt parser and the parser-combinator parser
# and asserts the resulting ASTs are alpha-equivalent. We also spot-check
# a couple of single-expression parses so a regression in the CLI driver
# is caught even if the suite tables change.
set -eu
cd "$(dirname "$0")"

echo "=== build ==="
make MODE=debug

echo
echo "=== built-in parser comparison suite ==="
./build/tpcas

echo
echo "=== single-expression spot checks ==="
check() {
    expr=$1
    want=$2
    out=$(./build/tpcas "$expr" 2>&1)
    if printf '%s\n' "$out" | grep -q -- "$want"; then
        echo "OK   $expr"
    else
        echo "FAIL $expr"
        echo "     wanted substring: $want"
        echo "     got: $out"
        exit 1
    fi
}

check 'A && B'                'A && B'
check 'forall x. P(x)'        'forall x'
check 'A => B => C'           'A => B => C'

echo
echo "All tests passed."
