#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/shell.h"

int main() {
    char input[MAX_INPUT];

    setup_signal_handlers();

    while (1) {
        char cwd[1024];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("processforge:%s$ ", cwd);
        } else {
            perror("getcwd");
            printf("processforge$ ");
        }

        fflush(stdout);

        if (fgets(input, MAX_INPUT, stdin) == NULL)
            break;

        // Remove ONLY trailing newline
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        // Optional: debug
        // printf("DEBUG LINE: [%s]\n", input);

        Pipeline p = parse_input(input);

        if (p.count == 2) {
            if (p.commands[0].args[0])
                execute_pipeline(&p.commands[0], &p.commands[1]);
        } else if (p.count == 1) {
            if (p.commands[0].args[0])
                execute_command(&p.commands[0]);
        }
    }

    return 0;
}