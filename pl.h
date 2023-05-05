#ifndef CAS_PL_H
#define CAS_PL_H

#include<stdbool.h>

typedef enum {
    PL_LEFT_ASSOC,
    PL_RIGHT_ASSOC,
    PL_NON_ASSOC
} pl_assoc_t;

typedef enum {
    PL_SYMBOL_NOT,
    PL_SYMBOL_AND,
    PL_SYMBOL_OR,
    PL_SYMBOL_IMPLIES,
    PL_SYMBOL_IFF,
} pl_symbol_type_t;

typedef struct {
    char *symbol;
    pl_symbol_type_t type;
} pl_symbol_map_t;

typedef struct pl_symbol {
    char *symbol;
    int argc;
    pl_assoc_t assoc;
} pl_symbol_t;

typedef enum pl_value {
    PL_TRUE,
    PL_FALSE,
    PL_UNKNOWN
} pl_value_t;

typedef struct pl_constant {
    char *constant;
    pl_value_t value;
} pl_constant_t;

typedef struct pl_variable {
    char *variable;
    pl_value_t value;
} pl_variable_t;

typedef struct pl_node {
    enum {
        PL_CONSTANT,
        PL_VARIABLE,
        PL_SYMBOL
    } type;
    union {
        char *constant;
        char *variable;
        struct {
            pl_symbol_t name;
            struct pl_node **arguments;
        } symbol;
    } value;
} pl_node_t;

typedef enum {
    PL_TOKEN_LEFT_PAREN,
    PL_TOKEN_RIGHT_PAREN,
    PL_TOKEN_CONSTANT,
    PL_TOKEN_VARIABLE,
    PL_TOKEN_SYMBOL
} pl_token_type_t;

typedef struct pl_token {
    pl_token_type_t type;
    char *value;
} pl_token_t;


typedef struct pl_token_node {
    pl_token_t *token;
    struct pl_token_node *next;
} pl_token_node_t;

bool pl_validate_symbol(char *symbol);

bool pl_is_well_formed(pl_node_t *ast);

pl_node_t *pl_make_constant(char *constant);

pl_node_t *pl_make_variable(char *variable);

pl_node_t *pl_make_symbol(char *symbol, int argc, pl_node_t *arguments[]);

pl_node_t *pl_make_binary(char *symbol, pl_node_t *left, pl_node_t *right);

pl_node_t *pl_make_unary(char *symbol, pl_node_t *argument);

void pl_print_tree(pl_node_t *ast);

pl_token_node_t *pl_add_token(pl_token_node_t *head, pl_token_type_t type, char *value);

pl_token_node_t *pl_add_token_symbol(pl_token_node_t *head, char *buffer, int *buffer_index);

pl_token_node_t *pl_tokenize(char *line);

pl_node_t *pl_parse_ast(pl_node_t *ast, pl_token_node_t *tokenized);

int pl_parse_and_print_tree(char *line);

pl_value_t pl_eval_not(pl_value_t value);

pl_value_t pl_eval_and(pl_value_t left, pl_value_t right);

pl_value_t pl_eval_or(pl_value_t left, pl_value_t right);

pl_value_t pl_eval_implies(pl_value_t left, pl_value_t right);

pl_value_t pl_eval_iff(pl_value_t left, pl_value_t right);

pl_value_t pl_eval(pl_node_t *ast, pl_variable_t *variables, int variables_count);


#endif //CAS_PL_H
