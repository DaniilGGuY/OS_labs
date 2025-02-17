#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define N       2

int flag = 0;

int sig_handler(int signal) 
{
    flag = 1;
    printf("Получен сигнал: %d\n", signal);
}

int main()
{
    int wstatus = 0;
    pid_t childpid[N], w = 0;
    char msgs[N][128] = { "xxxxxxxxxx", "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy" };
    int fd[2];

    printf("Нажмите сочетание клавиш ctrl C, чтобы послать сигнал\n");
    if (pipe(fd) == -1) {
        printf("Error: pipe failed.");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, sig_handler) == -1)
    {
    	perror("Нет сигнала.\n");
    	exit(EXIT_FAILURE);
    }
    
    sleep(5);

    for (int i = 0; i < N; ++i)
    {
        if ((childpid[i] = fork()) == -1) {
            perror("Can't fork.\n");
            exit(EXIT_FAILURE);
        } else if (childpid[i] == 0) {
            //printf("Child: pid=%d, ppid=%d, group=%d\n", getpid(), getppid(), getpgrp());
            close(fd[0]);
            if (flag)
            {
                if (write(fd[1], msgs[i], strlen(msgs[i])) == -1) {
                    printf("Error write");
                    exit(EXIT_FAILURE);
                }
                printf("ped=%d отправил сообщение: %s\n", getpid(), msgs[i]);
            }
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

   
    for (int i = 0; i < N + 1; ++i) 
    {
        char buffer[512] = {0};
        close(fd[1]);
        read(fd[0], buffer, strlen(msgs[i]));
        printf("Полученное сообщение: %s\n", buffer);
    }

    exit(EXIT_SUCCESS);
}
