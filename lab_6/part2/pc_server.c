#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include "pc.h"

#define SIZE_BUF 256

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

static char buf[SIZE_BUF];
static int prod_addr = 0;
static int cons_addr = 0;
static char alpha = 'a';

void *produce_1_svc(void *argp, struct svc_req *rqstp)
{
	static char * result;

    if (semop(semid, start_prod, 2) == -1) {
        perror("semop");
        exit(1);
    }

    *(buf + prod_addr) = alpha;
    ++prod_addr;
    if (alpha == 'z')
        alpha = 'a';
    else
        ++alpha;
    printf("Producer put %c\n", alpha);

    if (semop(semid, stop_prod, 2) == -1) {
        perror("semop");
        exit(1);
    }

	return (void *) &result;
}

char *consume_1_svc(void *argp, struct svc_req *rqstp)
{
	static char result;

    if (semop(semid, start_cons, 2) == -1) {
        perror("semop");
        exit(1);
    }

	result = *(buf + cons_addr);
    printf("Consumer get %c\n", result);
    ++cons_addr;

    if (semop(semid, stop_cons, 2) == -1) {
        perror("semop");
        exit(1);
    }

	return &result;
}
