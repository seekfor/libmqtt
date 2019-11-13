#ifndef __OS_H__
#define __OS_H__

typedef void* (*thread_route)(void* args);
typedef void* handle_t;

#ifdef __cplusplus
extern "C"
{
#endif
	void osInit();
	void osUninit();
	void osSleep(int ms);
	void* osMalloc(int size);
	void osFree(void* mem);
	unsigned int osGetTickCount();

	int osDefaultSignal();
	void* osCreateThread(int prior,thread_route route,void* args,int stack);
	int osThreadIsExit(void* hdl);
	int osJoinThread(void* thread);
	int osDestroyThread(void* thread,int wait);

	void* osCreateSemaphore(char* name,int level);
	int osTryWaitSemaphore(void* sem);
	int osWaitSemaphore(void* sem);
	int osWaitSemaphoreTimeout(void* sem,int ms);
	int osPostSemaphore(void* sem);
	int osDestroySemaphore(void* sem);


	void* osCreatePIPE(char* name);
	int osReadPIPE(void* pipe,char* buf,int size);
	int osWritePIPE(void* pipe,char* buf,int size);
	int osDestroyPIPE(void* pipe);


	int osModuleDiagnosis(char* buf);


#ifdef __cplusplus
}
#endif


#endif

