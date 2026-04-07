#include <stdio.h>
#include <string.h>
#include "../include/shell.h"

void parse_command(char *input, Command *cmd)
{
    int arg_index = 0;
    char *token;

    cmd->background = 0;
    cmd->redirect_in = 0;
    cmd->redirect_out = 0;
    cmd->append_out = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;

    token = strtok(input, " \t\r\n");

    while (token != NULL && arg_index < MAX_ARGS - 1)
    {
        if (strcmp(token, "&") == 0)
        {
            cmd->background = 1;
        }
        else if (strcmp(token, "<") == 0)
        {
            cmd->redirect_in = 1;
            token = strtok(NULL, " \t\r\n");
            if (token)
                cmd->input_file = token;
        }
        else if (strcmp(token, ">") == 0)
        {
            cmd->redirect_out = 1;
            cmd->append_out = 0;
            token = strtok(NULL, " \t\r\n");
            if (token)
                cmd->output_file = token;
        }
        else if (strcmp(token, ">>") == 0)
        {
            cmd->redirect_out = 1;
            cmd->append_out = 1;
            token = strtok(NULL, " \t\r\n");
            if (token)
                cmd->output_file = token;
        }
        else
        {
            token[strcspn(token, "\r\n")] = 0;
            cmd->args[arg_index++] = token;
        }

        token = strtok(NULL, " \t\r\n");
    }

    cmd->args[arg_index] = NULL;
}

Pipeline parse_input(char *input)
{
    Pipeline p;
    memset(&p, 0, sizeof(Pipeline));

    char *cmd_str = strtok(input, "|");

    while (cmd_str != NULL && p.count < MAX_COMMANDS)
    {
        // trim leading spaces
        while (*cmd_str == ' ')
            cmd_str++;

        // trim trailing spaces
        char *end = cmd_str + strlen(cmd_str) - 1;
        while (end > cmd_str && *end == ' ')
        {
            *end = '\0';
            end--;
        }

        parse_command(cmd_str, &p.commands[p.count]);
        p.count++;

        cmd_str = strtok(NULL, "|");
    }

    return p;
}
