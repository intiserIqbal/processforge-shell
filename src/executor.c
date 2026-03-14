#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "../include/shell.h"

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
    }

    pid_t pid = fork();

    if (pid == 0) {

        execvp(cmd->args[0], cmd->args);
        perror("exec failed");
        exit(1);

    } else {

        if (cmd->background) {
            printf("[background pid %d]\n", pid);
        } else {
            waitpid(pid, NULL, 0);
        }

    }
}

void execute_pipe(Command *cmd1, Command *cmd2) {
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid1 = fork();

    if (pid1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(cmd1->args[0], cmd1->args);
        perror("execvp");
        exit(1);
    }

    pid_t pid2 = fork();

    if (pid2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);
        execvp(cmd2->args[0], cmd2->args);
        perror("execvp");
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void execute_pipeline(Pipeline *pipeline) {
    execute_pipe(&pipeline->left, &pipeline->right);
}

void execute(Pipeline *pipeline) {
    if (pipeline->is_pipe) {
        execute_pipe(&pipeline->left, &pipeline->right);
    } else {
        execute_command(&pipeline->left);
    }
}