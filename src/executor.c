#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "../include/shell.h"
#include <glob.h>

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
        dup2(fd, STDIN_FILENO);
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
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

void expand_glob(Command *cmd)
{
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));

    for (int i = 0; cmd->args[i] != NULL; i++)
    {
        glob(cmd->args[i], GLOB_NOCHECK | (i > 0 ? GLOB_APPEND : 0), NULL, &glob_result);
    }

    for (size_t i = 0; i < glob_result.gl_pathc; i++)
    {
        cmd->args[i] = strdup(glob_result.gl_pathv[i]);
    }
    cmd->args[glob_result.gl_pathc] = NULL;

    globfree(&glob_result);
}

void execute_command(Command *cmd)
{
    if (!cmd->args[0])
        return;

    // Builtins
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

    pid_t pid = fork();
    if (pid == 0)
    {
        // Child
        signal(SIGINT, SIG_DFL);
        signal(SIGPIPE, SIG_DFL);

        expand_glob(cmd);
        apply_redirection(cmd);

        execvp(cmd->args[0], cmd->args);
        perror("exec failed");
        _exit(1);
    }
    else if (pid > 0)
    {
        // Parent
        waitpid(pid, NULL, 0);
    }
    else
    {
        perror("fork");
    }
}

int execute_pipeline(Pipeline *pipeline)
{
    int num_commands = pipeline->count;
    int pipefds[2 * (num_commands - 1)];
    pid_t pids[num_commands];

    // Create pipes
    for (int i = 0; i < num_commands - 1; i++)
    {
        if (pipe(pipefds + i * 2) < 0)
        {
            perror("pipe");
            return -1;
        }
    }

    // Fork processes
    for (int i = 0; i < num_commands; i++)
    {
        pids[i] = fork();
        if (pids[i] < 0)
        {
            perror("fork");
            return -1;
        }

        if (pids[i] == 0)
        {
            // Child process
            if (i > 0)
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            if (i < num_commands - 1)
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);

            // Close all pipes
            for (int j = 0; j < 2 * (num_commands - 1); j++)
                close(pipefds[j]);

            expand_glob(&pipeline->commands[i]);
            apply_redirection(&pipeline->commands[i]);

            execvp(pipeline->commands[i].args[0], pipeline->commands[i].args);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    // Close all pipes in parent
    for (int i = 0; i < 2 * (num_commands - 1); i++)
        close(pipefds[i]);

    // Wait for all processes
    int status;
    for (int i = 0; i < num_commands; i++)
        waitpid(pids[i], &status, 0);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
