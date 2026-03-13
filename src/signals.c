#include <stdio.h>
#include <signal.h>
#include "../include/shell.h"

void sigint_handler(int sig) {
    printf("\nprocessforge> ");
    fflush(stdout);
}

void setup_signal_handlers() {
    signal(SIGINT, sigint_handler);
}