#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <glob.h>

#include "../include/shell.h"
#include "../include/jobs.h"

extern pid_t shell_pgid;

void apply_redirection(Command *cmd)
{
    if (cmd->redirect_in)
    {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0)
        {
            fprintf(stderr, "%s: %s\n", cmd->input_file, strerror(errno));
            _exit(1);
        }
        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2 stdin");
            close(fd);
            _exit(1);
        }
        close(fd);
    }

    if (cmd->redirect_out)
    {
        int flags = O_WRONLY | O_CREAT;
        flags |= cmd->append_out ? O_APPEND : O_TRUNC;

        int fd = open(cmd->output_file, flags, 0644);
        if (fd < 0)
        {
            perror("output file");
            _exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 stdout");
            close(fd);
            _exit(1);
        }
        close(fd);
    }
}

void expand_glob(Command *cmd)
{
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));

    for (int i = 0; cmd->args[i] != NULL; i++)
    {
        glob(cmd->args[i],
             GLOB_NOCHECK | (i > 0 ? GLOB_APPEND : 0),
             NULL,
             &glob_result);
    }

    size_t count = glob_result.gl_pathc;
    if (count >= MAX_ARGS)
        count = MAX_ARGS - 1;

    for (size_t i = 0; i < count; i++)
        cmd->args[i] = strdup(glob_result.gl_pathv[i]);
    cmd->args[count] = NULL;

    globfree(&glob_result);
}

void execute_command(Command *cmd, const char *original_command)
{
    if (!cmd->args[0])
        return;

    if (strcmp(cmd->args[0], "cd") == 0)
    {
        builtin_cd(cmd->args);
        return;
    }
    if (strcmp(cmd->args[0], "help") == 0)
    {
        builtin_help();
        return;
    }
    if (strcmp(cmd->args[0], "exit") == 0)
    {
        builtin_exit();
        return;
    }
    if (strcmp(cmd->args[0], "jobs") == 0)
    {
        builtin_jobs(cmd->args);
        return;
    }
    if (strcmp(cmd->args[0], "fg") == 0)
    {
        builtin_fg(cmd->args);
        return;
    }
    if (strcmp(cmd->args[0], "bg") == 0)
    {
        builtin_bg(cmd->args);
        return;
    }
    if (strcmp(cmd->args[0], "kill") == 0)
    {
        builtin_kill(cmd->args);
        return;
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGPIPE, SIG_DFL);

        setpgid(0, 0);

        expand_glob(cmd);
        apply_redirection(cmd);

        execvp(cmd->args[0], cmd->args);
        perror("exec failed");
        _exit(1);
    }
    else if (pid > 0)
    {
        if (setpgid(pid, pid) < 0 && errno != EACCES && errno != EPERM)
            perror("setpgid");

        if (cmd->background)
        {
            int job_id = add_job(pid, original_command, JOB_RUNNING);
            if (job_id < 0)
                fprintf(stderr, "jobs: job list full\n");
            else
                printf("[%d] %d\n", job_id, pid);
        }
        else
        {
            int status;

            if (tcsetpgrp(STDIN_FILENO, pid) < 0)
                perror("tcsetpgrp");

            waitpid(pid, &status, WUNTRACED);

            if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
                perror("tcsetpgrp");

            if (WIFSTOPPED(status))
            {
                int job_id = add_job(pid, original_command, JOB_STOPPED);
                printf("\n[%d] Stopped %s\n", job_id, original_command);
            }
        }
    }
    else
    {
        perror("fork");
    }
}

int execute_pipeline(Pipeline *pipeline, const char *original_command)
{
    int num_commands = pipeline->count;
    if (num_commands <= 1)
        return -1;

    int (*pipefds)[2] = malloc((num_commands - 1) * sizeof(*pipefds));
    if (!pipefds)
    {
        perror("malloc");
        return -1;
    }

    for (int i = 0; i < num_commands - 1; i++)
    {
        if (pipe(pipefds[i]) < 0)
        {
            perror("pipe");
            for (int j = 0; j < i; j++)
            {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }
            free(pipefds);
            return -1;
        }
    }

    pid_t first_pid = 0;
    int is_background = pipeline->commands[0].background;

    for (int i = 0; i < num_commands; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            for (int j = 0; j < num_commands - 1; j++)
            {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }
            free(pipefds);
            return -1;
        }

        if (pid == 0) // child
        {
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGPIPE, SIG_DFL);

            if (first_pid == 0)
                setpgid(0, 0);
            else
                setpgid(0, first_pid);

            if (i > 0 && dup2(pipefds[i - 1][0], STDIN_FILENO) < 0)
            {
                perror("dup2 stdin");
                _exit(1);
            }
            if (i < num_commands - 1 && dup2(pipefds[i][1], STDOUT_FILENO) < 0)
            {
                perror("dup2 stdout");
                _exit(1);
            }

            for (int j = 0; j < num_commands - 1; j++)
            {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            expand_glob(&pipeline->commands[i]);
            apply_redirection(&pipeline->commands[i]);

            execvp(pipeline->commands[i].args[0], pipeline->commands[i].args);
            perror("execvp");
            _exit(1);
        }
        else // parent
        {
            if (first_pid == 0)
                first_pid = pid;
            if (setpgid(pid, first_pid) < 0 && errno != EPERM && errno != EACCES)
                perror("setpgid");
        }
    }

    for (int i = 0; i < num_commands - 1; i++)
    {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }
    free(pipefds);

    pid_t pgid = first_pid;

    if (is_background)
    {
        int job_id = add_job(pgid, original_command, JOB_RUNNING);
        if (job_id < 0)
            fprintf(stderr, "jobs: job list full\n");
        else
        {
            job_t *job = find_job_by_pgid(pgid);
            if (job)
            {
                job->total_processes = num_commands;
                job->active_processes = num_commands;
            }
            printf("[%d] %d\n", job_id, pgid);
        }
    }
    else // foreground pipeline
    {
        if (tcsetpgrp(STDIN_FILENO, pgid) < 0)
            perror("tcsetpgrp (pipeline)");

        int status;
        int alive = num_commands;

        while (alive > 0)
        {
            pid_t wpid = waitpid(-pgid, &status, WUNTRACED | WCONTINUED);
            if (wpid < 0)
            {
                if (errno == EINTR)
                    continue;
                if (errno == ECHILD) // all children already reaped by SIGCHLD
                    break;
                perror("waitpid");
                break;
            }
            if (WIFEXITED(status) || WIFSIGNALED(status))
                alive--;
            else if (WIFSTOPPED(status))
                break;
        }

        if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
            perror("tcsetpgrp (shell)");

        if (WIFSTOPPED(status))
        {
            int job_id = add_job(pgid, original_command, JOB_STOPPED);
            if (job_id >= 0)
            {
                job_t *job = find_job_by_pgid(pgid);
                if (job)
                {
                    job->total_processes = num_commands;
                    job->active_processes = num_commands;
                }
                printf("\n[%d] Stopped %s\n", job_id, original_command);
            }
        }
    }

    return 0;
}