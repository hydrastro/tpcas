#include "eval.h"
#include <string.h>

const char *pl_value_name(pl_value_t v) {
    switch (v) {
        case PL_TRUE:    return "true";
        case PL_FALSE:   return "false";
        case PL_UNKNOWN: return "?";
    }
    return "?";
}

static pl_value_t v_not(pl_value_t v) {
    if (v == PL_TRUE)  return PL_FALSE;
    if (v == PL_FALSE) return PL_TRUE;
    return PL_UNKNOWN;
}
static pl_value_t v_and(pl_value_t l, pl_value_t r) {
    if (l == PL_FALSE || r == PL_FALSE) return PL_FALSE;
    if (l == PL_TRUE  && r == PL_TRUE)  return PL_TRUE;
    return PL_UNKNOWN;
}
static pl_value_t v_or(pl_value_t l, pl_value_t r) {
    if (l == PL_TRUE  || r == PL_TRUE)  return PL_TRUE;
    if (l == PL_FALSE && r == PL_FALSE) return PL_FALSE;
    return PL_UNKNOWN;
}
static pl_value_t v_imp(pl_value_t l, pl_value_t r) {
    if (l == PL_FALSE || r == PL_TRUE)  return PL_TRUE;
    if (l == PL_TRUE  && r == PL_FALSE) return PL_FALSE;
    return PL_UNKNOWN;
}
static pl_value_t v_iff(pl_value_t l, pl_value_t r) {
    if (l == PL_UNKNOWN || r == PL_UNKNOWN) return PL_UNKNOWN;
    return l == r ? PL_TRUE : PL_FALSE;
}

pl_value_t pl_eval(const node_t *ast, const pl_binding_t *env, size_t n) {
    if (!ast) return PL_UNKNOWN;
    switch (ast->kind) {
        case NODE_CONST:
            if (strcmp(ast->cnst.name, "true")  == 0) return PL_TRUE;
            if (strcmp(ast->cnst.name, "false") == 0) return PL_FALSE;
            return PL_UNKNOWN;
        case NODE_VAR:
            for (size_t i = 0; i < n; i++)
                if (strcmp(env[i].name, ast->var.name) == 0) return env[i].value;
            return PL_UNKNOWN;
        case NODE_APP:
            if (ast->app.head->kind == NODE_CONST && ast->app.head->cnst.op) {
                const op_info_t *op = ast->app.head->cnst.op;
                if (op == &OP_NOT && ast->app.argc == 1)
                    return v_not(pl_eval(ast->app.args[0], env, n));
                if (ast->app.argc == 2) {
                    pl_value_t l = pl_eval(ast->app.args[0], env, n);
                    pl_value_t r = pl_eval(ast->app.args[1], env, n);
                    if (op == &OP_AND) return v_and(l, r);
                    if (op == &OP_OR)  return v_or(l, r);
                    if (op == &OP_IMP) return v_imp(l, r);
                    if (op == &OP_IFF) return v_iff(l, r);
                    if (op == &OP_EQ)  return v_iff(l, r);
                }
            }
            return PL_UNKNOWN;
        default:
            return PL_UNKNOWN;
    }
}
