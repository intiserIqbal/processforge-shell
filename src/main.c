#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // for getcwd()
#include "../include/shell.h"

int main() {

    char input[MAX_INPUT];
    Command cmd;

    setup_signal_handlers();

    while (1) {
        char cwd[1024];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            // --- Option A: Full path ---
             printf("processforge:%s$ ", cwd);
            // <end of Option A>

            // --- Option B: Cleaner prompt (just directory name) ---
            /*char *dir = strrchr(cwd, '/');
            if (dir)
                printf("processforge:%s$ ", dir + 1);
            else
                printf("processforge:%s$ ", cwd);*/
            // <end of Option B>
        } else {
            perror("getcwd");
            printf("processforge$ ");
        }

        fflush(stdout);

        if (fgets(input, MAX_INPUT, stdin) == NULL)
            break;

        input[strcspn(input, "\n")] = 0;

        parse_input(input, &cmd);

        if (cmd.args[0] == NULL)
            continue;

        execute_command(&cmd);
    }

    return 0;
}
