#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "../include/jobs.h"
#include "../include/signals.h"

static void handle_sigchld(int sig)
{
    (void)sig;
    int saved_errno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        job_t *job = find_job_by_pgid(pid);
        if (job)
        {
            if (WIFEXITED(status) || WIFSIGNALED(status))
            {
                update_job_state(pid, JOB_DONE);
            }
        }
    }
    errno = saved_errno;
}

void setup_signal_handlers()
{
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) < 0)
    {
        perror("sigaction SIGCHLD");
        _exit(1);
    }
    // SIGINT can be moved here later
}
