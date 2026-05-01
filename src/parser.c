#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/shell.h"

void parse_command(char *input, Command *cmd)
{
    int arg_index = 0;
    char *token;
    char *saveptr;

    cmd->background = 0;
    cmd->redirect_in = 0;
    cmd->redirect_out = 0;
    cmd->append_out = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;

    token = strtok_r(input, " \t\r\n", &saveptr);

    while (token != NULL && arg_index < MAX_ARGS - 1)
    {
        if (strcmp(token, "&") == 0)
        {
            cmd->background = 1;
        }
        else if (strcmp(token, "<") == 0)
        {
            cmd->redirect_in = 1;
            token = strtok_r(NULL, " \t\r\n", &saveptr);
            if (token)
                cmd->input_file = strdup(token);
        }
        else if (strcmp(token, ">") == 0)
        {
            cmd->redirect_out = 1;
            cmd->append_out = 0;
            token = strtok_r(NULL, " \t\r\n", &saveptr);
            if (token)
                cmd->output_file = strdup(token);
        }
        else if (strcmp(token, ">>") == 0)
        {
            cmd->redirect_out = 1;
            cmd->append_out = 1;
            token = strtok_r(NULL, " \t\r\n", &saveptr);
            if (token)
                cmd->output_file = strdup(token);
        }
        else
        {
            token[strcspn(token, "\r\n")] = 0;
            cmd->args[arg_index++] = strdup(token);
        }

        token = strtok_r(NULL, " \t\r\n", &saveptr);
    }

    cmd->args[arg_index] = NULL;
}

Pipeline parse_input(char *input)
{
    Pipeline p;
    memset(&p, 0, sizeof(Pipeline));

    char *input_copy = strdup(input);
    if (!input_copy)
        return p;

    char *saveptr;
    char *cmd_str = strtok_r(input_copy, "|", &saveptr);

    while (cmd_str != NULL && p.count < MAX_COMMANDS)
    {
        while (*cmd_str == ' ')
            cmd_str++;

        char *end = cmd_str + strlen(cmd_str) - 1;
        while (end > cmd_str && *end == ' ')
            *end-- = '\0';

        char *cmd_copy = strdup(cmd_str);
        if (cmd_copy)
        {
            parse_command(cmd_copy, &p.commands[p.count]);
            free(cmd_copy);
            p.count++;
        }

        cmd_str = strtok_r(NULL, "|", &saveptr);
    }

    free(input_copy);
    return p;
}