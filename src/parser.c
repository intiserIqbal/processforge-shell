#include <string.h>
#include "../include/shell.h"

void parse_input(char *input, Command *cmd) {

    int i = 0;
    char *token;

    cmd->background = 0;

    token = strtok(input, " ");

    while (token != NULL && i < MAX_ARGS - 1) {

        if (strcmp(token, "&") == 0) {
            cmd->background = 1;
        } else {
            cmd->args[i++] = token;
        }

        token = strtok(NULL, " ");
    }

    cmd->args[i] = NULL;
}