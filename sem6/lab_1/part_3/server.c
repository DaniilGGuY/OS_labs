#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#include "calc.h"

#define NAME     "server.socket"
#define LEN                  20

bool flag = 1;

int signal_handler(int sig_numb) {
    printf("Получен сигнал %d\n", sig_numb);
    flag = 0;
}

double process(struct calc_t *calc) {
    switch (calc->op) {
        case '+':
            return calc->a + calc->b;
        case '-':
            return calc->a - calc->b;
        case '*':
            return calc->a * calc->b;
        case '/':
            return calc->a / calc->b;
    }
}

int main()
{
    struct sigaction sa = { 0 };
    sa.sa_sigaction = &signal_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction()");
        exit(EXIT_FAILURE);
    }

    char smsg[LEN] = { 0 };
    sprintf(smsg, "server");

    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr serv_addr = { .sa_family = AF_UNIX };
    strcpy(serv_addr.sa_data, NAME);
    socklen_t serv_socklen = sizeof(serv_addr);

    struct sockaddr client_addr = { .sa_family = AF_UNIX };
    socklen_t client_socklen = sizeof(client_addr);

    if (bind(sockfd, &serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    while (flag) {
        struct calc_t calc;
        if (recvfrom(sockfd, &calc, sizeof(calc), 0, &client_addr, &client_socklen) == -1) {
            perror("recvfrom()");
            exit(EXIT_FAILURE);
        } 
        printf("recieve: %f %c %f\n", calc.a, calc.op, calc.b);

        double res = process(&calc);
        sleep(1);

        if (sendto(sockfd, &res, sizeof(res), 0, &client_addr, sizeof(client_addr)) == -1) {
            perror("send()");
            exit(EXIT_FAILURE);
        }
        printf("send: %f\n", res);
    }

    close(sockfd);
    unlink(NAME);

    return 0;
}
