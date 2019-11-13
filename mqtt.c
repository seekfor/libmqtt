#include "common.h"



#define MQTT_STATE_INIT			0
#define MQTT_STATE_CONNECTED		1


#define MQTT_BUFFER_SIZE		(128 * 1024)
#define MQTT_TIMEOUT			(3000)


typedef struct
{
	char topic[1024];
	char msg[MQTT_BUFFER_SIZE];
	int size;
}mqtt_msg_t;


typedef struct
{
	int state;
	void* thread;
	void* mutex;
	int fd;

	char hostname[256];
	int port;
	char username[256];
	char password[256];

	char cid[16];
	int tid;
	
	char topics[16][1024];
	int numtopics;
	int qos;

	mqtt_msg_t msgs[32];
	int wr;
	int rd;
}mqtt_t;



static int mqtt_decode_length(unsigned char* buf)
{
	int ret = buf[0] << 8;
	ret |= buf[1];
	return ret;
}

static int mqtt_encode_length(int value,unsigned char* buf)
{
	buf[0] = value >> 8;
	buf[1] = value & 0xff;
	return 2;
}

static int mqtt_decode_remain_length(unsigned char* buf)
{
	int multiplier = 1;
	int value = 0;
	int i;
	for(i = 0; i < 4; i ++,buf ++)
	{
		value += (buf[0] & 0x7f) * multiplier;
		multiplier *= 128;
		if(!(buf[0] & 0x80))
		{
			break;
		}
	}
	return value;
}

static int mqtt_encode_remain_length(int value,char* buf)
{
	int ret = 0;
	do
	{
		*buf = value % 128;
		value = value / 128;
		if(value > 0)
		{
			(*buf) |= 0x80;
		}
		buf ++;
		ret ++;
	}while(value > 0);
	return ret;
}


static int mqtt_decode_utf8(unsigned char** buf,char* msg)
{
	int ret;
	unsigned char* ptr = (unsigned char*)*buf;
	ret = (ptr[0] << 8) | ptr[1];
	if(ret > MQTT_BUFFER_SIZE)
	{
		ret = MQTT_BUFFER_SIZE;
	}
	else if(ret < 0)
	{
		ret = 0;
	}
	memcpy(msg,ptr + 2,ret);
	(*buf) += 2 + ret;
	return ret + 2;
}

static int mqtt_decode_int(unsigned char** buf,int* val)
{
	unsigned char* ptr = (unsigned char*)*buf;
	*val = (ptr[0] << 8) | ptr[1];
	(*buf) += 2;
	return 2;
}

static int mqtt_send_connect(int fd,char* cid,char* username,char* password)
{
	unsigned char buf[32];
	buf[0] = MQTT_CMD_CONNECT << 4;
	buf[1] = 10 + 18;

	/*Protocol Name:MQTT*/
	buf[2] = 0;
	buf[3] = 4;
	memcpy(buf + 4,"MQTT",4);
	/*Protocol Level:4*/	
	buf[8] = 0x04;
	/*Connect flags*/
	buf[9] = 0x02;
	/*Keep alive*/
	buf[10] = 0;
	buf[11] = 30;
	
	/*payload:client id*/	
	buf[12] = 0x00;
	buf[13] = 0x10;
	memcpy(buf + 14,cid,16);
	return netSend(fd,buf,30,1000);
}

static int mqtt_send_disconnect(int fd)
{
	unsigned char buf[4];
	buf[0] = (MQTT_CMD_DISCONNECT << 4);
	buf[1] = 0x00;
	return netSend(fd,buf,2,1000);
}

static int mqtt_send_pingreq(int fd)
{
	unsigned char buf[4];
	buf[0] = MQTT_CMD_PINGREQ << 4;
	buf[1] = 0x00;
	return netSend(fd,buf,2,1000);
}

static int mqtt_send_subscribe(int fd,int tid, char* topic,int size)
{
	int ret;
	unsigned char buf[8];
	buf[0] = (MQTT_CMD_SUBSCRIBE << 4) | 0x02;
	ret = mqtt_encode_remain_length(size + 2,buf + 1);
	netSend(fd,buf,ret + 1,1000);
	buf[0] = tid >> 8;
	buf[1] = tid & 0xff;
	netSend(fd,buf,2,1000);
	return netSend(fd,topic,size,1000);
}

static int mqtt_send_unsubscribe(int fd,int tid,char* topic,int size)
{
	unsigned char buf[4];
	buf[0] = (MQTT_CMD_UNSUBSCRIBE << 4) | 0x02;
	buf[1] = 0x02;
	buf[2] = tid >> 8;
	buf[3] = tid & 0xff;
	netSend(fd,buf,4,1000);
	return netSend(fd,topic,size,1000);
}

static int mqtt_send_puback(int fd,int pid)
{
	unsigned char buf[4];
	buf[0] = (MQTT_CMD_PUBACK << 4);
	buf[1] = 0x02;
	buf[2] = pid >> 8;
	buf[3] = pid & 0xff;
	return netSend(fd,buf,4,1000);
}

static int mqtt_send_pubrec(int fd,int pid)
{
	unsigned char buf[4];
	buf[0] = (MQTT_CMD_PUBREC << 4);
	buf[1] = 0x02;
	buf[2] = pid >> 8;
	buf[3] = pid & 0xff;
	return netSend(fd,buf,4,1000);
}

static int mqtt_send_pubrel(int fd,int pid)
{
	unsigned char buf[4];
	buf[0] = (MQTT_CMD_PUBREL << 4);
	buf[1] = 0x02;
	buf[2] = pid >> 8;
	buf[3] = pid & 0xff;
	return netSend(fd,buf,4,1000);
}

static int mqtt_send_pubcomp(int fd,int pid)
{
	unsigned char buf[4];
	buf[0] = (MQTT_CMD_PUBCOMP << 4);
	buf[1] = 0x02;
	buf[2] = pid >> 8;
	buf[3] = pid & 0xff;
	return netSend(fd,buf,4,1000);
}




static int mqtt_recv(int fd,char* buf,int timeout)
{
	int i;
	int ret;
	int size;
	char* oldbuf = buf;
	size = netReceive(fd,buf,1,timeout);
	if(size != 1)
	{
		return 0;
	}
	buf ++;
	for(i = 0; i < 4; i ++,buf ++)
	{
		size = netReceive(fd,buf,1,timeout);
		if(size != 1)
		{
			return 0;
		}
		if(!(buf[0] & 0x80))
		{
			break;
		}
	}
	
	ret = mqtt_decode_remain_length(oldbuf + 1) + 1; 
	return ret;
}


static void* mqtt_client_thread(void* args)
{
	mqtt_t* mq = (mqtt_t*)args;
	int ip;
	char topic[MQTT_BUFFER_SIZE];
	char msg[MQTT_BUFFER_SIZE];
	int pid = 0;
	char buf[MQTT_BUFFER_SIZE + 8];
	int size;
	int len;
	/*connect to server first!*/
	ip = netGetIPFrom((char*)mq->hostname);
	if(ip == -1)
	{
		return args;
	}
connect_retry:
	if(mq->fd >= 0)
	{
		closesocket(mq->fd);
	}
	mq->fd = netTCPConnectTimeout(ip,mq->port,20);
	if(mq->fd < 0)
	{
		goto connect_retry;
	}
	/*send CONNECT to server*/
	mqtt_send_connect(mq->fd,mq->cid,mq->username,mq->password);

	while(!osThreadIsExit(mq->thread))
	{
		size = mqtt_recv(mq->fd,buf,5000);
		if(size <= 0)
		{
			if(mqtt_send_pingreq(mq->fd) <= 0)
			{
				goto connect_retry;
			}
		}
		else if(size < MQTT_BUFFER_SIZE)
		{
			int ret;
			int qos;
			unsigned char* hdr;
			mqtt_msg_t* que;
			size = size - 1;
			hdr = buf + size;
			if(size == netReceive(mq->fd,buf + size,size,1000))
			{
				switch(buf[0] >> 4)
				{
					case MQTT_CMD_CONNACK:
						mq->state = (buf[3] == 0)?MQTT_STATE_CONNECTED:MQTT_STATE_INIT;
						if((mq->numtopics) && (mq->state == MQTT_STATE_CONNECTED))
						{
							char* topics[16];
							int i;
							for(i = 0; i < mq->numtopics;i ++)
							{
								topics[i] = mq->topics[i];
							}
							mqttSubscribe(mq,topics,mq->numtopics,mq->qos);
						}
						break;
					case MQTT_CMD_PINGRESP:
						break;
					case MQTT_CMD_PUBLISH:
						qos = MQTT_FLAG_QOS(buf[0]);
						memset(topic,0,sizeof(topic));
						memset(msg,0,sizeof(msg));
						ret = mqtt_decode_utf8(&hdr,topic);
						
						if(qos)
						{
							ret += mqtt_decode_int(&hdr,&pid);
						}
						if(size < ret)
						{
							goto connect_retry;
							break;
						}
						len = size - ret;
						if(len > MQTT_BUFFER_SIZE)
						{
							len = MQTT_BUFFER_SIZE;	
						}
						memcpy(msg,hdr,len);
						osWaitSemaphore(mq->mutex);
						que = &mq->msgs[mq->wr];
						que->size = len;
						strncpy(que->topic,topic,MQTT_BUFFER_SIZE);
						strncpy(que->msg,msg,MQTT_BUFFER_SIZE);
						mq->wr = (mq->wr + 1) % 0x1f;
						osPostSemaphore(mq->mutex);
						switch(qos)
						{
							case 1:
								mqtt_send_puback(mq->fd,pid);
								break;
							case 2:
								mqtt_send_pubrec(mq->fd,pid);
								mqtt_send_pubrel(mq->fd,pid);
								mqtt_send_pubcomp(mq->fd,pid);
								break;
							default:
								break;
						}
						break;
					case MQTT_CMD_SUBACK:
						break;
				}
			}
			else
			{
				goto connect_retry;
			}
		}
		else
		{
			goto connect_retry;
		}

	}

	/*send DISCONNECT to server*/
	mqtt_send_disconnect(mq->fd);

	closesocket(mq->fd);
	mq->fd = -1;

	return args;
}



int mqttInit()
{
	return 0;
}
        
int mqttUninit()
{
	return 0;
}

void* mqttConnect(char* hostname,int port,char* username,char* password)
{
	mqtt_t* mq = (mqtt_t*)osMalloc(sizeof(mqtt_t));
	if(!mq)
	{
		return NULL;
	}
	srand(osGetTickCount());
	memset(mq,0,sizeof(mqtt_t));
	strncpy(mq->hostname,hostname,256);	
	mq->fd = -1;
	mq->port = port;
	mq->tid = rand() & 0xffff;
	sprintf(mq->cid,"%08x",rand());
	if(username && password)
	{
		strncpy(mq->username,username,256);
		strncpy(mq->password,password,256);
	}
	mq->mutex = osCreateSemaphore("MTX-MQTT",1);
	mq->thread = osCreateThread(100,mqtt_client_thread,mq,128 * 1024);
	return mq;
}

int mqttDisconnect(void* hdl)
{
	mqtt_t* mq = (mqtt_t*)hdl;
	if(!mq)
	{
		return -1;
	}
	osJoinThread(mq->thread);
	osFree(mq);
	return 0;
}

int mqttWait(void* hdl,int timeout)
{
	mqtt_t* mq = (mqtt_t*)hdl;
	unsigned int now = osGetTickCount();
	while((osGetTickCount() - now) < timeout)
	{
		osSleep(100);
		if(mq->state == MQTT_STATE_CONNECTED)
		{
			return 0;
		}
	}
	return -1;
}

int mqttSubscribe(void* hdl,char* topics[],int num,int qos)
{
	char buf[65536];
	int i;
	int len;
	char* str = buf;
	mqtt_t* mq = (mqtt_t*)hdl;

	if(num > 16)
	{
		num = 16;
	}
		
	mq->numtopics = num;
	mq->qos = qos;

	for(i = 0; i < num; i ++)
	{
		strcpy(mq->topics[i],topics[i]);
	}

	if(mq->state != MQTT_STATE_CONNECTED)
	{
		return 0;
	}

	for(i = 0; i < num; i ++)
	{
		len = strlen(topics[i]);
		str[0] = len >> 8;
		str[1] = len & 0xff;
		memcpy(str + 2,topics[i],len);
		str[2 + len] = qos;
		str += 3 + len;
	}	
	return mqtt_send_subscribe(mq->fd,mq->tid,buf,(unsigned int)str - (unsigned int)buf);
}

int mqttUnsubscribe(void* hdl,char* topics[],int num)
{
	char buf[65536];
	int size;
	int i;
	int len;
	char* str = buf;
	mqtt_t* mq = (mqtt_t*)hdl;

	if(num > 16)
	{
		num = 16;
	}
	
	for(i = 0; i < num; i ++)
	{
		len = strlen(topics[i]);
		str[0] = len >> 8;
		str[1] = len & 0xff;
		memcpy(str + 2,topics[i],len);
		str += 2 + len;
	}	
	return mqtt_send_unsubscribe(mq->fd,mq->tid,buf,(unsigned int)str - (unsigned int)buf);
}

int mqttPublish(void* hdl,char* topic,char* msg,int size,int retain)
{
	mqtt_t* mq = (mqtt_t*)hdl;
	int ret;
	unsigned char buf[8 + 128 * 1024];
	buf[0] = (MQTT_CMD_PUBLISH << 4) | retain;
	ret = size + strlen(topic) + 2;
	ret = mqtt_encode_remain_length(ret,buf + 1);
	netSend(mq->fd,buf,ret + 1,1000);
	
	ret = strlen(topic);
	mqtt_encode_length(ret,buf);
	netSend(mq->fd,buf,2,1000);
	netSend(mq->fd,topic,ret,1000);
	
	return netSend(mq->fd,msg,size,1000);
}




int mqttRecv(void* hdl,char* topic,char* msg)
{
	int ret = 0;
	mqtt_msg_t* que;
	mqtt_t* mq = (mqtt_t*)hdl;
	osWaitSemaphore(mq->mutex);
	if(mq->rd != mq->wr)
	{
		que = &mq->msgs[mq->rd];
		strncpy(topic,que->topic,128 * 1024);
		strncpy(msg,que->msg,128 * 1024);
		ret = que->size;
		mq->rd = (mq->rd + 1) & 0x1f;
	}
	osPostSemaphore(mq->mutex);
	return ret;
}


