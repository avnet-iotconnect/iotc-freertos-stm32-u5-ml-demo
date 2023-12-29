/*
 * iotc_awsmqttcore_client.c
 *
 *  Created on: Oct 23, 2023
 *      Author: mgilhespie
 */


#define LOG_LEVEL    LOG_INFO

/* Standard includes. */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "logging_levels.h"
#include "logging.h"

#include "mbedtls_transport.h"

/* MQTT includes */
#include "core_mqtt.h"
#include "core_mqtt_agent.h"
#include "mqtt_agent_task.h"
#include "subscription_manager.h"

/* IoT-Connect includes */
#include "iotconnect.h"
#include "iotconnect_lib.h"
#include "iotconnect_event.h"
#include <iotc_mqtt_client.h>
#include "sys_evt.h"


// @brief 	Format of topic string used to subscribe to incoming messages for this device
#define SUBSCRIBE_TOPIC_FORMAT   "iot/%s/cmd"

// @brief 	Format of topic string used to publish events (e.g. telemetry and acknowledgements for this device
#define PUBLISH_TOPIC_FORMAT	"devices/%s/messages/events/"

// @brief 	Queue size of acknowledgements offloaded to the vMQTTSubscribeTask
#define ACK_MSG_Q_SIZE	5

// @brief 	Length of buffer to hold subscribe topic string containing the device id (thing_name)
#define MQTT_SUBSCRIBE_TOPIC_STR_LEN           	( 256 )

// @brief 	Size of statically allocated buffers for holding payloads.
#define confgPAYLOAD_BUFFER_LENGTH           	( 256 )

// @brief	Size of statically allocated buffers for holding topic names and payloads.
#define MQTT_PUBLISH_MAX_LEN                 ( 1024 )
#define MQTT_PUBLISH_PERIOD_MS               ( 3000 )
#define MQTT_PUBLISH_TOPIC_STR_LEN           ( 256 )
#define MQTT_PUBLISH_BLOCK_TIME_MS           ( 200 )
#define MQTT_PUBLISH_NOTIFICATION_WAIT_MS    ( 1000 )
#define MQTT_NOTIFY_IDX                      ( 1 )
#define MQTT_PUBLISH_QOS                     ( MQTTQoS0 )

// @brief 	Defines the structure to use as the command callback context in this demo.
struct MQTTAgentCommandContext
{
    MQTTStatus_t xReturnStatus;
    TaskHandle_t xTaskToNotify;
    uint32_t ulNotificationValue;
};


typedef struct AWSMQTTContext
{
	int port;
	const char *host;
	const char subTopicString[256];		// MQTT topic to subscribe to for incoming cloud-2-device commands
	const char pubTopicString[256];		// MQTT topcc to publish to for events (telemetry, command acknowledements)
} AWSMQTTContext_t;

// @brief 	The MQTT agent manages the MQTT contexts.  This set the handle to the context used by this demo.
extern MQTTAgentContext_t xGlobalMqttAgentContext;

// @beief Handle to MQTT agent
static MQTTAgentHandle_t xMQTTAgentHandle = NULL;

// @brief 	Handle to message queue of acknowledgements offloaded onto vMQTTSubscribeTask.
static QueueHandle_t mqtt_message_queue = NULL;
static AWSMQTTContext_t xGlobalAWSMQTTContext;


// Prototypes
static void publish_events_task(void * pvParameters);
static void publish_complete_callback(MQTTAgentCommandContext_t * pxCommandContext,
                                      MQTTAgentReturnInfo_t * pxReturnInfo);
static BaseType_t publish_and_wait_for_ack(MQTTAgentHandle_t xAgentHandle,
                                           const char * pcTopic,
                                           const void * pvPublishData,
                                           size_t xPublishDataLen);
static MQTTStatus_t subscribe_to_topic(MQTTQoS_t xQoS, char *pcTopicFilter);
static void incoming_message_callback(void *pvIncomingPublishCallbackContext, MQTTPublishInfo_t *pxPublishInfo);


/* @brief	Initialize the MQTT client and associated tasks for publishing and receiving commands
 *
 * @param	awsmqtt_config,
 * @param 	awsrtos_config,
 *
 * TODO: This could be merged with the Common/app/mqtt/ore mqtt_agent_task.c code.  This may
 * avoid the need for creating the additional iotc_publish_events_task in this file that
 * handles the publishing of messages.
 */
int awsmqtt_client_init(IotConnectAWSMQTTConfig *awsmqtt_config, IotConnectAwsrtosConfig *awsrtos_config)
{
    BaseType_t xResult;
    IotclConfig *iotcl_config;
    MQTTStatus_t xMQTTStatus;
    const char * pcDeviceId;
	const char * pcTelemetryCd;
	int lSubTopicLen = 0;
	int lPubTopicLen = 0;

	LogInfo("awsmqtt_client_init");

    iotcl_config = iotcl_get_config();

    if (iotcl_config == NULL) {
		LogError( "iotcl_config uninitialized." );
		vTaskDelete( NULL );
    }

	pcDeviceId = iotcl_config->device.duid;
	pcTelemetryCd = iotcl_config->telemetry.cd;

    LogInfo("cd = %08x", (uint32_t)iotcl_config->telemetry.cd);
    vTaskDelay(200);

	if (pcDeviceId == NULL) {
		LogError( "Error getting the thing_name setting." );
		vTaskDelete( NULL );
	}

	if (pcTelemetryCd == NULL) {
		LogError( "Error getting the telemetry_cd setting." );
		return -1;
	}

    LogInfo(".. checked for null telemetry and device id");
    vTaskDelay(200);

	lSubTopicLen = snprintf(xGlobalAWSMQTTContext.subTopicString, ( size_t ) MQTT_SUBSCRIBE_TOPIC_STR_LEN, SUBSCRIBE_TOPIC_FORMAT, pcDeviceId);
	if( ( lSubTopicLen <= 0 ) || ( lSubTopicLen > MQTT_SUBSCRIBE_TOPIC_STR_LEN) ) {
		LogError( "Error while constructing subscribe topic string." );
		vTaskDelete( NULL );
	}

	lPubTopicLen = snprintf(xGlobalAWSMQTTContext.pubTopicString, ( size_t ) MQTT_PUBLISH_TOPIC_STR_LEN, PUBLISH_TOPIC_FORMAT, pcDeviceId, pcTelemetryCd);
	if( ( lPubTopicLen <= 0 ) || ( lPubTopicLen > MQTT_PUBLISH_TOPIC_STR_LEN) ) {
		LogError( "Error while constructing ack publsh topic string, len: %d.", lPubTopicLen );
		return -1;
	}


    LogInfo(".. generated sub and pub topic strings");
    vTaskDelay(200);


	xResult = vSetMQTTConfig(awsmqtt_config->host,
							  awsmqtt_config->port,
							  pcDeviceId,
							  xGlobalAWSMQTTContext.subTopicString,
							  xGlobalAWSMQTTContext.pubTopicString,
							  awsmqtt_config->auth->mqtt_root_ca,
							  awsmqtt_config->auth->data.cert_info.device_cert,
							  awsmqtt_config->auth->data.cert_info.device_key);

    LogInfo("called vSetMQTTConfig");
    vTaskDelay(200);

    LogInfo("creating mqtt agent task");
    vTaskDelay(200);


	configASSERT( xResult == MQTTSuccess );

    xResult = xTaskCreate( vMQTTAgentTask, "MQTTAgent", 4096, NULL, 10, NULL );
    configASSERT( xResult == pdTRUE );


	mqtt_message_queue = xQueueCreate(ACK_MSG_Q_SIZE, sizeof(char *));
	if (mqtt_message_queue == NULL) {
		LogError("Failed to create Ack message queue");
		vTaskDelete( NULL );
	}

    vSleepUntilMQTTAgentReady();
    xMQTTAgentHandle = xGetMqttAgentHandle();
    configASSERT( xMQTTAgentHandle != NULL );
    vSleepUntilMQTTAgentConnected();

    LogInfo( ( "MQTT Agent is connected. subscribing to topic" ) );

	xMQTTStatus = subscribe_to_topic(MQTTQoS1, xGlobalAWSMQTTContext.subTopicString);	// Deliver at least once

	if( xMQTTStatus != MQTTSuccess ) {
		LogError( "Failed to subscribe to topic: %s.", xGlobalAWSMQTTContext.subTopicString );
		return -1;
	}

	// FIXME: May not be needed, prvAgentMessageSend
    xResult = xTaskCreate(publish_events_task, "iotc_pub_events_task", 2048, NULL, 5, NULL );

    if (xResult != pdTRUE ) {
		LogError( "Failed to create iotc_pub_events_task");
    	return -1;
    }

	return 0;
}


/* @brief	disconnect from the MQTT server
 *
 */
void awsmqtt_client_disconnect(void)
{
	// TODO
}


/* @brief	Determine if the device is connected to the MQTT server
 *
 */
bool awsmqtt_client_is_connected(void)
{
    if (xIsMqttAgentConnected() == pdTRUE ) {
    	return true;
    } else {
    	return false;
    }
}

/* @brief	Publish a message to the PUBLISH_TOPIC_FORMAT topic
 *
 * @param	message, JSON formatted string to publish on this topic.
 * @return	0 on success, -1 on error
 *
 * This creates a copy of the supplied JSON message string and puts it onto
 * the mqtt_message_queue. The publish_events_task receives this from the queue
 * and performs the actual publishing of the message.  The publish_events_task
 * also frees the message.
 */
int awsmqtt_send_message(const char *message)
{
    int status;
    char *buf;

    if (message == NULL) {
    	return -1;
    }

    buf = malloc(strlen(message) + 1);

    if (!buf) {
        LogError("failed to allocate msg_buf!");
    	return -1;
    }

    strcpy(buf, message);

    status = xQueueSendToBack(mqtt_message_queue, &buf, 10);

    if(status != pdTRUE) {
        free(buf);
        return -1;
    }

    return 0;
}


/*-----------------------------------------------------------*/


/* @brief 	Task that offloads publishing to the ACK_PUBLISH_TOPIC_FORMAT topic
 *
 * @param 	pvParameters, The parameters passed to the task.
 */
static void publish_events_task( void * pvParameters )
{
    ( void ) pvParameters;
    BaseType_t xStatus;
    int ret;
    char *pcMsgBuf;
    size_t lMsgLen;

    while (1) {
        xStatus = xQueueReceive(mqtt_message_queue, &pcMsgBuf, portMAX_DELAY);
        if (xStatus != pdPASS) {
            LogError("[%s] Q recv error (%d)\r\n", __func__, xStatus);
            break;
        }

        if (pcMsgBuf) {
        	lMsgLen = strlen(pcMsgBuf) + 1;

        	ret = publish_and_wait_for_ack(xMQTTAgentHandle,
        								  xGlobalAWSMQTTContext.pubTopicString,
        	                              pcMsgBuf,
        	                              lMsgLen);

            if (!ret) {
                LogError("Sending a message failed\n");
            }

            // allocated in _mqtt_send_to_q()
            free(pcMsgBuf);
        } else {
            LogError("[%s] can't send a NULL user_msg_buf\r\n", __func__);
        }
    }

    vTaskDelete( NULL );
}


/* @brief	Completion routine when a message has been published.
 *
 */
static void publish_complete_callback( MQTTAgentCommandContext_t * pxCommandContext,
                                       MQTTAgentReturnInfo_t * pxReturnInfo )
{
    TaskHandle_t xTaskHandle = ( TaskHandle_t ) pxCommandContext;

    configASSERT( pxReturnInfo != NULL );

    uint32_t ulNotifyValue = pxReturnInfo->returnCode;

    if( xTaskHandle != NULL ) {
        /* Send the context's ulNotificationValue as the notification value so
         * the receiving task can check the value it set in the context matches
         * the value it receives in the notification. */
        ( void ) xTaskNotifyIndexed( xTaskHandle,
                                     MQTT_NOTIFY_IDX,
                                     ulNotifyValue,
                                     eSetValueWithOverwrite );
    }
}


/* @brief	Publish to an MQTT topic and wait for an acknowledgement
 *
 * FIXME: Probably not needed, remove.  MQTT_Publish queues message, does not block,
 * presumably agent task handles it.
 */
static BaseType_t publish_and_wait_for_ack( MQTTAgentHandle_t xAgentHandle,
                                           const char * pcTopic,
                                           const void * pvPublishData,
                                           size_t xPublishDataLen )
{
    MQTTStatus_t xStatus;
    size_t uxTopicLen = 0;

    configASSERT( pcTopic != NULL );
    configASSERT( pvPublishData != NULL );
    configASSERT( xPublishDataLen > 0 );

    uxTopicLen = strnlen( pcTopic, UINT16_MAX );

    MQTTPublishInfo_t xPublishInfo = {
        .qos             = MQTT_PUBLISH_QOS,
        .retain          = 0,
        .dup             = 0,
        .pTopicName      = pcTopic,
        .topicNameLength = ( uint16_t ) uxTopicLen,
        .pPayload        = pvPublishData,
        .payloadLength   = xPublishDataLen
    };

    MQTTAgentCommandInfo_t xCommandParams = {
        .blockTimeMs                 = MQTT_PUBLISH_BLOCK_TIME_MS,
        .cmdCompleteCallback         = publish_complete_callback,
        .pCmdCompleteCallbackContext = ( void * ) xTaskGetCurrentTaskHandle(),
    };

    if (xPublishInfo.qos > MQTTQoS0) {
        xCommandParams.pCmdCompleteCallbackContext = ( void * ) xTaskGetCurrentTaskHandle();
    }

    /* Clear the notification index */
    xTaskNotifyStateClearIndexed( NULL, MQTT_NOTIFY_IDX );

    xStatus = MQTTAgent_Publish( xAgentHandle, &xPublishInfo, &xCommandParams );

    if (xStatus == MQTTSuccess) {
        uint32_t ulNotifyValue = 0;
        BaseType_t xResult = pdFALSE;

        xResult = xTaskNotifyWaitIndexed( MQTT_NOTIFY_IDX,
                                          0xFFFFFFFF,
                                          0xFFFFFFFF,
                                          &ulNotifyValue,
                                          pdMS_TO_TICKS( MQTT_PUBLISH_NOTIFICATION_WAIT_MS ) );

        if( xResult ) {
            xStatus = ( MQTTStatus_t ) ulNotifyValue;
            if( xStatus != MQTTSuccess ) {
                LogError( "MQTT Agent returned error code: %d during publish operation.", xStatus );
                xResult = pdFALSE;
            }
        } else {
            LogError( "Timed out while waiting for publish ACK or Sent event. xTimeout = %d",
            							pdMS_TO_TICKS( MQTT_PUBLISH_NOTIFICATION_WAIT_MS ) );
            xResult = pdFALSE;
        }
    } else {
        LogError( "MQTTAgent_Publish returned error code: %d.", xStatus );
    }

    return( xStatus == MQTTSuccess );
}


/* @brief 	Subscribe to the topic the demo task will also publish to - that
 * results in all outgoing publishes being published back to the task
 * (effectively echoed back).
 *
 * @param[in] xQoS The quality of service (QoS) to use.  Can be zero or one
 * for all MQTT brokers.  Can also be QoS2 if supported by the broker.  AWS IoT
 * does not support QoS2.
 */
static MQTTStatus_t subscribe_to_topic( MQTTQoS_t xQoS, char *pcTopicFilter )
{
    MQTTStatus_t xMQTTStatus;

    /* Loop in case the queue used to communicate with the MQTT agent is full and
     * attempts to post to it time out.  The queue will not become full if the
     * priority of the MQTT agent task is higher than the priority of the task
     * calling this function. */
    do
    {
        xMQTTStatus = MqttAgent_SubscribeSync( xMQTTAgentHandle,
                                               pcTopicFilter,
                                               xQoS,
                                               incoming_message_callback,
                                               NULL );

        if( xMQTTStatus != MQTTSuccess ) {
            LogError( ( "Failed to SUBSCRIBE to topic with error = %u.", xMQTTStatus ) );
        } else {
            LogInfo( ( "Subscribed to topic %s.\n\n", pcTopicFilter ) );
        }
    } while( xMQTTStatus != MQTTSuccess);

    return xMQTTStatus;
}


/* @brief	Internal callback that is called upon receipt of a cloud-to-device message on the subscribed topic
 *
 * Passed into MQTTAgent_Subscribe() as the callback to execute when
 * there is an incoming publish on the topic being subscribed to.  Its
 * implementation just logs information about the incoming publish including
 * the publish messages source topic and payload.
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pvIncomingPublishCallbackContext Context of the initial command.
 * @param[in] pxPublishInfo Deserialized publish.
 */
static void incoming_message_callback( void * pvIncomingPublishCallbackContext, MQTTPublishInfo_t * pxPublishInfo )
{
    static char cTerminatedString[ confgPAYLOAD_BUFFER_LENGTH ];

    ( void ) pvIncomingPublishCallbackContext;

    LogInfo("iotc mqtt client - incoming_message_callback");

    /* Create a message that contains the incoming MQTT payload to the logger,
     * terminating the string first. */
    if( pxPublishInfo->payloadLength < confgPAYLOAD_BUFFER_LENGTH ) {
        memcpy( ( void * ) cTerminatedString, pxPublishInfo->pPayload, pxPublishInfo->payloadLength );
        cTerminatedString[ pxPublishInfo->payloadLength ] = 0x00;
    } else {
        memcpy( ( void * ) cTerminatedString, pxPublishInfo->pPayload, confgPAYLOAD_BUFFER_LENGTH );
        cTerminatedString[ confgPAYLOAD_BUFFER_LENGTH - 1 ] = 0x00;
    }

    if (! iotcl_process_event(cTerminatedString)) {
        LogError ( "Failed to process event message" );
    }
}
