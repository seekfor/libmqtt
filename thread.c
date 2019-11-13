#include "common.h"

#define MAX_STATCK_SIZE				(1024 * 1024)
#define MAX_THREADS				256

#define THREAD_STATE_IDLE			0
#define THREAD_STATE_RUN			1
#define THREAD_STATE_QUIT			2
#define THREAD_STATE_DESTROY			3

typedef struct
{
	int idx;
	volatile unsigned int quit;
	void* wsig;
	int thread_prior;
	void* thread_handle;
	thread_route thread_func;
	void* thread_args;
}thread_pool_t;

static void* mutex = NULL;
static int max_threads = MAX_THREADS,max_statck_size = MAX_STATCK_SIZE;
static thread_pool_t threads[MAX_THREADS];

static void* _thread_create_thread(int prior,thread_route route,void* args,int stack);

static void* common_thread_func(void* args)
{
	thread_pool_t* thread = (thread_pool_t*)args;
	osDefaultSignal();
	while(1)
	{
		osWaitSemaphore(thread->wsig);
		thread->thread_func(thread->thread_args);
		osWaitSemaphore(mutex);
		if(thread->thread_prior & PTHREAD_DETACHED)
		{
			thread->quit = THREAD_STATE_IDLE;
		}
		else
		{
			thread->quit = THREAD_STATE_DESTROY;
		}	
		osPostSemaphore(mutex);
	}
	return args;
}

static void* _thread_create_thread(int prior,thread_route route,void* args,int stack)
{
	pthread_t* ret;
	pthread_attr_t attr;
	struct sched_param thread_param;
	ret = (pthread_t*)osMalloc(sizeof(pthread_t));
	if(ret)
	{
		pthread_attr_init(&attr);
		if(stack)
		{
			pthread_attr_setstacksize(&attr,stack);
		}
		pthread_attr_setschedpolicy(&attr,(prior == 0)?SCHED_OTHER:SCHED_FIFO);
		pthread_attr_getschedparam(&attr, &thread_param);
		thread_param.sched_priority = prior & 0xffff;
		pthread_attr_setschedparam(&attr,&thread_param);
		pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
		if(prior & PTHREAD_DETACHED)
		{
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		}
		if(0 == pthread_create(ret,&attr,route,args))
		{
			pthread_attr_destroy(&attr);
			return (void*)ret;
		}
		pthread_attr_destroy(&attr);
	}
	return ret;
}

void* osCreateThread(int prior,thread_route route,void* args,int stack)
{
	int i = 0;
	void* ret = NULL;
	if(prior & PTHREAD_HIGH)
	{
		i = MAX_THREADS - 10;
	}
	osWaitSemaphore(mutex);
	for(; i < MAX_THREADS; i ++)
	{
		if(threads[i].quit != THREAD_STATE_IDLE)
		{
			continue;
		}
		threads[i].quit = THREAD_STATE_RUN;
		threads[i].thread_prior = prior;
		threads[i].thread_func = route;
		threads[i].thread_args = args;
		osPostSemaphore(threads[i].wsig);
		ret = &threads[i];
		break;
	}
	osPostSemaphore(mutex);
	return ret;
}

void threadInit(int threadnum,int statcksize)
{
	int i;
	max_threads = threadnum?threadnum:MAX_THREADS;
	max_statck_size = statcksize?statcksize:MAX_STATCK_SIZE;
	osDefaultSignal();
	mutex = osCreateSemaphore((char*)"MTX-OS",1);
	for(i = 0; i < max_threads; i ++)
	{
		int prior = (i < (max_threads - 10))?0:10;
		threads[i].idx = i;
		threads[i].quit = THREAD_STATE_IDLE;
		threads[i].wsig = osCreateSemaphore((char*)"MTX-WSIG",0);
		threads[i].thread_func = NULL;
		threads[i].thread_args = NULL;
		threads[i].thread_handle = _thread_create_thread(prior,common_thread_func,&threads[i],max_statck_size);
	}
}

void threadUninit()
{

}

int osThreadIsExit(void* hdl)
{
	int quit;
	thread_pool_t* thread = (thread_pool_t*)hdl;
	if(!hdl)
	{
		return 0;
	}
	osWaitSemaphore(mutex);
	quit = thread->quit;
	osPostSemaphore(mutex);
	return quit != THREAD_STATE_RUN;
}

int osJoinThread(void* hdl)
{
	if(!hdl)
	{
		return -1;
	}
	thread_pool_t* thread = (thread_pool_t*)hdl;
	osWaitSemaphore(mutex);
	switch(thread->quit)
	{
		case THREAD_STATE_IDLE:
		case THREAD_STATE_DESTROY:
			thread->quit = THREAD_STATE_IDLE;
			osPostSemaphore(mutex);
			return 0;
		default:
			break;
	}
	thread->quit = THREAD_STATE_QUIT;
	do
	{
		osPostSemaphore(mutex);
		osSleep(100);
		osWaitSemaphore(mutex);
	}while(thread->quit == THREAD_STATE_QUIT);
	thread->quit = THREAD_STATE_IDLE;
	osPostSemaphore(mutex);
	return 0;
}

int osDestroyThread(void* thread,int wait)
{
	return osJoinThread(thread);
}

int osDefaultSignal()
{
	struct sigaction usr;
	sigset_t signal_mask;
	sigemptyset(&signal_mask);
        sigaddset(&signal_mask, SIGPIPE);
      	pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);

	usr.sa_flags = 0;
	usr.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &usr, NULL);
	return 0;
}

