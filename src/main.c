#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "../include/shell.h"
#include "../include/jobs.h"
#include "../include/signals.h"
#include "../include/logging.h"
#include "../include/scheduler.h"
#include "../include/logo.h"

pid_t shell_pgid;
int shell_terminal;

/* Timer for scheduler ticks (called every 50 ms) */
static void alarm_handler(int sig)
{
    (void)sig;
    scheduler_tick();
}

char *get_prompt(void);

int main()
{
    shell_terminal = STDIN_FILENO;

    /* Become foreground process group */
    pid_t current_pgid;
    while (tcgetpgrp(shell_terminal) != (current_pgid = getpgrp()))
        kill(-current_pgid, SIGTTIN);

    /* Ignore job control signals – we handle manually */
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    /* Put shell in its own process group */
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
    if (init_logging(getenv("PF_LOG_FILE")) < 0)
        fprintf(stderr, "Warning: logging disabled\n");
    scheduler_init();
    printf("Starting interactive shell...\n\n");
    print_logo();
    printf("\n");

    /* Load command history */
    const char *home = getenv("HOME");
    char history_path[512];
    if (home)
    {
        snprintf(history_path, sizeof(history_path), "%s/.processforge_history", home);
        read_history(history_path);
    }

    /* Set up timer for scheduler ticks (50 ms) */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = alarm_handler;
    sa.sa_flags = SA_RESTART; // restart interrupted readline
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 50000; // first tick after 50 ms
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 50000; // repeat every 50 ms
    setitimer(ITIMER_REAL, &timer, NULL);

    int running = 1;
    while (running)
    {
        /* Process any deferred log entries */
        process_deferred_logs();

        /* Get prompt (dynamic with job count) */
        char *prompt = get_prompt();

        /* Blocking readline – handles arrow keys, history, editing */
        char *line = readline(prompt);
        free(prompt);

        if (!line) // Ctrl-D
        {
            printf("\n");
            break;
        }

        if (strlen(line) > 0)
        {
            add_history(line);
            char original_command[MAX_INPUT];
            strncpy(original_command, line, MAX_INPUT - 1);
            original_command[MAX_INPUT - 1] = '\0';

            Pipeline p = parse_input(line);
            if (p.count == 1)
                execute_command(&p.commands[0], original_command);
            else if (p.count > 1)
                execute_pipeline(&p, original_command);
        }
        free(line);
    }

    /* Save history and clean up */
    if (home)
        write_history(history_path);

    /* Disable timer */
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    return 0;
}

/* Returns a dynamically allocated prompt string – caller must free */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
char *get_prompt(void)
{
    char *prompt = malloc(4096);
    if (!prompt)
        return strdup("$ ");
    int bg_count = 0;
    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].state == JOB_RUNNING || jobs[i].state == JOB_STOPPED)
            bg_count++;
    }
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        strcpy(cwd, "?");
    if (bg_count > 0)
        snprintf(prompt, 4096, "\033[1;31m[%d]\033[0m processforge:%s$ ", bg_count, cwd);
    else
        snprintf(prompt, 4096, "processforge:%s$ ", cwd);
    return prompt;
}
#pragma GCC diagnostic pop