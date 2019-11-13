#ifndef __MQTT_H__
#define __MQTT_H__


#define MQTT_DEFAULT_PORT			1883

#define MQTT_CMD_CONNECT			1
#define MQTT_CMD_CONNACK			2
#define MQTT_CMD_PUBLISH			3
#define MQTT_CMD_PUBACK				4
#define MQTT_CMD_PUBREC				5
#define MQTT_CMD_PUBREL				6
#define MQTT_CMD_PUBCOMP			7
#define MQTT_CMD_SUBSCRIBE			8
#define MQTT_CMD_SUBACK				9
#define MQTT_CMD_UNSUBSCRIBE			10
#define MQTT_CMD_UNSUBACK			11
#define MQTT_CMD_PINGREQ			12
#define MQTT_CMD_PINGRESP			13
#define MQTT_CMD_DISCONNECT			14


#define MQTT_FLAG_RETAIN			(1 << 0)
#define MQTT_FLAG_QOS0				(1 << 1)
#define MQTT_FLAG_QOS1				(1 << 2)
#define MQTT_FLAG_DUP				(1 << 3)

#define MQTT_FLAG_QOS(a)			((a >> 1) & 0x03)



#ifdef __cplusplus
extern "C"
{
#endif

	int mqttInit();
	int mqttUninit();

	void* mqttConnect(char* ip,int port,char* username,char* password);
	int mqttDisconnect(void* hdl);
	
	int mqttWait(void* hdl,int timeout);

	int mqttSubscribe(void* hdl,char* subscribes[],int num,int qos);
	int mqttUnsubscribe(void* hdl,char* subscribes[],int num);
	
	int mqttPublish(void* hdl,char* topic,char* msg,int size,int retain);
	int mqttRecv(void* hdl,char* topic,char* msg);

#ifdef __cplusplus
}
#endif

#endif

