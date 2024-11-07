#include "multithreading.h"

/**
* tprintf - print a message using printf
* @format: format string
*
* Return: 0
*/
int tprintf(char const *format, ...)
{
	pthread_t tid = pthread_self(); /* get the thread id. See man pthread_self  */

	printf("[%lu] %s", tid, format);
	return (0);
}
