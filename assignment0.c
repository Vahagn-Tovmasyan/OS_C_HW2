#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
    pid_t pid;

    pid = fork();

    if (pid ==-1) {
        perror("First fork failed");
        return 1;
    }

    pid = fork();

    if (pid == -1) {
        perror("Second fork failed");
        return 1;
    }

    pid = fork();

    if (pid ==-1) {
        perror("Third fork failed");
        return 1;
    }

    printf("PID=%ld PPID=%ld\n",
           (long)getpid(), (long)getppid());

    fflush(stdout);

    /* Wait until this process has collected all its children. */
    for (;;) {
        pid_t finished = waitpid(-1, NULL, 0);

        if (finished > 0) {
            continue;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == ECHILD) {
            break;
        }

        perror("waitpid failed");
        return 1;
    }

    return 0;
}
