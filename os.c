#include "common.h"



extern int threadInit(int threadnum,int statcksize);

void osInit()
{
	threadInit(0,0);
}

void osUninit()
{
}


unsigned int osGetTickCount()
{
	struct timeval tv;
	unsigned int ret;
	gettimeofday(&tv,NULL);
	ret = tv.tv_sec * 1000ll + tv.tv_usec / 1000;
	return ret;
}

void osSleep(int ms)
{
	usleep(ms * 1000);
}

void* osCreateSemaphore(char* name,int level)
{
	sem_t* ret;
	ret = (sem_t*)osMalloc(sizeof(sem_t) + strlen(name));
	if(ret)
	{
		sem_init(ret,0,level);
	}
	return ret;
}

int osTryWaitSemaphore(void* sem)
{
	if(!sem)
	{
		return -1;
	}
	return sem_trywait((sem_t*)sem);
}

int osWaitSemaphore(void* sem)
{
	if(!sem)
	{
		return -1;
	}
	return sem_wait((sem_t*)sem);
}

int osWaitSemaphoreTimeout(void* sem,int ms)
{
	struct timespec ts;
	if(!sem)
	{
		return -1;
	}
	clock_gettime(CLOCK_REALTIME,&ts);
	ts.tv_sec += ms / 1000;
	ts.tv_nsec += (ms % 1000) * 1000 * 1000;
	return sem_timedwait((sem_t*)sem,&ts);
}

int osPostSemaphore(void* sem)
{
	if(!sem)
	{
		return -1;
	}
	return sem_post((sem_t*)sem);
}

int osDestroySemaphore(void* sem)
{
	if(sem)
	{
		sem_destroy((sem_t*)sem);
		osFree(sem);
	}
	return 0;
}

void* osMalloc(int size)
{
	void* ret;
  	if(size & 0x1f)
	{
		size += 32 - (size & 0x1f);
	}
	ret = malloc(size);
	return ret;
}

void osFree(void* mem)
{
	if(mem)
	{
		free(mem);
	}
}

