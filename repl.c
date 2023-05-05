#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "repl.h"
#include "pl.h"

command_regex_map_t command_regex_maps[COMMAND_REGEX_MAPS_SIZE] = {
        {
                .regex = "^[Tt][Rr][Ee]{2}\\((.*)\\)",
                .execute_function = &pl_parse_and_print_tree
        },
        {
                .regex = "^([Ee][Xx][Ii][Tt]|[Qq][Uu][Ii][Tt]).*",
                .execute_function = &shell_exit
        }
};

void compile_command_regex_maps(void) {
    int i;
    for (i = 0; i < COMMAND_REGEX_MAPS_SIZE; i++) {
        if (regcomp(
                &command_regex_maps[i].compiled_regex,
                command_regex_maps[i].regex,
                REG_EXTENDED
        )) {
            fprintf(stderr, "Could not compile regex\n");
            exit(1);
        }
    }
}

int shell_exit(char *line) {
    int i;
    for (i = 0; i < COMMAND_REGEX_MAPS_SIZE; i++) {
        regfree(&command_regex_maps[i].compiled_regex);
    }
    return 0;
}

int process_line(char *line) {
    regex_t regex;
    int return_value, i;
    char msgbuf[100];
    regmatch_t match[2];
    char *match_string = NULL;
    for (i = 0; i < COMMAND_REGEX_MAPS_SIZE; i++) {
        return_value = regexec(&command_regex_maps[i].compiled_regex, line, 2, match, 0);
        if (!return_value) {
            if (match[1].rm_so == -1) {
                match_string = NULL;
            } else {
                match_string = (char *) malloc(match[1].rm_eo - match[1].rm_so + 1);
                strncpy(match_string, line + match[1].rm_so, match[1].rm_eo - match[1].rm_so);
                match_string[match[1].rm_eo - match[1].rm_so] = '\0';
            }
            return (*command_regex_maps[i].execute_function)(match_string);
        } else if (return_value == REG_NOMATCH) {
            // do nothing
        } else {
            regerror(return_value, &regex, msgbuf, sizeof(msgbuf));
            fprintf(stderr, "Regex match failed: %s\n", msgbuf);
            exit(1);
        }
    }

    return 1;
}

char *read_line(void) {
    char *line;
    ssize_t buffer_size;
    line = NULL;
    buffer_size = 0;
    if (getline(&line, &buffer_size, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        } else {
            perror("Error: getline\n");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

void shell_loop() {
    char *line;
    int status;
    do {
        fprintf(stdout, "> ");
        line = read_line();
        status = process_line(line);
        free(line);
    } while (status);
}
