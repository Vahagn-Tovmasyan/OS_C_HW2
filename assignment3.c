#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        /* Only the child enters this block. */
        execl("/bin/echo", "echo", "Hello from the child process",  (char *)NULL);

        /* These lines run only if execl fails. */
        perror("execl failed");
        _exit(127);
    }

    /* Only the parent reaches this section. */
    int status;

    while (waitpid(pid, &status, 0) == -1) {
        if (errno == EINTR) {
            continue;
        }

        perror("waitpid failed");
        return 1;
    }

    printf("Parent process done\n");

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return 1;
}
