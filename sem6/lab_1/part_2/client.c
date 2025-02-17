#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define NAME     "sock.socket"
#define LEN                 10

int sockfd;

int signal_handler(int sig_numb) {
    close(sockfd);
    printf("PID %d signal %d\n", getpid(), sig_numb);
    exit(EXIT_SUCCESS);
}

int main()
{
    struct sigaction sa = { 0 };
    sa.sa_sigaction = &signal_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction()");
        exit(EXIT_FAILURE);
    }

    char buf[LEN] = { 0 };
    sprintf(buf, "%d", getpid());
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr saddr = { .sa_family = AF_UNIX };
    strcpy(saddr.sa_data, NAME);
    
    while (1) {
        if (sendto(sockfd, buf, strlen(buf), 0, &saddr, sizeof(saddr)) == -1) {
            perror("sendto()");
            exit(EXIT_SUCCESS);
        }
        printf("send: %s\n", buf);

        sleep(1);
    }

    return 0;
}