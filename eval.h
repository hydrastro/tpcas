#ifndef EVAL_H
#define EVAL_H

#include <stdbool.h>
#include <stddef.h>
#include "ast.h"

typedef enum { PL_TRUE, PL_FALSE, PL_UNKNOWN } pl_value_t;

typedef struct {
    const char *name;
    pl_value_t  value;
} pl_binding_t;

/* Three-valued evaluation: variables with no binding evaluate to PL_UNKNOWN. */
pl_value_t pl_eval(const node_t *ast, const pl_binding_t *env, size_t n);

const char *pl_value_name(pl_value_t v);

#endif
