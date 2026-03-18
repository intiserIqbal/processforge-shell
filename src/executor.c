#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include "../include/shell.h"

void apply_redirection(Command *cmd) {
    if (cmd->redirect_in) {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0) {
            perror("input file");
            _exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (cmd->redirect_out) {
        int flags = O_WRONLY | O_CREAT;
        if (cmd->append_out)
            flags |= O_APPEND;
        else
            flags |= O_TRUNC;

        int fd = open(cmd->output_file, flags, 0644);
        if (fd < 0) {
            perror("output file");
            _exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

void execute_command(Command *cmd) {
    if (strcmp(cmd->args[0], "cd") == 0) {
        builtin_cd(cmd->args);
        return;
    }
    if (strcmp(cmd->args[0], "help") == 0) {
        builtin_help();
        return;
    }
    if (strcmp(cmd->args[0], "exit") == 0) {
        builtin_exit();
        return;
    }

    pid_t pid = fork();

    if (pid == 0) {
        apply_redirection(cmd);
        if (!cmd->args[0]) _exit(1);
        execvp(cmd->args[0], cmd->args);
        perror("exec failed");
        _exit(1);
    }

    waitpid(pid, NULL, 0);
}

int execute_pipeline(Command *cmd1, Command *cmd2) {
    int pipefd[2];
    pipe(pipefd);

    pid_t pid1 = fork();

    if (pid1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        apply_redirection(cmd1);
        if (!cmd1->args[0]) _exit(1);
        execvp(cmd1->args[0], cmd1->args);
        perror("exec1");
        _exit(1);
    }

    pid_t pid2 = fork();

    if (pid2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);
        apply_redirection(cmd2);
        if (!cmd2->args[0]) _exit(1);
        execvp(cmd2->args[0], cmd2->args);
        fprintf(stderr, "%s: command not found\n", cmd2->args[0]);
        _exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}