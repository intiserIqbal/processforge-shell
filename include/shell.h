#ifndef SHELL_H
#define SHELL_H

#define MAX_INPUT 1024
#define MAX_ARGS 64

typedef struct {
    char *args[MAX_ARGS];
    int background;
} Command;

void parse_input(char *input, Command *cmd);
void execute_command(Command *cmd);

int builtin_cd(char **args);
int builtin_help();
int builtin_exit();

void setup_signal_handlers();

#endif