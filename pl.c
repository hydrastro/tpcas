#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include "pl.h"

// constants and symbols could be turned into regexes

pl_symbol_map_t pl_symbol_maps[] = {
        {"!", PL_SYMBOL_NOT},
        {"&&", PL_SYMBOL_AND},
        {"||", PL_SYMBOL_OR},
        {"=>", PL_SYMBOL_IMPLIES},
        {"<=>", PL_SYMBOL_IFF}
};

pl_symbol_t pl_symbols[] = {
        {"!",   1, PL_NON_ASSOC},
        {"&&",  2, PL_LEFT_ASSOC},
        {"||",  2, PL_LEFT_ASSOC},
        {"=>",  2, PL_RIGHT_ASSOC},
        {"<=>", 2, PL_NON_ASSOC}
};

pl_constant_t pl_constants[] = {
        {"true",  PL_TRUE},
        {"false", PL_FALSE}
};

bool pl_validate_symbol(char* symbol) {
    int i;
    for (i = 0; i < sizeof(pl_symbol_maps) / sizeof(pl_symbol_map_t); i++) {
        if (strcmp(symbol, pl_symbol_maps[i].symbol) == 0) {
            return true;
        }
    }
    return false;
}

bool pl_is_well_formed(pl_node_t *ast) {
    if (ast->type == PL_CONSTANT) {
        return true;
    } else if (ast->type == PL_VARIABLE) {
        return true;
    } else if (ast->type == PL_SYMBOL) {

        int i;
        for (i = 0; i < ast->value.symbol.name.argc; i++) {
            if (!pl_is_well_formed(ast->value.symbol.arguments[i])) {
                return false;
            }
        }
    } else {
        return false;
    }
    return true;
}

pl_node_t *pl_make_constant(char *constant) {
    pl_node_t *node = (pl_node_t *) malloc(sizeof(pl_node_t));
    node->type = PL_CONSTANT;
    node->value.constant = constant;
    return node;
}

pl_node_t *pl_make_variable(char *variable) {
    pl_node_t *node = (pl_node_t *) malloc(sizeof(pl_node_t));
    node->type = PL_VARIABLE;
    node->value.variable = variable;
    return node;
}

pl_node_t *pl_make_symbol(char* symbol, int argc, pl_node_t *arguments[]) {
    pl_node_t *node = (pl_node_t *) malloc(sizeof(pl_node_t));
    node->type = PL_SYMBOL;
    int i;
    for (i = 0; i < sizeof(pl_symbols) / sizeof(pl_symbol_t); i++) {
        if (strcmp(pl_symbols[i].symbol, symbol) == 0) {
            node->value.symbol.name = pl_symbols[i];
            break;
        }
    }
    if (i == sizeof(pl_symbols) / sizeof(pl_symbol_t)) {
        // operator not found
        return NULL;
    }
    if (argc != node->value.symbol.name.argc) {
        // wrong number of arguments
        return NULL;
    }
    node->value.symbol.arguments = (pl_node_t **) malloc(sizeof(pl_node_t *) * argc);
    memcpy(node->value.symbol.arguments, arguments, sizeof(pl_node_t *) * argc);
    return node;
}

pl_node_t *pl_make_binary(char* symbol, pl_node_t *left, pl_node_t *right) {
    pl_node_t *node = pl_make_symbol(symbol, 2, (pl_node_t *[]) {left, right});
    return node;
}

pl_node_t *pl_make_unary(char* symbol, pl_node_t *argument) {
    pl_node_t *node = pl_make_symbol(symbol, 1, (pl_node_t *[]) {argument});
    return node;
}

void pl_print_tree(pl_node_t *ast) {
    if (ast->type == PL_CONSTANT) {
        printf("%s", ast->value.constant);
    } else if (ast->type == PL_VARIABLE) {
        printf("%s", ast->value.variable);
    } else if (ast->type == PL_SYMBOL) {
        printf("%s (", ast->value.symbol.name.symbol);
        int i;
        for (i = 0; i < ast->value.symbol.name.argc; i++) {
            pl_print_tree(ast->value.symbol.arguments[i]);
            if (i < ast->value.symbol.name.argc - 1) {
                printf(", ");
            }
        }
        printf(")");
    } else {
        printf("unknown node type");
    }
}

pl_token_node_t *pl_add_token(pl_token_node_t *head, pl_token_type_t type, char *value) {
    pl_token_node_t *temp = (pl_token_node_t *) malloc(sizeof(pl_token_node_t));
    temp->token = (pl_token_t *) malloc(sizeof(pl_token_t));
    temp->token->type = type;
    temp->token->value = value;
    temp->next = head;
    head = temp;
    return head;
}

pl_token_node_t *pl_add_token_symbol(pl_token_node_t *head, char *buffer, int *buffer_index) {
    if (*buffer_index > 0) {
        char *value = (char *) malloc(sizeof(char) * (*buffer_index + 1));
        memcpy(value, buffer, sizeof(char) * (*buffer_index));
        value[*buffer_index] = '\0';
        head = pl_add_token(head, PL_TOKEN_VARIABLE, value);
        *buffer_index = 0;
        memset(buffer, 0, sizeof(char) * 100);
    }
    return head;
}

pl_token_node_t *pl_tokenize(char *line) {
    pl_token_node_t *head = NULL;
    char buffer[100];
    int buffer_index = 0;
    int i = 0;
    bool match;
    while (line[i] != '\0') {
        match = false;
        if (line[i] == ' ') {
            continue;
        }
        if (line[i] == '(') {
            head = pl_add_token_symbol(head, buffer, &buffer_index);
            head = pl_add_token(head, PL_TOKEN_LEFT_PAREN, "(");
        } else if (line[i] == ')') {
            head = pl_add_token_symbol(head, buffer, &buffer_index);
            head = pl_add_token(head, PL_TOKEN_RIGHT_PAREN, ")");
        } else {
            for (int j = 0; j < sizeof(pl_symbols) / sizeof(pl_symbols[0]); j++) {
                if (strncmp(line + i, pl_symbols[j].symbol, strlen(pl_symbols[j].symbol)) == 0) {
                    head = pl_add_token_symbol(head, buffer, &buffer_index);
                    head = pl_add_token(head, PL_TOKEN_SYMBOL, pl_symbols[j].symbol);
                    i += strlen(pl_symbols[j].symbol) - 1;
                    match = true;
                    break;
                }
            }
            for (int j = 0; j < sizeof(pl_constants) / sizeof(pl_constants[0]); j++) {
                if (strncmp(line + i, pl_constants[j].constant, strlen(pl_constants[j].constant)) == 0) {
                    head = pl_add_token_symbol(head, buffer, &buffer_index);
                    head = pl_add_token(head, PL_TOKEN_CONSTANT, pl_constants[j].constant);
                    i += strlen(pl_constants[j].constant) - 1;
                    match = true;
                    break;
                }
            }
            if (!match) {
                buffer[buffer_index] = line[i];
                buffer_index++;
            }
        }
        i++;
    }
    head = pl_add_token_symbol(head, buffer, &buffer_index);
    return head;
}

pl_node_t *pl_parse_ast(pl_node_t *ast, pl_token_node_t *tokenized) {
    if (tokenized == NULL) {
        return ast;
    }
    if (ast == NULL) {
        ast = (pl_node_t *) malloc(sizeof(pl_node_t *));
    }
    pl_node_t *node;
    while (tokenized != NULL) {
        switch (tokenized->token->type) {
            case PL_TOKEN_RIGHT_PAREN:
                *tokenized = *(tokenized->next);
                node = pl_parse_ast(ast, tokenized);
                break;
            case PL_TOKEN_LEFT_PAREN:
                if(tokenized->next != NULL) {
                    *tokenized = *(tokenized->next);
                }
                return ast;
            case PL_TOKEN_CONSTANT:
                node = pl_make_constant((tokenized)->token->value);
                break;
            case PL_TOKEN_VARIABLE:
                node = pl_make_variable((tokenized)->token->value);
                break;
            case PL_TOKEN_SYMBOL:

                for (int i = 0; i < sizeof(pl_symbols) / sizeof(pl_symbol_t); i++) {
                    if (strcmp((tokenized)->token->value, pl_symbols[i].symbol) != 0) {
                        continue;
                    }

                    if (pl_symbols[i].argc == 1) {
                        node = pl_make_unary(pl_symbols[i].symbol, ast);
                    } else if (pl_symbols[i].argc == 2) {
                        // non associative operators go left because i said so
                        if (pl_symbols[i].assoc == PL_LEFT_ASSOC || pl_symbols[i].assoc == PL_NON_ASSOC) {
                            tokenized = tokenized->next;
                            pl_node_t *right = pl_parse_ast(NULL, tokenized);
                            node = pl_make_binary(pl_symbols[i].symbol, ast, right);
                            return node;
                        } else if (pl_symbols[i].assoc == PL_RIGHT_ASSOC) {
                            tokenized = tokenized->next;
                            pl_node_t *left = pl_parse_ast(NULL, tokenized);
                            node = pl_make_binary(pl_symbols[i].symbol, left, ast);
                            return node;
                        }
                    } else {
                        node = NULL;
                    }
                }

                break;
            default:
                break;
        }
        ast = node;
        if (tokenized->next == NULL) {
            return ast;
        }
        *tokenized = *(tokenized->next);
    }

    return ast;
}

int pl_parse_and_print_tree(char *line) {
    pl_token_node_t *tokenized = pl_tokenize(line);


    pl_node_t *tree = pl_parse_ast(NULL, tokenized);
    pl_print_tree(tree);
    printf("\n");
    return 1;
}

pl_value_t pl_eval_not(pl_value_t value) {
    if(value == PL_TRUE) {
        return PL_FALSE;
    }
    if(value == PL_FALSE) {
        return PL_TRUE;
    }
    return PL_UNKNOWN;
}

pl_value_t pl_eval_and(pl_value_t left, pl_value_t right) {
    if(left == PL_TRUE && right == PL_TRUE) {
        return PL_TRUE;
    }
    if(left == PL_FALSE || right == PL_FALSE) {
        return PL_FALSE;
    }
    return PL_UNKNOWN;
}

pl_value_t pl_eval_or(pl_value_t left, pl_value_t right) {
    if(left == PL_TRUE || right == PL_TRUE) {
        return PL_TRUE;
    }
    if(left == PL_FALSE && right == PL_FALSE) {
        return PL_FALSE;
    }
    return PL_UNKNOWN;
}

pl_value_t pl_eval_implies(pl_value_t left, pl_value_t right) {
    if(left == PL_TRUE && right == PL_FALSE) {
        return PL_FALSE;
    }
    if(left == PL_FALSE || right == PL_TRUE) {
        return PL_TRUE;
    }
    return PL_UNKNOWN;
}

pl_value_t pl_eval_iff(pl_value_t left, pl_value_t right) {
    if(left == PL_TRUE && right == PL_TRUE) {
        return PL_TRUE;
    }
    if(left == PL_FALSE && right == PL_FALSE) {
        return PL_TRUE;
    }
    if(left == PL_TRUE && right == PL_FALSE) {
        return PL_FALSE;
    }
    if(left == PL_FALSE && right == PL_TRUE) {
        return PL_FALSE;
    }
    return PL_UNKNOWN;
}

pl_value_t pl_eval(pl_node_t*ast, pl_variable_t*variables, int variables_count) {

    // work in progress..

    if(ast == NULL) {
        return PL_UNKNOWN;
    }
    if(ast->type == PL_CONSTANT) {
        for(int i = 0; i < sizeof(pl_constants) / sizeof(pl_constant_t); i++) {
            if(strcmp(ast->value.constant, pl_constants[i].constant) == 0) {
                return pl_constants[i].value;
            }
        }
        return PL_UNKNOWN;
    }

    if(ast->type == PL_VARIABLE) {
        for(int i = 0; i < variables_count; i++) {
            if(strcmp(ast->value.variable, variables[i].variable) == 0) {
                return variables[i].value;
            }
        }
        return PL_UNKNOWN;
    }

    if(ast->type == PL_SYMBOL) {

    }
    return PL_UNKNOWN;
}
