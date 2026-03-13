#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/shell.h"

int builtin_cd(char **args) {

    if (args[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
        return 1;
    }

    if (chdir(args[1]) != 0)
        perror("cd");

    return 0;
}

int builtin_help() {

    printf("ProcessForge Shell\n");
    printf("Built-in commands:\n");
    printf("  cd <dir>\n");
    printf("  help\n");
    printf("  exit\n");

    return 0;
}

int builtin_exit() {
    exit(0);
}