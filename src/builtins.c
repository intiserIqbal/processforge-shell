#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>

#include "../include/scheduler.h"
#include "../include/shell.h"
#include "../include/jobs.h"

extern pid_t shell_pgid;

// ========== ORIGINAL BUILTINS ==========
int builtin_cd(char **args)
{
    const char *path = args[1];
    if (path == NULL)
        path = getenv("HOME");
    if (chdir(path) != 0)
        perror("cd");
    return 0;
}

int builtin_help()
{
    printf("Built-in commands:\n");
    printf("  cd <dir>              Change current directory\n");
    printf("  help                  Show this help\n");
    printf("  exit, quit            Exit the shell\n");
    printf("  jobs                  List background jobs\n");
    printf("  fg <%%jobid>           Bring a background job to foreground\n");
    printf("  bg <%%jobid>           Resume a stopped job in background\n");
    printf("  kill <%%jobid>         Terminate a job (send SIGTERM)\n");
    printf("  kill -<sig> <%%jobid>  Send specific signal (e.g., kill -9 %%1)\n");
    printf("  sched [policy]        Show/change scheduler (none, roundrobin, priority)\n");
    printf("  prio <%%jobid> <prio>  Change job priority (-20 highest, 19 lowest)\n");
    printf("  log [-n <N>]          Show last N log entries from job finishes (default 10)\n");
    printf("  menu                  Interactive demo menu\n\n");
    printf("Pipelines, redirections (<, >, >>), globbing, and full job control supported.\n");
    return 0;
}

int builtin_exit()
{
    printf("exit\n");
    exit(0);
}

int builtin_quit(char **args)
{
    (void)args;
    return builtin_exit();
}

int builtin_jobs(char **args)
{
    (void)args;
    print_jobs();
    return 0;
}

// ========== JOB CONTROL BUILTINS ==========
int builtin_fg(char **args)
{
    if (!args[1])
    {
        fprintf(stderr, "fg: usage: fg %%jobid\n");
        return 1;
    }
    if (args[1][0] != '%')
    {
        fprintf(stderr, "fg: job id must start with %%\n");
        return 1;
    }
    int job_id = atoi(args[1] + 1);
    job_t *job = find_job_by_id(job_id);
    if (!job)
    {
        fprintf(stderr, "fg: job %d not found\n", job_id);
        return 1;
    }

    /* Bring job to foreground */
    if (tcsetpgrp(STDIN_FILENO, job->pgid) < 0)
    {
        perror("tcsetpgrp");
        return 1;
    }

    /* If the job is stopped, resume it */
    if (job->state == JOB_STOPPED)
    {
        if (kill(-job->pgid, SIGCONT) < 0)
        {
            perror("kill(SIGCONT)");
            tcsetpgrp(STDIN_FILENO, shell_pgid);
            return 1;
        }
        job->state = JOB_RUNNING;
    }

    /* Wait for the job to finish or stop */
    int status;
    pid_t w;
    do
    {
        w = waitpid(-job->pgid, &status, WUNTRACED);
        if (w < 0 && errno != EINTR)
        {
            perror("waitpid");
            break;
        }
    } while (w > 0 && WIFCONTINUED(status));

    if (w > 0 && WIFSTOPPED(status))
        job->state = JOB_STOPPED;
    else if (w > 0 && (WIFEXITED(status) || WIFSIGNALED(status)))
        remove_job(job->pgid);

    /* Restore shell as foreground process group */
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
        perror("tcsetpgrp restore");

    return 0;
}

int builtin_bg(char **args)
{
    if (!args[1])
    {
        fprintf(stderr, "bg: usage: bg %%jobid\n");
        return 1;
    }
    if (args[1][0] != '%')
    {
        fprintf(stderr, "bg: job id must start with %%\n");
        return 1;
    }
    int job_id = atoi(args[1] + 1);
    job_t *job = find_job_by_id(job_id);
    if (!job)
    {
        fprintf(stderr, "bg: job %d not found\n", job_id);
        return 1;
    }

    if (job->state == JOB_RUNNING)
    {
        printf("bg: job %d is already running in background\n", job_id);
        return 0;
    }

    if (job->state != JOB_STOPPED)
    {
        fprintf(stderr, "bg: job %d is not stopped\n", job_id);
        return 1;
    }

    if (kill(-job->pgid, SIGCONT) < 0)
    {
        perror("kill(SIGCONT)");
        return 1;
    }

    job->state = JOB_RUNNING;
    printf("[%d] %s &\n", job->id, job->command);
    return 0;
}

int builtin_kill(char **args)
{
    if (!args[1])
    {
        fprintf(stderr, "kill: usage: kill %%jobid\n");
        return 1;
    }
    char *endptr;
    long signum = SIGTERM;
    int job_id;
    char *id_str = args[1];

    if (id_str[0] == '-')
    {
        signum = strtol(id_str + 1, &endptr, 10);
        if (*endptr != '\0')
        {
            fprintf(stderr, "kill: invalid signal number\n");
            return 1;
        }
        if (!args[2])
        {
            fprintf(stderr, "kill: missing job id\n");
            return 1;
        }
        id_str = args[2];
    }

    if (id_str[0] != '%')
    {
        fprintf(stderr, "kill: job id must start with %%\n");
        return 1;
    }
    job_id = atoi(id_str + 1);
    job_t *job = find_job_by_id(job_id);
    if (!job)
    {
        fprintf(stderr, "kill: job %d not found\n", job_id);
        return 1;
    }

    if (kill(-job->pgid, signum) < 0)
    {
        if (errno == ESRCH)
        {
            fprintf(stderr, "kill: job %d already terminated (removing from table)\n", job_id);
            remove_job(job->pgid);
            return 0;
        }
        perror("kill");
        return 1;
    }

    printf("Signal %ld sent to job %d (pgid %d)\n", signum, job_id, job->pgid);

    if (signum == SIGTERM || signum == SIGKILL)
    {
        struct timespec req = {0, 50000000}; // 50 ms
        nanosleep(&req, NULL);

        int status;
        pid_t pid;
        int any_reaped = 0;
        while ((pid = waitpid(-job->pgid, &status, WNOHANG)) > 0)
        {
            any_reaped = 1;
            if (job && job->active_processes > 0)
                job->active_processes--;
        }
        if (any_reaped && job && job->active_processes == 0)
        {
            fprintf(stderr, "kill: job %d terminated, cleaning up\n", job_id);
            remove_job(job->pgid);
            return 0;
        }

        if (kill(job->pgid, 0) < 0 && errno == ESRCH)
        {
            fprintf(stderr, "kill: job %d appears dead, removing from table\n", job_id);
            remove_job(job->pgid);
            return 0;
        }
    }
    return 0;
}

// ========== SCHEDULER BUILTIN ==========
int builtin_sched(char **args)
{
    if (args[1] == NULL)
    {
        printf("Scheduler policy: %s\n", sched_get_policy_name());
        return 0;
    }

    if (strcmp(args[1], "none") == 0)
    {
        scheduler_set_policy(SCHED_NONE);
        printf("Scheduler policy changed to 'none' (no preemption).\n");
    }
    else if (strcmp(args[1], "roundrobin") == 0)
    {
        scheduler_set_policy(SCHED_RR);
        printf("Scheduler policy changed to 'roundrobin' (50 ms timeslices).\n");
    }
    else if (strcmp(args[1], "priority") == 0)
    {
        scheduler_set_policy(SCHED_PRIO);
        printf("Scheduler policy changed to 'priority' (static priority).\n");
    }
    else
    {
        fprintf(stderr, "sched: unknown policy '%s'\n", args[1]);
        return 1;
    }
    return 0;
}

// ========== LOG BUILTINS ==========

/*
 * Convert a 24-hour timestamp string "YYYY-MM-DD HH:MM:SS"
 * into 12-hour format "YYYY-MM-DD HH:MM:SS AM/PM".
 * Output written into `out` (must be at least 32 bytes).
 */
static void ts_to_12h(const char *ts, char *out, size_t out_sz)
{
    int year, mon, day, hour, min, sec;
    if (sscanf(ts, "%d-%d-%d %d:%d:%d",
               &year, &mon, &day, &hour, &min, &sec) != 6)
    {
        /* Fallback: copy as-is */
        snprintf(out, out_sz, "%s", ts);
        return;
    }
    const char *ampm = (hour >= 12) ? "PM" : "AM";
    int h12 = hour % 12;
    if (h12 == 0)
        h12 = 12;
    snprintf(out, out_sz, "%04d-%02d-%02d %02d:%02d:%02d %s",
             year, mon, day, h12, min, sec, ampm);
}

/* Pretty-print one CSV log line inside the box */
/* Pretty-print one CSV log line inside the box */
static void print_log_entry(const char *line)
{
    char ts_raw[32], ts_fmt[36], cmd[128];
    int c1, c2, c3, pid, exit_code;

    if (sscanf(line, "%31[^,],%d,%d,%d,%d,\"%127[^\"]\",%d",
               ts_raw, &c1, &c2, &c3, &pid, cmd, &exit_code) != 7)
    {
        /* Fallback: raw line */
        printf("\033[1;36m  │\033[0m  \033[0;37m%s\033[0m\n", line);
        return;
    }

    ts_to_12h(ts_raw, ts_fmt, sizeof(ts_fmt));

    const char *status_color = (exit_code == 0) ? "\033[0;32m" : "\033[0;31m";
    const char *status_label = (exit_code == 0) ? " OK " : "ERR ";

    /* Always truncate command to 28 characters to fit in 30‑char column */
    char cmd_display[32];
    snprintf(cmd_display, sizeof(cmd_display), "%.28s", cmd);

    printf("\033[1;36m  ║\033[0m  "
           "\033[0;33m%-25s\033[0m" /* timestamp (25 chars) */
           "\033[0;37m%-30s\033[0m" /* command   (30 chars) */
           "%s[%s]\033[0m"          /* status              */
           "\n",
           ts_fmt, cmd_display, status_color, status_label);
}

/* LOG built-in — show last N lines of /tmp/processforge.log */
int builtin_log(char **args)
{
    int n = 10;
    if (args[1] && args[2] && strcmp(args[1], "-n") == 0)
    {
        n = atoi(args[2]);
        if (n <= 0)
            n = 10;
    }

    const char *logpath = "/tmp/processforge.log";
    FILE *fp = fopen(logpath, "r");
    if (!fp)
    {
        fprintf(stderr, "log: cannot open %s: %s\n", logpath, strerror(errno));
        return 1;
    }

    /* Circular buffer to keep last N lines */
    char **lines = malloc(n * sizeof(char *));
    if (!lines)
    {
        fclose(fp);
        fprintf(stderr, "log: malloc failed\n");
        return 1;
    }
    for (int i = 0; i < n; i++)
        lines[i] = NULL;

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    int idx = 0, total = 0;

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';
        free(lines[idx]);
        lines[idx] = strdup(line);
        idx = (idx + 1) % n;
        total++;
    }
    free(line);
    fclose(fp);

    if (total == 0)
    {
        printf("  No log entries yet.\n");
        free(lines);
        return 0;
    }

    /* ── Header ── */
    printf("\033[1;36m  ╔═══════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m  ║\033[0m\033[1;37m%-25s%-30s%-8s\033[0m\033[1;36m║\033[0m\n",
        "Timestamp", "Command", "Status");
    printf("\033[1;36m  ╠═══════════════════════════════════════════════════════════════╣\033[0m\n");

    /* Print entries oldest-first */
    int start = (total < n) ? 0 : idx;
    int count = (total < n) ? total : n;
    for (int i = 0; i < count; i++)
    {
        int cur = (start + i) % n;
        if (lines[cur])
            print_log_entry(lines[cur]);
    }

    /* ── Footer ── */
    printf("\033[1;36m  ╚═══════════════════════════════════════════════════════════════╝\033[0m\n");

    for (int i = 0; i < n; i++)
        free(lines[i]);
    free(lines);
    return 0;
}

// ========== MENU BUILTIN ==========
int builtin_menu(char **args)
{
    (void)args;
    int choice;
    char input_buf[16];
    char cmd[256];

    while (1)
    {
        printf("\033[2J\033[H"); /* clear screen */
        printf("\n");
        printf("\033[1;36m  ╔════════════════════════════════════════════╗\033[0m\n");
        printf("\033[1;36m  ║\033[0m\033[1;37m    ProcessForge  —  Demo Menu              \033[1;36m║\033[0m\n");
        printf("\033[1;36m  ╠════════════════════════════════════════════╣\033[0m\n");
        printf("\033[1;36m  ║\033[0m  \033[0;36m[1]\033[0m  Run pipeline    \033[0;37mls | grep .c | wc -l\033[0m \033[1;36m║\033[0m\n");
        printf("\033[1;36m  ║\033[0m  \033[0;36m[2]\033[0m  Background job  \033[0;37msleep 30 &\033[0m           \033[1;36m║\033[0m\n");
        printf("\033[1;36m  ║\033[0m  \033[0;36m[3]\033[0m  List jobs       \033[0;37mjobs\033[0m                 \033[1;36m║\033[0m\n");
        printf("\033[1;36m  ║\033[0m  \033[0;36m[4]\033[0m  Scheduler       \033[0;37msched priority\033[0m       \033[1;36m║\033[0m\n");
        printf("\033[1;36m  ║\033[0m  \033[0;36m[5]\033[0m  View logs       \033[0;37mlast 5 entries\033[0m       \033[1;36m║\033[0m\n");
        printf("\033[1;36m  ╠════════════════════════════════════════════╣\033[0m\n");
        printf("\033[1;36m  ║\033[0m  \033[0;31m[0]\033[0m  Exit menu                            \033[1;36m║\033[0m\n");
        printf("\033[1;36m  ╚════════════════════════════════════════════╝\033[0m\n");
        printf("\n\033[1;37m  Enter choice (0-5): \033[0m");
        fflush(stdout);

        if (!fgets(input_buf, sizeof(input_buf), stdin))
            break;
        choice = atoi(input_buf);

        if (choice == 0)
        {
            printf("\n\033[1;36m  Goodbye.\033[0m\n\n");
            return 0;
        }

        if (choice < 1 || choice > 5)
        {
            printf("\n\033[1;31m  [!] Invalid choice. Please enter 0-5.\033[0m\n");
            printf("\n\033[0;37m  Press Enter to continue...\033[0m");
            fflush(stdout);
            getchar();
            continue;
        }

        /* Output section label */
        const char *labels[] = {
            "",
            "Pipeline  —  ls | grep .c | wc -l",
            "Background Job  —  sleep 30 &",
            "Job List  —  jobs",
            "Scheduler  —  sched priority",
            "Logs  —  last 5 entries",
        };
        printf("\n\033[1;36m  ┌─ %-43s\033[0m\n", labels[choice]);
        printf("\033[1;36m  │\033[0m\n");

        switch (choice)
        {
        case 1:
            strcpy(cmd, "ls | grep .c | wc -l");
            break;
        case 2:
            strcpy(cmd, "sleep 30 &");
            break;
        case 3:
            strcpy(cmd, "jobs");
            break;
        case 4:
            strcpy(cmd, "sched priority");
            break;
        case 5:
        {
            char *log_args[] = {"log", "-n", "5", NULL};
            builtin_log(log_args);
            printf("\n\033[1;36m  └─────────────────────────────────────────────┘\033[0m\n");
            printf("\n\033[0;37m  Press Enter to continue...\033[0m");
            fflush(stdout);
            getchar();
            continue;
        }
        }

        Pipeline p = parse_input(cmd);
        if (p.count == 1)
            execute_command(&p.commands[0], cmd);
        else if (p.count > 1)
            execute_pipeline(&p, cmd);

        printf("\n\033[1;36m  └─────────────────────────────────────────────┘\033[0m\n");
        printf("\n\033[0;37m  Press Enter to continue...\033[0m");
        fflush(stdout);
        getchar();
    }
    return 0;
}

// ========== PRIO BUILTIN ==========
int builtin_prio(char **args)
{
    if (!args[1] || !args[2])
    {
        fprintf(stderr, "usage: prio %%jobid <priority>\n");
        return 1;
    }
    if (args[1][0] != '%')
    {
        fprintf(stderr, "prio: job id must start with %%\n");
        return 1;
    }
    int job_id = atoi(args[1] + 1);
    job_t *job = find_job_by_id(job_id);
    if (!job)
    {
        fprintf(stderr, "prio: job %d not found\n", job_id);
        return 1;
    }

    char *endptr;
    long new_prio = strtol(args[2], &endptr, 10);
    if (*endptr != '\0')
    {
        fprintf(stderr, "prio: invalid priority number\n");
        return 1;
    }
    if (new_prio < -20 || new_prio > 19)
    {
        fprintf(stderr, "prio: priority must be between -20 (highest) and 19 (lowest)\n");
        return 1;
    }

    job->priority = (int)new_prio;
    printf("Job %d (pgid %d) priority set to %d\n", job->id, job->pgid, job->priority);

    if (scheduler_get_policy() != SCHED_PRIO)
        printf("Note: scheduler policy is not 'priority'. Use 'sched priority' to activate.\n");

    return 0;
}