#ifndef CAS_FOL_H
#define CAS_FOL_H

// work in progress...

typedef struct fol_node {
    enum {
        FOL_CONSTANT,
        FOL_VARIABLE,
        FOL_FUNCTION,
        FOL_PREDICATE
    } type;
    union {
        char *constant;
        char *variable;
        struct {
            char *name;
            int argc;
            struct fol_node **arguments;
        } function;
        struct {
            char *name;
            int argc;
            struct fol_node **arguments;
        } predicate;
    } value;
} fol_node_t;


#endif //CAS_FOL_H
