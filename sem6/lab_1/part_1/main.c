#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define N       1
#define LEN    10

int main()
{
    int wstatus = 0;
    pid_t childpid[N], w = 0;
    char chmsgs[N][LEN] = { "xxx", "yyy", "zzz" };
    char prmsg[LEN] = { "parent" };
    int fdsock[2];
    char buf[LEN] = { 0 };

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fdsock) == -1) {
        perror("socketpair()");
        exit(EXIT_SUCCESS);
    }

    for (int i = 0; i < N; ++i)
    {
        if ((childpid[i] = fork()) == -1) {
            perror("fork()");
            exit(EXIT_FAILURE);
        } else if (childpid[i] == 0) {
            write(fdsock[1], chmsgs[i], LEN);
            printf("child %d send \"%s\"\n", getpid(), chmsgs[i]);
            read(fdsock[1], buf, LEN);
            printf("child %d recieve \"%s\"\n", getpid(), buf);
            exit(EXIT_SUCCESS);
        } else {
            read(fdsock[0], buf, LEN);
            printf("parent %d recieve \"%s\"\n", getpid(), buf);
            write(fdsock[0], prmsg, LEN);
            printf("parent %d send \"%s\"\n", getpid(), prmsg);
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

    return 0;
}