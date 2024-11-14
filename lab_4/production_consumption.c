#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define BUF_SIZE       26
#define SLEEP_TIMEP     2
#define SLEEP_TIMEC     2
#define PRODUCERS       3
#define CONSUMERS       4
// Семафоры
#define SBIN            0
#define SEMPTY          1
#define SFULL           2
// Операции 
#define P              -1
#define V               1

struct sembuf start_prod[2] = { { SEMPTY, P, 0 }, { SBIN, P, 0 } };
struct sembuf stop_prod[2] = { { SBIN, V, 0 }, { SFULL, V, 0 } };
struct sembuf start_cons[2] = { { SFULL, P, 0 }, { SBIN, P, 0 } };
struct sembuf stop_cons[2] = { { SBIN, V, 0 }, { SEMPTY, V, 0} };

void producer(const int shmid, const int semid) {
    char *addr = shmat(shmid, NULL, 0);
    if (addr == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    char **prod_ptr = addr;
    char **cons_ptr = prod_ptr + sizeof(char*);
    char *alpha_ptr = cons_ptr + sizeof(char*);

    srand(getpid());
    while (1) {
        int semop_P = semop(semid, start_prod, 2);
        if (semop_P == -1) {
            perror("semop");
            exit(1);
        }

        **prod_ptr = *alpha_ptr;
        printf("%d put %c\n", getpid(), **prod_ptr);
        ++(*prod_ptr);
        ++(*alpha_ptr);
        if (*alpha_ptr == 'z' + 1)
            *alpha_ptr = 'a';

        sleep(rand() % SLEEP_TIMEP);

        int semop_V = semop(semid, stop_prod, 2);
        if (semop_V == -1) {
            perror("semop");
            exit(1);
        }
    }

    if (shmdt((void *)prod_ptr) == -1) {
        perror("shmdt");
        exit(1);
    }

    exit(0);
}

void consumer(const int shmid, const int semid) {
    char *addr = shmat(shmid, NULL, 0);
    if (addr == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    char **prod_ptr = addr;
    char **cons_ptr = prod_ptr + sizeof(char*);
    char *alpha_ptr = cons_ptr + sizeof(char*);

    srand(getpid());
    while (1) {
        int semop_P = semop(semid, start_cons, 2);
        if (semop_P == -1) {
            perror("semop");
            exit(1);
        }
 
        printf("%d get %c\n", getpid(), **cons_ptr);
        ++(*cons_ptr);

        sleep(rand() % SLEEP_TIMEC);

        int semop_V = semop(semid, stop_cons, 2);
        if (semop_V == -1) {
            perror("semop");
            exit(1);
        }
    }

    if (shmdt((void *)addr) == -1) {
        perror("shmdt");
        exit(1);
    }

    exit(0);
}

int main() {
    int semid, shmid;
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;

    char **prod_ptr;
    char **cons_ptr;
    char *alpha_ptr;

    if ((shmid = shmget(IPC_PRIVATE, (BUF_SIZE + 3) * sizeof(char), IPC_CREAT | perms)) == -1) {
        perror("shmget");
        exit(1);
    }

    char *addr = shmat(shmid, NULL, 0);
    if (addr == (char *)-1) {
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
        if (childpid[i] == 0)
            producer(shmid, semid);
    }

    for (int i = PRODUCERS; i < CONSUMERS + PRODUCERS; ++i)
    {
        if ((childpid[i] = fork()) == -1) {
            perror("fork");
            exit(1);
        }
        if (childpid[i] == 0)
            consumer(shmid, semid);
    }

    for (int i = 0; i < CONSUMERS + PRODUCERS; ++i) {
        int wstatus;
		pid_t w = waitpid(childpid[i], &wstatus, WUNTRACED | WCONTINUED);
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