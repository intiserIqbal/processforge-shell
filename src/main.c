#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/select.h>
#include <errno.h>

#include "../include/shell.h"
#include "../include/jobs.h"
#include "../include/signals.h"
#include "../include/logging.h"
#include "../include/scheduler.h"

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

    /* Initialize logging (use env PF_LOG_FILE or default) */
    if (init_logging(getenv("PF_LOG_FILE")) < 0)
        fprintf(stderr, "Warning: logging disabled\n");
    
    /* Initialize scheduler (Day 8) */
    scheduler_init();

    int show_prompt = isatty(STDIN_FILENO);
    int need_prompt = 1; // print prompt initially

    fd_set readfds;
    struct timeval tv;
    int running = 1;

    while (running)
    {
        /* Process any deferred log entries from background jobs */
        process_deferred_logs();

        /* Print prompt only when needed (start of a new command) */
        if (show_prompt && need_prompt)
        {
            int bg_count = 0;
            for (int i = 0; i < job_count; i++)
            {
                if (jobs[i].state == JOB_RUNNING || jobs[i].state == JOB_STOPPED)
                    bg_count++;
            }
            if (bg_count > 0)
                printf("\033[1;31m[%d]\033[0m ", bg_count);
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
                printf("processforge:%s> ", cwd);
            else
                printf("processforge> ");
            fflush(stdout);
            need_prompt = 0;
        }

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 50000; // 50 ms

        int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
            perror("select");
            break;
        }
        else if (ret > 0)
        {
            // Input available – read a line
            if (fgets(input, MAX_INPUT, stdin) == NULL)
                break; // EOF

            size_t len = strlen(input);
            if (len > 0 && input[len - 1] == '\n')
                input[len - 1] = '\0';

            // If the line is empty (just Enter), just print prompt again
            if (strlen(input) == 0)
            {
                need_prompt = 1;
                continue;
            }

            char original_command[MAX_INPUT];
            strncpy(original_command, input, MAX_INPUT - 1);
            original_command[MAX_INPUT - 1] = '\0';

            Pipeline p = parse_input(input);

            if (p.count == 1)
                execute_command(&p.commands[0], original_command);
            else if (p.count > 1)
            {
                if (execute_pipeline(&p, original_command) < 0)
                    fprintf(stderr, "Error: Failed to execute pipeline\n");
            }

            // After executing a command, we need a fresh prompt
            need_prompt = 1;
        }
        else
        {
            // Timeout – scheduler tick (no prompt printed)
            scheduler_tick();
        }
    }

}