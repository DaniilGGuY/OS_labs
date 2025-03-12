#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <sys/stat.h>

#include "settings.h"

bool flag = 1;


int signal_handler(int sig_numb) {
    printf("Получен сигнал %d\n", sig_numb);
    flag = 0;
}


void reader(char *buf, int sockfd, int semid) {
    printf("Получен запрос на чтение\n");
    char ans[SIZE];
    int err = semop(semid, start_read, 5);
    if (err == -1) {
        perror("semop()");
        exit(EXIT_FAILURE);
    }
    memcpy(ans, buf, sizeof(ans));
    err = semop(semid, stop_read, 1);
    if (err == -1) {
        perror("semop\n");
        exit(EXIT_FAILURE);
    }
    if (write(sockfd, &ans, sizeof(ans)) == -1) {
        perror("write()");
        exit(EXIT_FAILURE);
    }
    printf("Запрос на чтение обработан\n\n");
}


void writer(char *buf, int sockfd, int semid, int ppid) {
    int size;
    int write_ind;
    char res = '\0';
    printf("Получен запрос на запись\n");
    if ((size = read(sockfd, &write_ind, sizeof(write_ind))) < 0) {
        perror("read()");
        close(sockfd);
        exit(EXIT_FAILURE);
    } else if (size == 0) {
        printf("Соединение закрыто\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int err = semop(semid, start_write, 5), rc = 0, end = 0;
    if (err == -1) {
        perror("semop\n");
        exit(EXIT_FAILURE);
    }

    if (write_ind < 0 || write_ind >= SIZE) {
        rc = 1;
        end = 1;
    } else if (buf[write_ind] == BUSY)
        rc = 2;
    else {
        res = buf[write_ind];
        buf[write_ind] = BUSY;
    }

    err = semop(semid, stop_write, 1);
    if (err == -1) {
        perror("semop()\n");
        exit(EXIT_FAILURE);
    }

    rc = htonl(rc);
    if (write(sockfd, &rc, sizeof(rc)) == -1)
        perror("write()");
    if (res != '\0') {
        if (write(sockfd, &res, sizeof(res)) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }
    }
    if (end == 1) {
        kill(ppid, SIGINT);
        close(sockfd);
        flag = 0;
        exit(EXIT_SUCCESS);
    }

    printf("Запрос на запись обработан\n\n");
}


void proc(char *buf, int sockfd, int semid, int ppid) {
    int size, type_req;
    while ((size = read(sockfd, &type_req, sizeof(type_req))) > 0) {
        type_req = ntohl(type_req);
        if (type_req == READ) {
            reader(buf, sockfd, semid);
        }
        else if (type_req == WRITE) {
            writer(buf, sockfd, semid, ppid);
        } else {
            printf("Пришел неизвестный запрос\n\n");
            exit(EXIT_FAILURE);
        }
    }

    close(sockfd);
    if (size == -1) {
        perror("read()");
        exit(EXIT_FAILURE);
    } else if (size == 0)
        printf("Соединение закрыто\n");
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

    int semid, shmid;
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    
    if ((shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT | perms)) == -1) {
        perror("shmget()");
        exit(EXIT_FAILURE);
    }

    char *addr = shmat(shmid, NULL, 0);
    if (addr == (char *)-1) {
        perror("shmat()");
        exit(EXIT_FAILURE);
    } 

    memset(addr, ' ', SIZE);
    for (int i = 0; i < SIZE; i++)
        addr[i] = 'a' + i % 26;

    if ((semid = semget(IPC_PRIVATE, 4, perms)) == -1) {
        perror("semget()");
        exit(EXIT_FAILURE);
    }

	int ar_sem = semctl(semid, NUMB_OF_READERS, SETVAL, 0);
    if (ar_sem == -1) {
        perror("semctl()");
        exit(EXIT_FAILURE);
    }
	int w_queue = semctl(semid, WRITERS_QUEUE, SETVAL, 0);
    if (w_queue == -1) {
        perror("semctl()");
        exit(EXIT_FAILURE);
    }
	int aw_sem = semctl(semid, ACTIVE_WRITER, SETVAL, 0);
    if (aw_sem == -1) {
        perror("semctl()");
        exit(EXIT_FAILURE);
    }
    int r_queue = semctl(semid, READERS_QUEUE, SETVAL, 0);
    if (r_queue == -1) {
        perror("semctl()");
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0), connfd;
    if (sockfd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, &serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, SIG_IGN);

    while (flag) {
        if ((connfd = accept(sockfd, NULL, NULL)) == -1) {
            perror("accept()");
            exit(EXIT_FAILURE);
        }
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork()");
            close(connfd);
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            close(sockfd);
            proc(addr, connfd, semid, getppid());
        }
    }
    close(sockfd);

    return 0;
}

