#include <string.h>
#include "../include/shell.h"

// Helper function to parse a single command string into Command struct
void parse_command(char *input, Command *cmd) {
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

// Main parser function to handle pipes
Pipeline parse_input(char *input) {
    Pipeline p;
    memset(&p, 0, sizeof(Pipeline));

    char *pipe_pos = strchr(input, '|');
    if (pipe_pos) {
        *pipe_pos = '\0';
        char *right_cmd = pipe_pos + 1;

        // Remove leading/trailing spaces
        while (*right_cmd == ' ') right_cmd++;
        while (input[strlen(input) - 1] == ' ') input[strlen(input) - 1] = '\0';

        parse_command(input, &p.left);
        parse_command(right_cmd, &p.right);
        p.is_pipe = 1;
    } else {
        parse_command(input, &p.left);
        p.is_pipe = 0;
    }
    return p;
}