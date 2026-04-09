#ifndef SHELL_H
#define SHELL_H

#define MAX_INPUT 1024
#define MAX_ARGS 64     // max arguments per command
#define MAX_COMMANDS 64 // max commands per pipeline

typedef struct
{
    char *args[MAX_ARGS];
    int background;

    int redirect_in;
    int redirect_out;
    int append_out;
    char *input_file;
    char *output_file;
} Command;

typedef struct
{
    Command commands[MAX_COMMANDS];
    int count; // number of commands in the pipeline
} Pipeline;

Pipeline parse_input(char *input);
void parse_command(char *input, Command *cmd);
void execute_command(Command *cmd, const char *original_command);

int builtin_cd(char **args);
int builtin_help();
int builtin_exit();
int builtin_jobs(char **args);

void setup_signal_handlers();

int execute_pipeline(Pipeline *pipeline);

#endif
