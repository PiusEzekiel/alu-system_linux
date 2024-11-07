#include "multithreading.h"

/**
* thread_entry - Thread entry point
* @arg: Unused
*
* Return: NULL
*/
void *thread_entry(void *arg)
{
	printf("%s\n", (char *)arg);
	pthread_exit(NULL);
}
