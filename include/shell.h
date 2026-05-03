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

// Parser
Pipeline parse_input(char *input);
void parse_command(char *input, Command *cmd);

// Execution
void execute_command(Command *cmd, const char *original_command);
int execute_pipeline(Pipeline *pipeline, const char *original_command);

// Builtins
int builtin_cd(char **args);
int builtin_help();
int builtin_exit();
int builtin_quit(char **args);
int builtin_jobs(char **args);
int builtin_fg(char **args);
int builtin_bg(char **args);
int builtin_kill(char **args);
int builtin_sched(char **args);
int builtin_menu(char **args);
int builtin_prio(char **args);
int builtin_log(char **args);

// Signals
void setup_signal_handlers();

#endif