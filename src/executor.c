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