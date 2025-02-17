#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define N       2

int main()
{
    pid_t childpid[N];
    for (int i = 0; i < N; ++i) {
        if ((childpid[i] = fork()) == -1) {
            perror("Can't fork.\n");
            exit(EXIT_FAILURE);
        } else if (childpid[i] == 0) {
            printf("Child before sleep: pid=%d, ppid=%d, group=%d\n", getpid(), getppid(), getpgrp());
            sleep(2);
            printf("Child after sleep: pid=%d, ppid=%d, group=%d\n", getpid(), getppid(), getpgrp());
            exit(EXIT_SUCCESS);
        } else {
            printf("Parent: pid=%d, ppid=%d, group=%d\n", getpid(), getppid(), getpgrp());
        }
    }
    
    exit(EXIT_SUCCESS);
}