#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>

#include "../include/shell.h"
#include "../include/jobs.h"
#include "../include/signals.h"

/* Global shell state */
pid_t shell_pgid;
int shell_terminal;

int main()
{
    char input[MAX_INPUT];

    shell_terminal = STDIN_FILENO;

    pid_t current_pgid;
    while (tcgetpgrp(shell_terminal) != (current_pgid = getpgrp()))
    {
        kill(-current_pgid, SIGTTIN);
    }

    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0)
    {
        perror("setpgid");
        exit(1);
    }

    if (tcsetpgrp(shell_terminal, shell_pgid) < 0)
    {
        perror("tcsetpgrp");
        exit(1);
    }

    init_jobs();
    setup_signal_handlers();

    int show_prompt = isatty(STDIN_FILENO);

    while (1)
    {
        if (show_prompt)
        {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
                printf("processforge:%s> ", cwd);
            else
                printf("processforge> ");
            fflush(stdout);
        }

        if (fgets(input, MAX_INPUT, stdin) == NULL)
            break;

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n')
            input[len - 1] = '\0';

        char original_command[MAX_INPUT];
        strncpy(original_command, input, MAX_INPUT - 1);
        original_command[MAX_INPUT - 1] = '\0';

        Pipeline p = parse_input(input);

        if (p.count == 1)
            execute_command(&p.commands[0], original_command);
        else if (p.count > 1)
        {
            // Pass original_command to pipeline
            if (execute_pipeline(&p, original_command) < 0)
                fprintf(stderr, "Error: Failed to execute pipeline\n");
        }
    }

    return 0;
}