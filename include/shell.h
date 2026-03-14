#ifndef SHELL_H
#define SHELL_H

#define MAX_INPUT 1024
#define MAX_ARGS 64

typedef struct {
    char *args[MAX_ARGS];
    int background;
} Command;

typedef struct {
    Command left;   // command before '|'
    Command right;  // command after '|'
    int is_pipe;    // 1 if pipe exists, 0 otherwise
} Pipeline;

Pipeline parse_input(char *input);
void execute_command(Command *cmd);
void execute(Pipeline *pipeline);

int builtin_cd(char **args);
int builtin_help();
int builtin_exit();

void setup_signal_handlers();

#endif