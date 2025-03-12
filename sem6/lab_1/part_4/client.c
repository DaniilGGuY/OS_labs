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
#include <netinet/in.h>

#include "settings.h"

bool flag = 1;

int signal_handler(int sig_numb) {
    printf("Получен сигнал %d\n", sig_numb);
    flag = 0;
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

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    while (flag) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket()");
            exit(EXIT_FAILURE);
        }
        if (connect(sockfd, &serv_addr, sizeof(serv_addr)) == -1) {
            perror("connect()");
            exit(EXIT_FAILURE);
        }

        int type_req = htonl(READ);
        if (write(sockfd, &type_req, sizeof(type_req)) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        
        char readed[SIZE + 1];
        int size_resp = read(sockfd, readed, sizeof(readed));
        if (size_resp == -1) {
            perror("read()");
            exit(EXIT_FAILURE);
        }

        readed[size_resp] = '\0';
        if (size_resp == 0) {
            printf("Сервер закрыл соединение\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }
        printf("Клиент прочитал массив: %s\n", readed);
        sleep(1);

        int write_ind = -1;
        for (int i = 0; i < SIZE; ++i) {
            if (readed[i] != BUSY) {
                write_ind = i;
                break;
            }
        }
        
        type_req = htonl(WRITE);
        if (write(sockfd, &type_req, sizeof(type_req)) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        if (write(sockfd, &write_ind, sizeof(write_ind)) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }

        int rc;
        size_resp = read(sockfd, &rc, sizeof(rc));
        if (size_resp == -1) {
            perror("read()");
            exit(EXIT_FAILURE);
        } else if (size_resp == 0) {
            printf("Сервер закрыл соединение\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }

        rc = ntohl(rc);
        if (rc == 1) {
            printf("Нет свободных элементов для записи\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        } else if (rc == 2) {
            printf("Элемент %d уже занят\n", write_ind);
        } else {
            char res;
            size_resp = read(sockfd, &res, sizeof(res));
            if (size_resp == -1) {
                perror("read()");
                exit(EXIT_FAILURE);
            } else if (size_resp == 0) {
                printf("Сервер закрыл соединение\n");
                close(sockfd);
                exit(EXIT_SUCCESS);
            }
            printf("Получен элемент с индексом %d: %c\n", write_ind, res);
        }
        close(sockfd);
        sleep(3);
    }

    return 0;
}

