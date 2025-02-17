#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define N       2

int main()
{
    int wstatus = 0;
    pid_t childpid[N], w = 0;
    char *execs[N] = { "./a.out", "./b.out" };
    for (int i = 0; i < N; ++i) {
        if ((childpid[i] = fork()) == -1) {
            perror("Can't fork.\n");
            exit(EXIT_FAILURE);
        } else if (childpid[i] == 0) {
            //printf("Child: pid=%d, ppid=%d, group=%d\n", getpid(), getppid(), getpgrp());
            execl(execs[i], NULL);
            exit(EXIT_SUCCESS);
        } else {
            //printf("Parent: pid=%d, ppid=%d, group=%d\n", getpid(), getppid(), getpgrp());
        }
    }

    for (int i = 0; i < N; ++i) {
		w = waitpid(childpid[i], &wstatus, WUNTRACED | WCONTINUED);
        if (w == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        if (WIFEXITED(wstatus)) {
            printf("pid=%d exited, status=%d\n", w, WEXITSTATUS(wstatus));
        } else if (WIFSIGNALED(wstatus)) {
            printf("pid=%d killed by signal %d\n", w, WTERMSIG(wstatus));
        } else if (WIFSTOPPED(wstatus)) {
            printf("pid=%d stopped by signal %d\n", w, WSTOPSIG(wstatus));
        } else if (WIFCONTINUED(wstatus)) {
            printf("pid=%d continued\n", w);
        }
    }

    exit(EXIT_SUCCESS);
}
