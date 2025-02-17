#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#define NAME     "sock.socket"
#define LEN                 10

int sockfd;
bool flag = 1;
void signal_handler(int signo) {
    printf("Получен сигнал %d\n", signo);
    alarm(10);
    flag = 0;
}

int main()
{
    if (signal(SIGALRM, signal_handler) == -1) {
        perror("signal()");
        exit(EXIT_FAILURE);
    }

    char buf[LEN] = { 0 };
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr saddr = { .sa_family = AF_UNIX };
    strcpy(saddr.sa_data, NAME);
    socklen_t socklen = sizeof(saddr);

    if (bind(sockfd, &saddr, sizeof(saddr)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    alarm(10);
    while (flag) {
        if (recvfrom(sockfd, buf, LEN, 0, &saddr, &socklen) == -1) {
            perror("recvfrom()");
            exit(EXIT_FAILURE);
        } 
        printf("recieve: %s\n", buf);
    }

    close(sockfd);
    unlink(NAME);

    return 0;
}