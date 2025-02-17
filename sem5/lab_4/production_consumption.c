#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#define BUF_SIZE       50
#define SLEEP_TIMEP     2
#define SLEEP_TIMEC     3
#define PRODUCERS       3
#define CONSUMERS       4
// Семафоры
#define SBIN            0
#define SEMPTY          1
#define SFULL           2
// Операции 
#define P              -1
#define V               1

int flag = 1;

struct sembuf start_prod[2] = { { SEMPTY, P, 0 }, { SBIN, P, 0 } };
struct sembuf stop_prod[2] = { { SBIN, V, 0 }, { SFULL, V, 0 } };
struct sembuf start_cons[2] = { { SFULL, P, 0 }, { SBIN, P, 0 } };
struct sembuf stop_cons[2] = { { SBIN, V, 0 }, { SEMPTY, V, 0} };

int signal_handler(int sig_numb) {
    printf("PID %d signal %d\n", getpid(), sig_numb);
    flag = 0;
}

void producer(char *addr, const int semid, char *alpha) {
    char **prod_ptr = addr;
    char **cons_ptr = prod_ptr + sizeof(char*);
    char *alpha_ptr = cons_ptr + sizeof(char*);

    srand(getpid());
    while (flag) {
        sleep(rand() % SLEEP_TIMEP);
        int semop_P = semop(semid, start_prod, 2);
        if (semop_P == -1) {
            printf("PID %d blocked. Errno %d", getpid(), errno);
            perror("semop");
            exit(1);
        }

        **prod_ptr = *alpha_ptr;
        *alpha = **prod_ptr;
        printf("%d put %c\n", getpid(), *alpha);
        ++(*prod_ptr);
        ++(*alpha_ptr);
        if (*alpha_ptr == 'z' + 1)
            *alpha_ptr = 'a';

        int semop_V = semop(semid, stop_prod, 2);
        if (semop_V == -1) {
            printf("PID %d blocked. Errno %d", getpid(), errno);
            perror("semop");
            exit(1);
        }
    }
}

void consumer(char *addr, const int semid) {
    char **prod_ptr = addr;
    char **cons_ptr = prod_ptr + sizeof(char*);
    char *alpha_ptr = cons_ptr + sizeof(char*);

    srand(getpid());
    while (flag) {
        sleep(rand() % SLEEP_TIMEC);
        int semop_P = semop(semid, start_cons, 2);
        if (semop_P == -1) {
            printf("PID %d blocked. Errno %d", getpid(), errno);
            perror("semop");
            exit(1);
        }
 
        printf("%d get %c\n", getpid(), **cons_ptr);
        ++(*cons_ptr);

        int semop_V = semop(semid, stop_cons, 2);
        if (semop_V == -1) {
            printf("PID %d blocked. Errno %d", getpid(), errno);
            perror("semop");
            exit(1);
        }
    }
}

int main() {
    struct sigaction sa = { 0 };
    sa.sa_sigaction = &signal_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    int semid, shmid;
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;

    char **prod_ptr;
    char **cons_ptr;
    char *alpha_ptr;
    char alpha;

    if ((shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT | perms)) == -1) {
        perror("shmget");
        exit(1);
    }

    char *addr = shmat(shmid, NULL, 0);
    if (addr == (void *)-1) {
        perror("shmat");
        exit(1);
    } 
    
    prod_ptr = addr;
    cons_ptr = prod_ptr + sizeof(char*);
    alpha_ptr = cons_ptr + sizeof(char*);
    
    *prod_ptr = alpha_ptr + sizeof(char);
    *cons_ptr = alpha_ptr + sizeof(char);
    *alpha_ptr = 'a';

    if ((semid = semget(IPC_PRIVATE, 3, IPC_CREAT | perms)) == -1) {
        perror("semget");
        exit(1);
    }

    int sbin = semctl(semid, SBIN, SETVAL, 1);
    int sempty = semctl(semid, SEMPTY, SETVAL, BUF_SIZE);
    int sfull = semctl(semid, SFULL, SETVAL, 0);
    if (sbin == -1 || sempty == -1 || sfull == -1) {
        perror("semctl");
        exit(1);
    }

    pid_t childpid[PRODUCERS + CONSUMERS];
    
    for (int i = 0; i < PRODUCERS; ++i)
    {
        if ((childpid[i] = fork()) == -1) {
            perror("fork");
            exit(1);
        }
        if (childpid[i] == 0) {
            producer(addr, semid, &alpha);
            exit(0);
        }
    }

    for (int i = PRODUCERS; i < CONSUMERS + PRODUCERS; ++i)
    {
        if ((childpid[i] = fork()) == -1) {
            perror("fork");
            exit(1);
        }
        if (childpid[i] == 0) {
            consumer(addr, semid);
            exit(0);
        }
    }

    pid_t w = 0;
    int wstatus = 0;
    while (w != -1) {
        w = wait(&wstatus);  
        if (w == -1) {
            if (errno == EINTR) {
                w = 0;
            } else if (errno != ECHILD) {
                perror("wait");
                exit(1);
            }
        } else {
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
    }

    if (shmdt((void *)addr) == -1) {
        perror("shmdt");
        exit(1);
    }

    if (semctl(semid, 1, IPC_RMID, NULL) == -1) {
        perror("semctl");
        exit(1);
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    exit(0);
}