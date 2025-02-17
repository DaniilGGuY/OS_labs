#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

#include "calc.h"

#define NAME     "socket.socket"
#define LEN                  20

bool flag = 1;

int signal_handler(int sig_numb) {
    printf("Получен сигнал %d\n", sig_numb);
    flag = 0;
}

char get_operation_char(char op) {
    switch(op) {
        case 'a':
            return '+';
        case 'b':
            return '-';
        case 'c':
            return '*';
        case 'd':
            return '/';
        default:
            return '?';
    }
}

int main()
{
    srand(time(NULL));
    struct sigaction sa = { 0 };
    sa.sa_sigaction = &signal_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction()");
        exit(EXIT_FAILURE);
    }

    char clientname[LEN] = { 0 };
    sprintf(clientname, "%d.%s", getpid(), "socket");

    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr serv_addr = { .sa_family = AF_UNIX };
    strcpy(serv_addr.sa_data, NAME);
    socklen_t serv_socklen = sizeof(serv_addr);

    struct sockaddr client_addr = { .sa_family = AF_UNIX };
    strcpy(client_addr.sa_data, clientname);
    socklen_t client_socklen = sizeof(client_addr);
    
    if (bind(sockfd, &client_addr, sizeof(client_addr)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    while (flag) {
        struct calc_t calc = { 
            ((double)(rand() % 1000)) / 10.0 + 1, 
            ((double)(rand() % 1000)) / 10.0 + 1, 
            get_operation_char(rand() % 4 + 'a') };

        if (sendto(sockfd, &calc, sizeof(calc), 0, &serv_addr, sizeof(serv_addr)) == -1) {
            perror("sendto()");
            exit(EXIT_SUCCESS);
        }
        printf("send: %f %c %f\n", calc.a, calc.op, calc.b);
        
        double res;
        if (recvfrom(sockfd, &res, sizeof(res), 0, &serv_addr, &client_socklen) == -1) {
            perror("recvfrom()");
            exit(EXIT_FAILURE);
        }
        printf("recieve: %f\n", res);

        sleep(4);
    }

    close(sockfd);
    unlink(client_addr.sa_data);

    return 0;
}

