#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#define OK 0
#define ERROR_SIZE 1
#define ERROR_IO 2
#define ERROR_RES 3
#define N 10

// Функция ввода значений. Принимает массив и его размер
int input(int a[], size_t *n)
{
    printf("pid=%d: Input size:\n", getpid());
    if (scanf("%zu", n) != 1)
        return ERROR_IO;
    if (*n <= 0 || *n > N)
        return ERROR_SIZE;

    printf("pid=%d: Input elements:\n", getpid());
    for (size_t i = 0; i < *n; ++i)
        if (scanf("%d", &a[i]) != 1)
            return ERROR_IO;

    return OK;
}

// Функция подсчета результата. Принимает массив, его размер и произведение нечетных
int calc_res(const int a[], size_t n, int *res)
{
    *res = 0;
    for (size_t i = 0; i < n; ++i)
    {
        if (a[i] % 2)
        {
            if (*res == 0)
                *res += a[i];
            else
                *res *= a[i];
        }
    }

    if (*res == 0)
        return ERROR_RES;

    return OK;
}

int main(void)
{
    size_t n;
    int res;
    int a[N];
    int rc = input(a, &n);
    
    switch (rc)
    {
        case ERROR_IO:
            printf("pid=%d: Incorrect input\n", getpid());
            return rc;
        case ERROR_SIZE:
            printf("pid=%d: Incorrect size\n", getpid());
            return rc;
    }

    rc = calc_res(a, n, &res);

    switch (rc)
    {
        case ERROR_RES:
            printf("pid=%d: You need to input one more odds value\n", getpid());
            return rc;
        case OK:
            printf("pid=%d: Calculated mult of odds values is %d\n", getpid(), res);
    }
    
    return rc;
}