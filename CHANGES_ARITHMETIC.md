# tpcas — restore arithmetic operators (+ - * /)

A downstream consumer (dynsys) embeds tpcas as a general expression parser for
ODE/map right-hand sides like `sigma * (y - x)`. tpcas had become logic-only
(operators: ! && || = => <=>), so those expressions no longer tokenized or
parsed. This restores the arithmetic layer.

## Changes
- src/ast.h, src/ast.c: re-add the four arithmetic operators
    OP_MUL "*" (prec 85), OP_DIV "/" (85), OP_ADD "+" (80), OP_SUB "-" (80)
  and register them in the OPS table. They bind tighter than the logical
  connectives, so `a*b + c = d && e` groups as `((((a*b)+c) = d) && e)`.
  (The lexer needs no change: match_op() reads the OPS table, so it now
   tokenizes + - * / automatically. `->` still wins over `-` via the existing
   arrow check; `-` as an operator forbids a following `>`.)
- src/combo.c:
  - add map_op_value + combine_binop_from_value helpers so operators sharing a
    precedence level (* and /, + and -) can be parsed by one chain;
  - insert two arithmetic precedence levels (mul/div, then add/sub) between the
    prefix layer and the logical chain;
  - add prefix unary minus: `-x` parses as `(0 - x)`.

## Verified
- All 7 tpcas sources compile clean (-Wall -Wextra, 0 warnings on the changed
  files ast.c / combo.c).
- Parsing (validated with a test harness):
    sigma * (y - x)      -> sigma * (y - x)
    x * (rho - z) - y    -> x * (rho - z) - y
    x * y - beta * z     -> x * y - beta * z
    a + b * c            -> a + (b*c)        (precedence)
    -x + 1               -> (0 - x) + 1      (unary minus)
    a*b + c = d && e     -> ((((a*b)+c)=d)&&e) (arith tighter than logic)
  Logical operators (&&, ||, =, =>, <=>, !) still parse unchanged.
- AST shape: `p + q` -> NODE_APP, head.cnst.op == &OP_ADD, argc == 2, which is
  exactly what downstream pointer-comparison code (e.g. dynsys `op == &OP_ADD`)
  expects.

## Note
Also removed checked-in build/ artifacts and a stray src.zip from the repo root.
