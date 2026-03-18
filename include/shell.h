#ifndef SHELL_H
#define SHELL_H

#define MAX_INPUT 1024
#define MAX_ARGS 64

typedef struct {
    char *args[MAX_ARGS];
    int background;

    // Redirection fields
    int redirect_in;
    int redirect_out;
    int append_out;
    char *input_file;
    char *output_file;
} Command;

typedef struct {
    Command commands[MAX_ARGS]; // supports multi-stage pipelines
    int count;                  // number of commands in pipeline
} Pipeline;

Pipeline parse_input(char *input);
void execute_command(Command *cmd);
void execute(Pipeline *pipeline);

int builtin_cd(char **args);
int builtin_help();
int builtin_exit();

void setup_signal_handlers();

int execute_pipeline(Command *cmd1, Command *cmd2);

#endif