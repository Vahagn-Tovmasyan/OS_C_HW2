#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

/* Wait for one child and return its exit code. */
int wait_for_child(pid_t child)
{
    int status;

    while (waitpid(child, &status, 0) == -1) {
        if (errno == EINTR) {
            continue;
        }

        perror("waitpid failed");
        return 1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return 1;
}

int main(void)
{
    /* Create the first child. */
    pid_t first = fork();

    if (first == -1) {
        perror("First fork failed");
        return 1;
    }

    if (first == 0) {
        execl("/bin/ls", "ls", (char *)NULL);

        perror("execl ls failed");
        _exit(127);
    }

    /* Parent waits for ls to finish. */
    int first_status = wait_for_child(first);

    /* Create the second child. */
    pid_t second = fork();

    if (second == -1) {
        perror("Second fork failed");
        return 1;
    }

    if (second == 0) {
        execl("/bin/date", "date", (char *)NULL);

        perror("execl date failed");
        _exit(127);
    }

    /* Parent waits for date to finish. */
    int second_status = wait_for_child(second);

    printf("Parent process done\n");

    if (first_status != 0 || second_status != 0) {
        return 1;
    }

    return 0;
}
