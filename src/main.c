#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "../include/shell.h"
#include "../include/jobs.h"
#include "../include/signals.h"

int main()
{
    char input[MAX_INPUT];

    init_jobs();
    setup_signal_handlers();

    // Only show prompt if stdin is a terminal (interactive mode)
    int show_prompt = isatty(STDIN_FILENO);

    while (1)
    {
        if (show_prompt)
        {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                printf("processforge:%s> ", cwd);
            }
            else
            {
                printf("processforge> ");
            }
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
        {
            execute_command(&p.commands[0], original_command);
        }
        else if (p.count > 1)
        {
            if (execute_pipeline(&p) < 0)
                fprintf(stderr, "Error: Failed to execute pipeline\n");
        }
    }

    return 0;
}
