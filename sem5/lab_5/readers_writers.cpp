#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <process.h>

#define READERS        5
#define WRITERS        3
#define SLEEP_TW    3500
#define SLEEP_TR    3000

HANDLE may_read;
HANDLE may_write;

int readers_queue = 0;

char alpha = 'a';

void start_read()
{
    ++readers_queue;
    if (WaitForSingleObject(may_write, 0) == WAIT_OBJECT_0)
        WaitForSingleObject(may_read, INFINITE);
    SetEvent(may_read);
}

void stop_read()
{
    --readers_queue;
    if (readers_queue == 0)
        SetEvent(may_write);
}

void start_write()
{
    if (readers_queue > 0)
        WaitForSingleObject(may_write, INFINITE);
}

void stop_write()
{
    ResetEvent(may_write);
    if (readers_queue > 0)
        SetEvent(may_read);
    else
        SetEvent(may_write);
}

DWORD Reader(PVOID param)
{
    srand(GetCurrentThreadId());
    while (1)
    {
        Sleep(rand() % SLEEP_TR);
        start_read();
        printf("%d read %c\n", GetCurrentThreadId(), alpha);
        stop_read();
    }
    return EXIT_SUCCESS;
}

DWORD Writer(PVOID param)
{
    srand(GetCurrentThreadId());
    while (1)
    {
        Sleep(rand() % SLEEP_TW);
        start_write();
        alpha++;
        if (alpha > 'z') alpha = 'a';
        printf("%d write %c\n", GetCurrentThreadId(), alpha);
        stop_write();
    }
    return EXIT_SUCCESS;
}

int main(void)
{
    DWORD childpid[READERS + WRITERS];
    HANDLE pthread[READERS + WRITERS];  

    if ((may_read = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
        perror("CreateEvent()");
        ExitProcess(EXIT_FAILURE);
    }
    if ((may_write = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
        perror("CreateEvent()");
        ExitProcess(EXIT_FAILURE);
    }

    for (int i = 0; i < WRITERS; ++i)
    {
        pthread[i] = CreateThread(NULL, 0, Writer, NULL, 0, &childpid[i]);
        if (pthread[i] == NULL) {
            perror("CreateThread()");
            ExitProcess(EXIT_FAILURE);
        }
    }

    for (int i = WRITERS; i < READERS + WRITERS; ++i)
    {
        pthread[i] = CreateThread(NULL, 0, Reader, NULL, 0, &childpid[i]);
        if (pthread[i] == NULL) {
            perror("CreateThread()");
            ExitProcess(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < READERS + WRITERS; ++i)
    {
        DWORD wstatus = WaitForSingleObject(pthread[i], INFINITE);

        switch (wstatus)
        {
            case WAIT_OBJECT_0:
                printf("thread %d finished\n", childpid[i]);
                break;
            case WAIT_TIMEOUT:
                printf("timeout %d\n", wstatus);
                break;
            case WAIT_FAILED:
                printf("failed %d\n", wstatus);
                break;
            default:
                printf("unknown end %d\n", wstatus);
                break;
        }
    }

    CloseHandle(may_read);
    CloseHandle(may_write);

    ExitProcess(EXIT_SUCCESS);
}