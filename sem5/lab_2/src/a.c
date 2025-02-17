#include <stdio.h>
#include <unistd.h>

#define OK 0
#define ERROR_IO 1
#define ERROR_RANGE 2

//Функция находит НОД двух чисел, которые передаются в качестве аргументов функции (a и b соответственно)
int gcd(int a, int b)
{
	int tmp;

	while (a != b)
	{
		if (a > b)
		{
			tmp = a;
			a = b;
			b = tmp;
		}
		b = b - a;
	}

	return a;
}

int main(void)
{
	int first, second, ans;
    printf("pid=%d: Input two int values:\n", getpid());
	if (scanf("%d %d", &first, &second) != 2)
		return ERROR_IO;
	if (first < 1 || second < 1)
		return ERROR_RANGE;
	ans = gcd(first, second);
	printf("pid=%d: Calculated gcd is %d\n", getpid(), ans);
	return OK;
}