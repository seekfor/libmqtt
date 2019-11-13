#include "common.h"

static char topic[128 * 1024];
static char msg[128 * 1024];
int main(int argc,char* argv[])
{
	void* mq;
	char* topics[] = 
	{
		argv[1],
	};
	osInit();
	threadInit(0,0);
	mqttInit();
	mq = mqttConnect("m2m.eclipse.org",1883,NULL,NULL);
	printf("mq = %p\n",mq);
	mqttWait(mq,20000);
	mqttSubscribe(mq,topics,1,0);
	while(1)
	{
		int size = mqttRecv(mq,topic,msg);
		if(size > 0)
		{
			printf("topic = %s:msg = %s\n",topic,msg);
		}
	};
	mqttDisconnect(mq);
	mqttUninit();
	return 0;
}
