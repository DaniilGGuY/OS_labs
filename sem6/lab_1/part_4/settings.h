#ifndef SETTINGS_H__
#define SETTINGS_H__

#include <sys/sem.h>

#define PORT   9887
#define MSGSIZE  64

#define READERS          5
#define WRITERS          3
#define SLEEP_TIMEW      3
#define SLEEP_TIMER      3
// Семафоры

#define NUMB_OF_READERS   0	
#define WRITERS_QUEUE     1
#define ACTIVE_WRITER     2
#define READERS_QUEUE     3

// Операции
#define P               -1
#define V                1

struct sembuf start_read[5] = { { READERS_QUEUE, V, 0 }, 
								{ WRITERS_QUEUE,  0, 0 }, 
								{ ACTIVE_WRITER,  0, 0 }, 
								{ NUMB_OF_READERS,  V, 0 },
                                { READERS_QUEUE, P, 0} };
struct sembuf stop_read[1]	= { { NUMB_OF_READERS, P, 0 } };  
struct sembuf start_write[5] = { { WRITERS_QUEUE,  V, 0 }, 
								 { NUMB_OF_READERS,  0, 0 }, 
                                 { ACTIVE_WRITER, 0, 0 }, 
								 { WRITERS_QUEUE, P, 0 }, 
								 { ACTIVE_WRITER,  V, 0 } };  
                                     
struct sembuf stop_write[1]  = { { ACTIVE_WRITER,  P, 0 } };

#define READ  1
#define WRITE 2

#define SIZE 26
#define BUSY '*'

#endif