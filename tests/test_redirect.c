#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main() {
    int in_fd = open("input.txt", O_RDONLY);
    if (in_fd < 0) {
        perror("open input.txt");
        exit(1);
    }

    int out_fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) {
        perror("open output.txt");
        close(in_fd);
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(in_fd);
        close(out_fd);
        exit(1);
    }

    if (pid == 0) {
        if (dup2(in_fd, STDIN_FILENO) == -1) {
            perror("dup2 in");
            close(in_fd);
            close(out_fd);
            _exit(1);
        }
        if (dup2(out_fd, STDOUT_FILENO) == -1) {
            perror("dup2 out");
            close(in_fd);
            close(out_fd);
            _exit(1);
        }
        close(in_fd);
        close(out_fd);

        char *args[] = {"cat", NULL};
        execvp("cat", args);
        perror("execvp cat");
        _exit(1);
    }

    close(in_fd);
    close(out_fd);

    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        printf("Child exited with code %d\n", WEXITSTATUS(status));
    }
    return 0;
}