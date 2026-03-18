#include <stdio.h>
#include <string.h>
#include "../include/shell.h"

void parse_command(char *input, Command *cmd) {
    int arg_index = 0;
    char *token;

    cmd->background = 0;
    cmd->redirect_in = 0;
    cmd->redirect_out = 0;
    cmd->append_out = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;

    token = strtok(input, " \t\r\n");

    while (token != NULL && arg_index < MAX_ARGS - 1) {

        if (strcmp(token, "&") == 0) {
            cmd->background = 1;

        } else if (strcmp(token, "<") == 0) {
            cmd->redirect_in = 1;
            token = strtok(NULL, " \t\r\n");
            if (token) cmd->input_file = token;

        } else if (strcmp(token, ">") == 0) {
            cmd->redirect_out = 1;
            cmd->append_out = 0;
            token = strtok(NULL, " \t\r\n");
            if (token) cmd->output_file = token;

        } else if (strcmp(token, ">>") == 0) {
            cmd->redirect_out = 1;
            cmd->append_out = 1;
            token = strtok(NULL, " \t\r\n");
            if (token) cmd->output_file = token;

        } else {
            token[strcspn(token, "\r\n")] = 0;
            cmd->args[arg_index++] = token;
        }

        token = strtok(NULL, " \t\r\n");
    }

    cmd->args[arg_index] = NULL;
}

Pipeline parse_input(char *input) {
    Pipeline p;
    memset(&p, 0, sizeof(Pipeline));

    int pipe_count = 0;
    for (char *ptr = input; *ptr; ++ptr)
        if (*ptr == '|') pipe_count++;

    if (pipe_count > 1) {
        fprintf(stderr, "Error: Only single-stage pipelines are supported.\n");
        return p;
    }

    char *pipe_pos = strchr(input, '|');

    if (pipe_pos) {
        *pipe_pos = '\0';
        char *right_cmd = pipe_pos + 1;

        while (*right_cmd == ' ') right_cmd++;

        while (strlen(input) > 0 && input[strlen(input) - 1] == ' ')
            input[strlen(input) - 1] = '\0';

        parse_command(input, &p.commands[0]);
        parse_command(right_cmd, &p.commands[1]);

        p.count = 2;

    } else {
        parse_command(input, &p.commands[0]);
        p.count = 1;
    }

    return p;
}