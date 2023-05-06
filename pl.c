#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<assert.h>
#include "pl.h"

// constants and symbols could be turned into regexes

const pl_symbol_map_t pl_symbol_maps[] = {
        {"!",   PL_SYMBOL_NOT},
        {"&&",  PL_SYMBOL_AND},
        {"||",  PL_SYMBOL_OR},
        {"=>",  PL_SYMBOL_IMPLIES},
        {"<=>", PL_SYMBOL_IFF}
};

const pl_symbol_t pl_symbols[] = {
        {&(pl_symbol_maps[0].type), 1, PL_NON_ASSOC},
        {&(pl_symbol_maps[1].type), 2, PL_LEFT_ASSOC},
        {&(pl_symbol_maps[2].type), 2, PL_LEFT_ASSOC},
        {&(pl_symbol_maps[3].type), 2, PL_RIGHT_ASSOC},
        {&(pl_symbol_maps[4].type), 2, PL_NON_ASSOC}
};


const pl_constant_t pl_constants[] = {
        {"true",  PL_TRUE},
        {"false", PL_FALSE}
};

bool pl_validate_symbol(char *symbol) {
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
        if (ast->value.symbol.name->argc == 1) {
            return pl_is_well_formed(ast->value.symbol.arguments[0]);
        } else if (ast->value.symbol.name->argc == 2) {
            return pl_is_well_formed(ast->value.symbol.arguments[0]) &&
                   pl_is_well_formed(ast->value.symbol.arguments[1]);
        }
    }
    return false;
}

const pl_symbol_type_t *pl_get_symbol_type(char *symbol) {
    int i;
    for (i = 0; i < sizeof(pl_symbol_maps) / sizeof(pl_symbol_map_t); i++) {
        if (strcmp(pl_symbol_maps[i].symbol, symbol) == 0) {
            return &pl_symbol_maps[i].type;
        }
    }
    return NULL;
}

const pl_symbol_t *pl_get_symbol(char *symbol) {
    int i;
    for (i = 0; i < sizeof(pl_symbols) / sizeof(pl_symbol_t); i++) {
        if (strcmp(pl_symbol_maps[i].symbol, symbol) == 0) {
            return &pl_symbols[i];
        }
    }
    return NULL;
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

pl_node_t *pl_make_symbol(const pl_symbol_t *symbol, int argc, pl_node_t *arguments[]) {
    pl_node_t *node = (pl_node_t *) malloc(sizeof(pl_node_t));
    node->type = PL_SYMBOL;

    node->value.symbol.name = symbol;

    node->value.symbol.arguments = (pl_node_t **) malloc(sizeof(pl_node_t *) * argc);
    memcpy(node->value.symbol.arguments, arguments, sizeof(pl_node_t *) * argc);
    return node;
}

pl_node_t *pl_make_binary(const pl_symbol_t *symbol, pl_node_t *left, pl_node_t *right) {
    pl_node_t *node = pl_make_symbol(symbol, 2, (pl_node_t *[]) {left, right});
    return node;
}

pl_node_t *pl_make_unary(const pl_symbol_t *symbol, pl_node_t *argument) {
    pl_node_t *node = pl_make_symbol(symbol, 1, (pl_node_t *[]) {argument});
    return node;
}

char *pl_get_symbol_char(const pl_symbol_type_t *type) {
    int i;
    for (i = 0; i < sizeof(pl_symbol_maps) / sizeof(pl_symbol_map_t); i++) {
        if (pl_symbol_maps[i].type == *type) {
            return pl_symbol_maps[i].symbol;
        }
    }
    return NULL;
}

void pl_print_tree(pl_node_t *ast) {
    if (ast->type == PL_CONSTANT) {
        printf("%s", ast->value.constant);
    } else if (ast->type == PL_VARIABLE) {
        printf("%s", ast->value.variable);
    } else if (ast->type == PL_SYMBOL) {
        printf("%s (", pl_get_symbol_char(ast->value.symbol.name->type));
        int i;
        for (i = 0; i < ast->value.symbol.name->argc; i++) {
            pl_print_tree(ast->value.symbol.arguments[i]);
            if (i < ast->value.symbol.name->argc - 1) {
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
    char *symbol_char;
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
                symbol_char = pl_get_symbol_char(pl_symbols[j].type);
                if (symbol_char != NULL && strncmp(line + i, symbol_char, strlen(symbol_char)) == 0) {
                    head = pl_add_token_symbol(head, buffer, &buffer_index);
                    head = pl_add_token(head, PL_TOKEN_SYMBOL, symbol_char);
                    i += strlen(symbol_char) - 1;
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

    // we revert the list so that it is in the correct order
    pl_token_node_t *prev = NULL;
    pl_token_node_t *current = head;
    pl_token_node_t *next = NULL;
    while (current != NULL) {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    head = prev;

    return head;
}

int pl_get_symbol_precedence(pl_symbol_type_t type) {
    int i;
    for (i = 0; i < sizeof(pl_symbol_maps) / sizeof(pl_symbol_map_t); i++) {
        if (pl_symbol_maps[i].type == type) {
            return i;
        }
    }
    return -1;
}


int pl_get_precedence(const pl_symbol_type_t *type) {
    int i;
    for (i = 0; i < sizeof(pl_symbol_maps) / sizeof(pl_symbol_map_t); i++) {
        if (pl_symbol_maps[i].type == *type) {
            return i;
        }
    }
    return -1;
}

pl_node_t *pl_build_ast(pl_symbol_list_t *symbol_list, pl_node_list_t *node_list) {
    int i, j;
    pl_node_t *node;
    pl_symbol_node_t *symbol_node = symbol_list->head;
    pl_node_node_t *node_node;
    if (symbol_list->size == 0) {
        if (node_list->size == 1) {
            fflush(stdout);
            return node_list->head->node;
        }
        // bad
        return NULL;
    }
    for (i = 0; i < sizeof(pl_symbols) / sizeof(pl_symbol_t); i++) {
        if (pl_symbols[i].assoc == PL_LEFT_ASSOC || pl_symbols[i].assoc == PL_NON_ASSOC) {
            node_node = node_list->head;
        } else if (pl_symbols[i].assoc == PL_RIGHT_ASSOC) {
            node_node = node_list->tail;
        }
        symbol_node = symbol_list->tail;
        while (symbol_node != NULL) {
            if (symbol_node->symbol->type == pl_symbols[i].type) {
                if (pl_symbols[i].argc == 1) {
                    node = pl_make_unary(&(pl_symbols[i]), node_node->node);
                    node_node->node = node;
                } else if (pl_symbols[i].argc == 2) {
                    pl_node_node_t *temp = node_list->head;
                    while (temp != NULL) {
                        temp = temp->next;
                    }
                    if (pl_symbols[i].assoc == PL_LEFT_ASSOC || pl_symbols[i].assoc == PL_NON_ASSOC) {
                        node = pl_make_binary(&(pl_symbols[i]), node_node->node, node_node->next->node);
                        node_node->node = node;
                        pl_node_list_remove(node_list, node_node->next);
                    } else if (pl_symbols[i].assoc == PL_RIGHT_ASSOC) {
                        pl_print_tree(node_list->tail->node);
                        node = pl_make_binary(&(pl_symbols[i]), node_node->prev->node, node_node->node);
                        node_node->node = node;
                        pl_node_list_remove(node_list, node_node->prev);
                    }
                } else {
                    // bad
                }
            }
            if (pl_symbols[i].assoc == PL_LEFT_ASSOC || pl_symbols[i].assoc == PL_NON_ASSOC) {
                for (j = 0; j < symbol_node->symbol->argc && node_node->prev != NULL; j++) {
                    node_node = node_node->prev;
                }
            } else if (pl_symbols[i].assoc == PL_RIGHT_ASSOC) {
                for (j = 0; j < symbol_node->symbol->argc && node_node->next != NULL; j++) {
                    node_node = node_node->next;
                }
            }
            symbol_node = symbol_node->prev;
        }
    }

    return node_list->head->node;
}

pl_node_t *pl_parse_ast(pl_node_t *ast, pl_token_node_t *tokenized) {
    if (tokenized == NULL) {
        return ast;
    }
    if (ast == NULL) {
        ast = (pl_node_t *) malloc(sizeof(pl_node_t *));
    }
    pl_node_t *node;
    pl_symbol_list_t *symbols_list = pl_symbol_list_create();
    pl_node_list_t *nodes_list = pl_node_list_create();
    while (tokenized != NULL) {
        switch (tokenized->token->type) {
            case PL_TOKEN_RIGHT_PAREN:
                *tokenized = *(tokenized->next);
                node = pl_parse_ast(ast, tokenized);
                pl_node_list_push(nodes_list, node);
                break;
            case PL_TOKEN_LEFT_PAREN:
                if (tokenized->next != NULL) {
                    *tokenized = *(tokenized->next);
                }
                return pl_build_ast(symbols_list, nodes_list);
            case PL_TOKEN_CONSTANT:
                node = pl_make_constant((tokenized)->token->value);
                pl_node_list_push(nodes_list, node);
                break;
            case PL_TOKEN_VARIABLE:
                node = pl_make_variable((tokenized)->token->value);
                pl_node_list_push(nodes_list, node);
                break;
            case PL_TOKEN_SYMBOL:
                const pl_symbol_t *symbol = pl_get_symbol((tokenized)->token->value);
                pl_symbol_list_push(symbols_list, symbol);
                break;
            default:
                break;
        }
        if (tokenized->next == NULL) {
            return pl_build_ast(symbols_list, nodes_list);
        }
        *tokenized = *(tokenized->next);
    }
    return pl_build_ast(symbols_list, nodes_list);
}

int pl_parse_and_print_tree(char *line) {
    pl_token_node_t *tokenized = pl_tokenize(line);
    pl_node_t *tree = pl_parse_ast(NULL, tokenized);
    assert(tree != NULL);
    pl_print_tree(tree);
    printf("\n");
    return 1;
}

int pl_parse_and_eval(char *line) {
    pl_token_node_t *tokenized = pl_tokenize(line);
    pl_node_t *tree = pl_parse_ast(NULL, tokenized);
    assert(tree != NULL);
    return 1;
}

pl_value_t pl_eval_not(pl_value_t value) {
    if (value == PL_TRUE) {
        return PL_FALSE;
    }
    if (value == PL_FALSE) {
        return PL_TRUE;
    }
    return PL_UNKNOWN;
}

pl_value_t pl_eval_and(pl_value_t left, pl_value_t right) {
    if (left == PL_TRUE && right == PL_TRUE) {
        return PL_TRUE;
    }
    if (left == PL_FALSE || right == PL_FALSE) {
        return PL_FALSE;
    }
    return PL_UNKNOWN;
}

pl_value_t pl_eval_or(pl_value_t left, pl_value_t right) {
    if (left == PL_TRUE || right == PL_TRUE) {
        return PL_TRUE;
    }
    if (left == PL_FALSE && right == PL_FALSE) {
        return PL_FALSE;
    }
    return PL_UNKNOWN;
}

pl_value_t pl_eval_implies(pl_value_t left, pl_value_t right) {
    if (left == PL_TRUE && right == PL_FALSE) {
        return PL_FALSE;
    }
    if (left == PL_FALSE || right == PL_TRUE) {
        return PL_TRUE;
    }
    return PL_UNKNOWN;
}

pl_value_t pl_eval_iff(pl_value_t left, pl_value_t right) {
    if (left == PL_TRUE && right == PL_TRUE) {
        return PL_TRUE;
    }
    if (left == PL_FALSE && right == PL_FALSE) {
        return PL_TRUE;
    }
    if (left == PL_TRUE && right == PL_FALSE) {
        return PL_FALSE;
    }
    if (left == PL_FALSE && right == PL_TRUE) {
        return PL_FALSE;
    }
    return PL_UNKNOWN;
}

pl_value_t pl_eval(pl_node_t *ast, pl_variable_t *variables, int variables_count) {
    if (ast == NULL) {
        return PL_UNKNOWN;
    }
    if (ast->type == PL_CONSTANT) {
        for (int i = 0; i < sizeof(pl_constants) / sizeof(pl_constant_t); i++) {
            if (strcmp(ast->value.constant, pl_constants[i].constant) == 0) {
                return pl_constants[i].value;
            }
        }
        return PL_UNKNOWN;
    }

    if (ast->type == PL_VARIABLE) {
        for (int i = 0; i < variables_count; i++) {
            if (strcmp(ast->value.variable, variables[i].variable) == 0) {
                return variables[i].value;
            }
        }
        return PL_UNKNOWN;
    }

    if (ast->type == PL_SYMBOL) {

        switch (*(ast->value.symbol.name->type)) {
            case PL_SYMBOL_NOT:
                return pl_eval_not(pl_eval(ast->value.symbol.arguments[0], variables, variables_count));
            case PL_SYMBOL_AND:
                return pl_eval_and(pl_eval(ast->value.symbol.arguments[0], variables, variables_count),
                                   pl_eval(ast->value.symbol.arguments[1], variables, variables_count));
            case PL_SYMBOL_OR:
                return pl_eval_or(pl_eval(ast->value.symbol.arguments[0], variables, variables_count),
                                  pl_eval(ast->value.symbol.arguments[1], variables, variables_count));
            case PL_SYMBOL_IMPLIES:
                return pl_eval_implies(pl_eval(ast->value.symbol.arguments[0], variables, variables_count),
                                       pl_eval(ast->value.symbol.arguments[1], variables, variables_count));
            case PL_SYMBOL_IFF:
                return pl_eval_iff(pl_eval(ast->value.symbol.arguments[0], variables, variables_count),
                                   pl_eval(ast->value.symbol.arguments[1], variables, variables_count));
        }
    }
    return PL_UNKNOWN;
}

pl_symbol_list_t *pl_symbol_list_create() {
    pl_symbol_list_t *list = (pl_symbol_list_t *) malloc(sizeof(pl_symbol_list_t));
    list->head = NULL;
    list->tail = NULL;
    return list;
}

pl_node_list_t *pl_node_list_create() {
    pl_node_list_t *list = (pl_node_list_t *) malloc(sizeof(pl_node_list_t));
    list->head = NULL;
    list->tail = NULL;
    return list;
}

void pl_symbol_list_push(pl_symbol_list_t *list, const pl_symbol_t *symbol) {
    pl_symbol_node_t *new_node = malloc(sizeof(pl_symbol_node_t));
    new_node->symbol = symbol;
    new_node->next = NULL;
    new_node->prev = list->tail; // set prev field to current tail

    list->size++;

    if (list->head == NULL) {
        list->head = new_node;
        list->tail = new_node;
        return;
    }

    list->tail->next = new_node;
    list->tail = new_node;
}

void pl_node_list_push(pl_node_list_t *list, pl_node_t *node) {
    pl_node_node_t *new_node = malloc(sizeof(pl_node_node_t));
    new_node->node = node;
    new_node->next = NULL;
    new_node->prev = list->tail; // set prev field to current tail

    list->size++;

    if (list->head == NULL) {
        list->head = new_node;
        list->tail = new_node;
        return;
    }

    list->tail->next = new_node;
    list->tail = new_node;
}

const pl_symbol_t *pl_symbol_list_pop(pl_symbol_list_t *list) {
    if (list->head == NULL) {
        return NULL;
    }

    pl_symbol_node_t *node = list->head;
    const pl_symbol_t *symbol = node->symbol;
    list->head = node->next;
    free(node);
    list->size--;
    return symbol;
}

pl_node_t *pl_node_list_pop(pl_node_list_t *list) {
    printf("popping node\n");
    if (list->head == NULL) {
        return NULL;
    }

    pl_node_node_t *node = list->head;
    pl_node_t *value = node->node;
    list->head = node->next;
    free(node);
    list->size--;
    return value;
}

void pl_symbol_list_remove(pl_symbol_list_t *list, const pl_symbol_node_t *node) {
    pl_symbol_node_t *current = list->head;
    while (current != NULL) {
        if (current == node) {
            if (current->prev != NULL) {
                current->prev->next = current->next;
            }
            if (current->next != NULL) {
                current->next->prev = current->prev;
            }
            if (list->head == current) {
                list->head = current->next;
            }
            if (list->tail == current) {
                list->tail = current->prev;
            }
            free(current);
            list->size--;
            return;
        }
        current = current->next;
    }
}

void pl_node_list_remove(pl_node_list_t *list, pl_node_node_t *node) {
    pl_node_node_t *current = list->head;
    while (current != NULL) {
        if (current == node) {
            if (current->prev != NULL) {
                current->prev->next = current->next;
            }
            if (current->next != NULL) {
                current->next->prev = current->prev;
            }
            if (list->head == current) {
                list->head = current->next;
            }
            if (list->tail == current) {
                list->tail = current->prev;
            }
            free(current);
            list->size--;
            return;
        }
        current = current->next;
    }
}
