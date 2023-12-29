/*
 * iotc_device_client.h
 *
 *  Created on: Oct 23, 2023
 *      Author: mgilhespie
 */

#ifndef IOTC_AWSRTOS_SDK_PLACEHOLDER_INCLUDE_IOTC_AWSMQTT_CLIENT_H_
#define IOTC_AWSRTOS_SDK_PLACEHOLDER_INCLUDE_IOTC_AWSMQTT_CLIENT_H_

#include "iotconnect.h"

typedef void (*IotConnectC2dCallback)(char* message, size_t message_len);

typedef struct {
    char *host;    			// Host to connect the client to
    int port;				// MQTT Port to connect to
    char *device_name;   	// Name of the device
	char *sub_topic;
    char *pub_topic;
    IotConnectAuth *auth; 				// Pointer to IoTConnect auth configuration
    IotConnectC2dCallback c2d_msg_cb; 	// callback for inbound messages
    IotConnectStatusCallback status_cb; // callback for connection status
} IotConnectAWSMQTTConfig;


int awsmqtt_client_init(IotConnectAWSMQTTConfig *c, IotConnectAwsrtosConfig* awsrtos_config);
void awsmqtt_client_disconnect(void);
bool awsmqtt_client_is_connected(void);
int awsmqtt_send_message(const char *message);	// send a null terminated string to MQTT host

MQTTStatus_t vSetMQTTConfig( const char *host,
						  int port,
						  const char *device_id,
						  const char *sub_topic,
						  const char *pub_topic,
						  PkiObject_t root_ca_cert,
						  PkiObject_t device_cert,
						  PkiObject_t device_key);



/**
Receive message(s) from IoTHub when a message is received, status_cb is called.

loop_forever if you wish to call this function from own thread.
wait_time will be ignored in this case and set NX_WAIT_FOREVER.

If calling from a single thread (loop_forever = false)
that will be sending and receiving set wait time to the desired value as a multiple of NX_IP_PERIODIC_RATE.
 *
 */
//UINT iothub_c2d_receive(bool loop_forever, ULONG wait_ticks);



#endif /* IOTC_AWSRTOS_SDK_PLACEHOLDER_INCLUDE_IOTC_AWSMQTT_CLIENT_H_ */
