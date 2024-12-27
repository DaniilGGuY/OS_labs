#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "pc.h"

void pc_prog(char *host, int type)
{
	CLIENT *clnt;
	char  *result;
	char *arg;

    clnt = clnt_create(host, PC_PROG, PC_VER, "udp");
    if (clnt == NULL) {
        clnt_pcreateerror (host);
        exit (1);
    }

	if (type == 0) {
		while (1) {
			sleep(rand() % 5 + 1);
			result = consume_1((void*)&consume_1_arg, clnt);
			if (result == (char *) NULL)
				clnt_perror (clnt, "call failed");
			else
				printf("Consumer get %c\n", getpid(), *result);
		}
	}
	else {
		while (1) {
			result = produce_1((void*)&produce_1_arg, clnt);
			if (result == (void *) NULL)
				clnt_perror (clnt, "call failed");
		}
	}

    clnt_destroy (clnt);
}

int main (int argc, char *argv[])
{
	char *host;
	int role;

	if (argc < 2) {
		printf ("usage: %s server_host\n", argv[0]);
		exit (1);
	}
	if (argc < 3) {
		printf ("usage: %s client hasn't role\n", argv[0]);
		exit (1);
	}
	role = atoi(argv[2]);
	if (role != 0 && role != 1) {
		printf ("usage: %s wrong client's role\n", argv[0]);
		exit (1);
	}
	host = argv[1];
	srand(time(NULL));

	ps_prog(host, role);

	exit (0);
}
