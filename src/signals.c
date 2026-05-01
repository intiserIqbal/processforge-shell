#define _POSIX_C_SOURCE 200809L

#include <stdio.h>  // perror
#include <unistd.h> // getpgid
#include <signal.h> // sigaction
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "../include/jobs.h"
#include "../include/signals.h"
#include "../include/logging.h"   // for enqueue_deferred_log
#include "../include/scheduler.h" // NEW

/*
 * SIGCHLD handler
 * Reaps children asynchronously and updates job states
 */
static void handle_sigchld(int sig)
{
    (void)sig;

    int saved_errno = errno;
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status,
                          WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    {
        pid_t pgid = getpgid(pid);
        if (pgid < 0)
            continue;

        job_t *job = find_job_by_pgid(pgid);
        if (!job)
            continue;

        if (WIFEXITED(status) || WIFSIGNALED(status))
        {
            /* Record exit status */
            if (WIFEXITED(status))
                job->exit_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                job->exit_status = -WTERMSIG(status); // negative means signal

            if (job->active_processes > 0)
                job->active_processes--;

            if (job->active_processes == 0)
            {
                job->state = JOB_DONE;
                /* Defer logging – we are inside signal handler, cannot write to file */
                enqueue_deferred_log(job->pgid);
                /* Remove this finished job from the scheduler */
                scheduler_remove_job(job->pgid);
            }
        }
        else if (WIFSTOPPED(status))
        {
            job->state = JOB_STOPPED;
            /* Do NOT remove from scheduler – it may be resumed later */
        }
        else if (WIFCONTINUED(status))
        {
            job->state = JOB_RUNNING;
            /* Scheduler will handle it via its policy */
        }
    }

    errno = saved_errno;
}

/*
 * Setup signal handlers for the shell
 */
void setup_signal_handlers()
{
    struct sigaction sa;

    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);

    /*
     * SA_RESTART:
     *   Restart interrupted syscalls (important for fgets)
     *
     * DO NOT use SA_NOCLDSTOP:
     *   we WANT stop notifications for job control
     */
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) < 0)
    {
        perror("sigaction");
        _exit(1);
    }
}