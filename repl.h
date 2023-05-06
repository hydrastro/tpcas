#ifndef CAS_REPL_H
#define CAS_REPL_H
#define COMMAND_REGEX_MAPS_SIZE 3
#include <regex.h>

typedef enum SHELL_EXIT_CODES {
    SHELL_EXIT_SUCCESS,
    SHELL_EXIT_FAILURE
} SHELL_EXIT_CODES;

typedef struct command_regex_map {
    char *regex;
    regex_t compiled_regex;

    int (*execute_function)(char *);
} command_regex_map_t;


void compile_command_regex_maps(void);

int shell_exit(char *line);

int process_line(char *line);

char *read_line(void);

void shell_loop();

#endif //CAS_REPL_H
