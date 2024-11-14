#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/types.h>  
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define READERS          5
#define WRITERS          3
#define SLEEP_TIMEW      3
#define SLEEP_TIMER      3
// Семафоры
#define ACTIVE_READERS   0	
#define WRITERS_QUEUE    1
#define ACTIVE_WRITERS   2
#define READERS_QUEUE    3
// Операции
#define P               -1
#define V                1

struct sembuf start_read[4] = { { READERS_QUEUE, V, 0 }, 
								{ WRITERS_QUEUE,  0, 0 }, 
								{ ACTIVE_WRITERS,  0, 0 }, 
								{ ACTIVE_READERS,  V, 0 } };
struct sembuf stop_read[1]	= { { ACTIVE_READERS, P, 0 } };  
struct sembuf start_write[5] = { { WRITERS_QUEUE,  V, 0 }, 
								 { ACTIVE_WRITERS, 0, 0 }, 
								 { ACTIVE_READERS,  0, 0 }, 
								 { WRITERS_QUEUE, P, 0 }, 
								 { ACTIVE_WRITERS,  V, 0 } };      
struct sembuf stop_write[1]  = { { ACTIVE_WRITERS,  P, 0 } };

void writer(const int shmid, const int semid) {
    char *addr = shmat(shmid, NULL, 0);
    if (addr == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    srand(getpid());
    while (1) {
        sleep(rand() % SLEEP_TIMEW);
        int semop_P = semop(semid, start_write, 5);
        if (semop_P == -1) {
            perror("semop");
            exit(1);
        }

        *addr += 1;
		if (*addr == 'z' + 1)
			*addr = 'a';
        printf("%d write %c\n", getpid(), *addr);

        int semop_V = semop(semid, stop_write, 1);
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

void reader(const int shmid, const int semid) {
    char *addr = shmat(shmid, NULL, 0);
    if (addr == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    srand(getpid());
    while (1) {
        sleep(rand() % SLEEP_TIMER);
        int semop_P = semop(semid, start_read, 4);
        if (semop_P == -1) {
            perror("semop");
            exit(1);
        }
 
        printf("%d read %c\n", getpid(), *addr);

        int semop_V = semop(semid, stop_read, 1);
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

int main(void)
{
	int semid, shmid;
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    
    if ((shmid = shmget(IPC_PRIVATE, sizeof(char), IPC_CREAT | perms)) == -1) {
        perror("shmget");
        exit(1);
    }

    char *addr = shmat(shmid, NULL, 0);
    if (addr == (char *)-1) {
        perror("shmat");
        exit(1);
    } 
    
    if ((semid = semget(IPC_PRIVATE, 4, perms)) == -1) {
        perror("semget");
        exit(1);
    }
	
	*addr = 'a' - 1;

	int ar_sem = semctl(semid, ACTIVE_READERS, SETVAL, 0);
	int w_queue = semctl(semid, WRITERS_QUEUE, SETVAL, 0);
	int aw_sem = semctl(semid, ACTIVE_WRITERS, SETVAL, 0);
    int r_queue = semctl(semid, READERS_QUEUE, SETVAL, 0);
    if (ar_sem == -1 || w_queue == -1 || aw_sem == -1 || r_queue == -1) {
        perror("semctl");
        exit(1);
    }

	pid_t childpid[READERS + WRITERS];
    
    for (int i = 0; i < WRITERS; ++i)
    {
        if ((childpid[i] = fork()) == -1) {
            perror("fork");
            exit(1);
        }
        if (childpid[i] == 0)
            writer(shmid, semid);
    }

    for (int i = WRITERS; i < READERS + WRITERS; ++i)
    {
        if ((childpid[i] = fork()) == -1) {
            perror("fork");
            exit(1);
        }
        if (childpid[i] == 0)
            reader(shmid, semid);
    }

    for (int i = 0; i < READERS + WRITERS; ++i) {
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