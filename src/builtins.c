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
    printf("ProcessForge Shell\n");
    printf("Built-in commands: cd, help, exit, jobs, fg, bg, kill, sched\n");
    printf("Pipelines, redirections, globbing, job control supported.\n");
    return 0;
}

int builtin_exit()
{
    printf("exit\n");
    exit(0);
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
    if (job->state != JOB_STOPPED)
    {
        fprintf(stderr, "fg: job %d is not stopped\n", job_id);
        return 1;
    }

    if (tcsetpgrp(STDIN_FILENO, job->pgid) < 0)
    {
        perror("tcsetpgrp");
        return 1;
    }

    if (kill(-job->pgid, SIGCONT) < 0)
    {
        perror("kill(SIGCONT)");
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        return 1;
    }

    job->state = JOB_RUNNING;

    int status;
    pid_t w;
    do
    {
        w = waitpid(-job->pgid, &status, WUNTRACED | WCONTINUED);
        if (w < 0 && errno != EINTR)
        {
            perror("waitpid");
            break;
        }
    } while (w > 0 && WIFCONTINUED(status));

    if (w > 0 && WIFSTOPPED(status))
    {
        job->state = JOB_STOPPED;
    }
    else if (w > 0 && (WIFEXITED(status) || WIFSIGNALED(status)))
    {
        remove_job(job->pgid);
    }

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

    // Send the signal to the whole process group
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

    // For SIGTERM/SIGKILL, we should wait a bit and then try to reap
    if (signum == SIGTERM || signum == SIGKILL)
    {
        // Give the kernel a moment to deliver the signal
        struct timespec req = {0, 50000000}; // 50 ms
        nanosleep(&req, NULL);

        // Reap any dead child processes belonging to this job
        int status;
        pid_t pid;
        int any_reaped = 0;
        while ((pid = waitpid(-job->pgid, &status, WNOHANG)) > 0)
        {
            any_reaped = 1;
            // Decrement active_processes in job (job may have been found already)
            if (job && job->active_processes > 0)
            {
                job->active_processes--;
            }
        }
        if (any_reaped && job && job->active_processes == 0)
        {
            fprintf(stderr, "kill: job %d terminated, cleaning up\n", job_id);
            remove_job(job->pgid);
            return 0;
        }

        // Fallback: force removal if the process group leader is gone
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
    }
    else if (strcmp(args[1], "roundrobin") == 0)
    {
        scheduler_set_policy(SCHED_RR);
    }
    else if (strcmp(args[1], "priority") == 0)
    {
        scheduler_set_policy(SCHED_PRIO);
    }
    else
    {
        fprintf(stderr, "sched: unknown policy '%s'\n", args[1]);
        return 1;
    }
    return 0;
}