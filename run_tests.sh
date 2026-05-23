#!/bin/sh
# Smoke-test the REPL.  Compares output against an expected fixture.
set -e
cd "$(dirname "$0")"

# 1. round-trip test (built separately)
cc -Wall -Wextra -std=c11 -g -D_POSIX_C_SOURCE=200809L \
   arena.c ast.c lex.c parse.c print.c test_roundtrip.c -o test_rt
./test_rt

echo
echo "=== REPL smoke test ==="
out=$(./tpcas <<'EOF'
parse A && B
parse forall x. P(x) => exists y. Q(x, y)
cnf (A => B) && (B => C)
cnf !(forall x. P(x))
beta (\f. \x. f(f(x)))(g)(a)
eval true => (false || true)
EOF
)
echo "$out"

# spot-check a few key outputs
echo "$out" | grep -q "A && B"                       || { echo "FAIL: basic and"; exit 1; }
echo "$out" | grep -q "(!A || B) && (!B || C)"       || { echo "FAIL: cnf"; exit 1; }
echo "$out" | grep -q "exists x. !P(x)"              || { echo "FAIL: push not through forall"; exit 1; }
echo "$out" | grep -q "g(g(a))"                      || { echo "FAIL: beta"; exit 1; }
echo "$out" | grep -q "true"                         || { echo "FAIL: eval"; exit 1; }

echo
echo "All tests passed."
