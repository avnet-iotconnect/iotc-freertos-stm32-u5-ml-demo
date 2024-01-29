/*
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Derived from simple_sub_pub_demo.c
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */
#include <time.h>

#include "logging_levels.h"
/* define LOG_LEVEL here if you want to modify the logging level from the default */

#define LOG_LEVEL LOG_DEBUG

#include "logging.h"

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "kvstore.h"

/* MQTT library includes. */
#include "core_mqtt.h"
#include "core_mqtt_agent.h"
#include "subscription_manager.h"
#include "mqtt_agent_task.h"
#include "sys_evt.h"

/* Subscription manager header include. */
#include "subscription_manager.h"

/* Sensor includes */
#include "b_u585i_iot02a_audio.h"

/* Preprocessing includes */
#include "preproc_dpu.h"

/* AI includes */
#include "ai_dpu.h"

extern UBaseType_t uxRand(void);

#define MQTT_PUBLISH_MAX_LEN (512)
#define MQTT_PUBLISH_TIME_BETWEEN_MS (5000)
#define MQTT_PUBLISH_TOPIC "mic_sensor_data"
#define MQTT_PUBLICH_TOPIC_STR_LEN (256)
#define IOTC_CD_MAX_LEN (10)
#define MQTT_PUBLISH_BLOCK_TIME_MS (1000)
#define MQTT_PUBLISH_NOTIFICATION_WAIT_MS (1000)

#define MQTT_NOTIFY_IDX (1)
#define MQTT_PUBLISH_QOS (MQTTQoS0)

#define MIC_EVT_DMA_HALF (1 << 0)
#define MIC_EVT_DMA_CPLT (1 << 1)

#define APP_VERSION "1.4.0"

/**
 * @brief Defines the structure to use as the command callback context in this
 * demo.
 */
struct MQTTAgentCommandContext
{
	MQTTStatus_t xReturnStatus;
	TaskHandle_t xTaskToNotify;
};

/* Private variables ---------------------------------------------------------*/
static uint8_t pucAudioBuff[AUDIO_BUFF_SIZE];
static int8_t pcSpectroGram[CTRL_X_CUBE_AI_SPECTROGRAM_COL * CTRL_X_CUBE_AI_SPECTROGRAM_NMEL];
static float32_t pfAIOutput[AI_NETWORK_OUT_1_SIZE];

/**
 * Specifies the labels for the classes of the demo.
 */
static const char *sAiClassLabels[CTRL_X_CUBE_AI_MODE_CLASS_NUMBER] = CTRL_X_CUBE_AI_MODE_CLASS_LIST;
/**
 * DPUs context
 */
static AudioProcCtx_t xAudioProcCtx;
static AIProcCtx_t xAIProcCtx;
/**
 * Microphone task handle
 */
static TaskHandle_t xMicTask;

static TickType_t last_detection_time;
static int confidence_threshold = 42;
static int inactivity_timeout = 5000;
static int confidence_offsets[AI_NETWORK_OUT_1_SIZE] = {0, -49, -19, 39, 27, -25};

/*-----------------------------------------------------------*/
static void prvPublishCommandCallback(MQTTAgentCommandContext_t *pxCommandContext,
									  MQTTAgentReturnInfo_t *pxReturnInfo)
{
	configASSERT(pxCommandContext != NULL);
	configASSERT(pxReturnInfo != NULL);

	pxCommandContext->xReturnStatus = pxReturnInfo->returnCode;

	if (pxCommandContext->xTaskToNotify != NULL)
	{
		/* Send the context's ulNotificationValue as the notification value so
		 * the receiving task can check the value it set in the context matches
		 * the value it receives in the notification. */
		(void)xTaskNotifyGiveIndexed(pxCommandContext->xTaskToNotify,
									 MQTT_NOTIFY_IDX);
	}
}

/*-----------------------------------------------------------*/

static BaseType_t prvPublishAndWaitForAck(MQTTAgentHandle_t xAgentHandle,
										  const char *pcTopic,
										  const void *pvPublishData,
										  size_t xPublishDataLen)
{
	BaseType_t xResult = pdFALSE;
	MQTTStatus_t xStatus;

	configASSERT(pcTopic != NULL);
	configASSERT(pvPublishData != NULL);
	configASSERT(xPublishDataLen > 0);

	MQTTPublishInfo_t xPublishInfo =
		{
			.qos = MQTT_PUBLISH_QOS,
			.retain = 0,
			.dup = 0,
			.pTopicName = pcTopic,
			.topicNameLength = (uint16_t)strlen(pcTopic),
			.pPayload = pvPublishData,
			.payloadLength = xPublishDataLen};

	MQTTAgentCommandContext_t xCommandContext =
		{
			.xTaskToNotify = xTaskGetCurrentTaskHandle(),
			.xReturnStatus = MQTTIllegalState,
		};

	MQTTAgentCommandInfo_t xCommandParams =
		{
			.blockTimeMs = MQTT_PUBLISH_BLOCK_TIME_MS,
			.cmdCompleteCallback = prvPublishCommandCallback,
			.pCmdCompleteCallbackContext = &xCommandContext,
		};

	/* Clear the notification index */
	xTaskNotifyStateClearIndexed(NULL, MQTT_NOTIFY_IDX);

	xStatus = MQTTAgent_Publish(xAgentHandle,
								&xPublishInfo,
								&xCommandParams);

	if (xStatus == MQTTSuccess)
	{
		xResult = ulTaskNotifyTakeIndexed(MQTT_NOTIFY_IDX,
										  pdTRUE,
										  pdMS_TO_TICKS(MQTT_PUBLISH_NOTIFICATION_WAIT_MS));

		if (xResult == 0)
		{
			LogError("Timed out while waiting for publish ACK or Sent event. xTimeout = %d",
					 pdMS_TO_TICKS(MQTT_PUBLISH_NOTIFICATION_WAIT_MS));
			xResult = pdFALSE;
		}
		else if (xCommandContext.xReturnStatus != MQTTSuccess)
		{
			LogError("MQTT Agent returned error code: %d during publish operation.",
					 xCommandContext.xReturnStatus);
			xResult = pdFALSE;
		}
	}
	else
	{
		LogError("MQTTAgent_Publish returned error code: %d.",
				 xStatus);
	}

	return xResult;
}

static BaseType_t xIsMqttConnected(void)
{
	/* Wait for MQTT to be connected */
	EventBits_t uxEvents = xEventGroupWaitBits(xSystemEvents,
											   EVT_MASK_MQTT_CONNECTED,
											   pdFALSE,
											   pdTRUE,
											   0);

	return ((uxEvents & EVT_MASK_MQTT_CONNECTED) == EVT_MASK_MQTT_CONNECTED);
}

/* CRC init function */
static void CRC_Init(void)
{
	CRC_HandleTypeDef hcrc;
	hcrc.Instance = CRC;
	hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
	hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
	hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
	hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
	hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
	__HAL_RCC_CRC_CLK_ENABLE();
	if (HAL_CRC_Init(&hcrc) != HAL_OK)
	{
		LogError("CRC Init Error");
	}
}

static BaseType_t xInitSensors(void)
{
	int32_t lBspError = BSP_ERROR_NONE;

	BSP_AUDIO_Init_t AudioInit;

	/* Select device depending on the Instance */
	AudioInit.Device = AUDIO_IN_DEVICE_DIGITAL_MIC1;
	AudioInit.SampleRate = AUDIO_FREQUENCY_16K;
	AudioInit.BitsPerSample = AUDIO_RESOLUTION_16B;
	AudioInit.ChannelsNbr = 1;
	AudioInit.Volume = 100; /* Not used */
	lBspError = BSP_AUDIO_IN_Init(0, &AudioInit);
	return (lBspError == BSP_ERROR_NONE ? pdTRUE : pdFALSE);
}

static bool scan_command_number_array_arg(const char* payload, const char* command, int *value_array, size_t num_values) {
    // we should get something like {"v":"2.1","ct":0,"cmd":"set-confidence-threshold 22"}
    const char* command_pos = strstr(payload, command);

    if (!command_pos) {
        return false;
    }

    if (0 == num_values) {
        return false;
    }

    const char * value_pos = &command_pos[strlen(command)];
    size_t value_idx = 0;
    do {
        int num_found = sscanf(value_pos, "%d", &value_array[value_idx]);
        if (num_found != 1) {
        	return false;
        }
        value_idx++;
    	value_pos = strstr(value_pos, " ");
    	if (NULL == value_pos) {
    		break;
    	}
    	value_pos++; // move the the "end" of space
    } while(NULL != value_pos && value_idx < num_values);

    if (value_idx < num_values) {
    	return false;
    }
    return true;
}

static void apply_class_offset(const char* name, int offset) {
	for (int i = 0; i < AI_NETWORK_OUT_1_SIZE; i++) {
    	if (0 == strcmp(name, sAiClassLabels[i])) {
    		LogInfo("Applying offset %d to %s", offset, sAiClassLabels[i]);
    		confidence_offsets[i] = offset;
    		return;
    	}
	}
}

static bool scan_command_number_arg(const char* payload, const char* command, int *value) {
    // we should get something like {"v":"2.1","ct":0,"cmd":"set-confidence-threshold 22"}
    const char* command_pos = strstr(payload, command);
    if (!command_pos) {
        return false;
    }
    const char * value_pos = &command_pos[strlen(command)];
    int num_found = sscanf(value_pos, "%d", value);
    return (num_found == 1);
}

static void on_c2d_message( void * subscription_context, MQTTPublishInfo_t * publish_info ) {
    (void) subscription_context;

    const char* OFFSETS_CMD = "set-confidence-offsets ";
    const char* THRESHOLD_CMD = "set-confidence-threshold ";
    const char* INACTIVITY_TIMEOUT_CMD = "set-inactivity-timeout ";
    if (!publish_info) {
        LogError("on_c2d_message: Publish info is NULL?");
        return;
    }
    LogInfo("<<< %.*s", publish_info->payloadLength, publish_info->pPayload);

    char* payload = (char *)publish_info->pPayload;
    // terminate the string just in case.
    // Don't really care about the last char for now so we can overwrite it with null and truncate the value
    payload[publish_info->payloadLength] = 0;

    if (NULL != strstr(payload, OFFSETS_CMD)) {
    	if (scan_command_number_array_arg(payload, OFFSETS_CMD, confidence_offsets, AI_NETWORK_OUT_1_SIZE)) {
        	LogInfo("New offsets set:");
        	for (int i = 0; i < AI_NETWORK_OUT_1_SIZE; i++) {
            	LogInfo("%s: %d", sAiClassLabels[i], confidence_offsets[i]);
        	}
    	} else {
    		LogError("Failed %s!", OFFSETS_CMD);
    	}
    } else if (NULL != strstr(payload, THRESHOLD_CMD)) {
    	if (scan_command_number_arg(payload, THRESHOLD_CMD, &confidence_threshold)) {
        	LogInfo("New confidence threshold: %d", confidence_threshold);
    	} else {
    		LogError("Failed %s!", THRESHOLD_CMD);
    	}
    } else if (NULL != strstr(payload, INACTIVITY_TIMEOUT_CMD)) {
    	if (scan_command_number_arg(payload, INACTIVITY_TIMEOUT_CMD, &inactivity_timeout)) {
        	LogInfo("New inactivity timeout: %d", inactivity_timeout);
    	} else {
    		LogError("Failed %s!", INACTIVITY_TIMEOUT_CMD);
    	}
    } else {
    	LogError("Unknown command!");
    }
}

static bool subscribe_to_c2d_topic(const char * device_id)
{
    char sub_topic[strlen(device_id) + 20];
    sprintf(sub_topic, "iot/%s/cmd", device_id);

    MQTTAgentHandle_t agent_handle = xGetMqttAgentHandle();
    if (agent_handle == NULL )  {
	    LogError("Unable to get agent handle");
	    return false;
    }

    MQTTStatus_t mqtt_status = MqttAgent_SubscribeSync( agent_handle,
		sub_topic,
		1 /* qos */,
		on_c2d_message,
		NULL
    );
    if (MQTTSuccess != mqtt_status) {
        LogError("Failed to SUBSCRIBE to topic with error = %u.", mqtt_status);
        return false;
    }

    LogInfo("Subscribed to topic %s.\n\n", sub_topic);

    return true;
}

// basically makes the next detection trigger due to long time diff:
static void set_detected_never(void) {
	last_detection_time = pdMS_TO_TICKS(xTaskGetTickCount()) - 99999U;
}
static void set_detected_now(void) {
	last_detection_time = pdMS_TO_TICKS(xTaskGetTickCount());
}
static bool is_detection_blocked(void) {
	return ((int)(pdMS_TO_TICKS(xTaskGetTickCount()) - last_detection_time) < inactivity_timeout);
}

void vMicSensorPublishTask(void *pvParameters)
{
	BaseType_t xResult = pdFALSE;
	BaseType_t xExitFlag = pdFALSE;
	char payloadBuf[MQTT_PUBLISH_MAX_LEN];
	MQTTAgentHandle_t xAgentHandle = NULL;
	char pcTopicString[MQTT_PUBLICH_TOPIC_STR_LEN] = {0};
	uint32_t ulNotifiedValue = 0;

	(void)pvParameters; /* unused parameter */

	/**
	 * Initialize the CRC IP required by X-CUBE-AI.
	 * Must be called before any usage of the ai library API.
	 */
	CRC_Init();

	/**
	 * get task handle for notifications
	 */
	xMicTask = xTaskGetCurrentTaskHandle();

	xResult = xInitSensors();

	if (xResult != pdTRUE)
	{
		LogError("Error while Audio sensor.");
		vTaskDelete(NULL);
	}

	xResult = PreProc_DPUInit(&xAudioProcCtx);

	if (xResult != pdTRUE)
	{
		LogError("Error while initializing Preprocessing.");
		vTaskDelete(NULL);
	}

	/**
	 * get the AI model
	 */
	AiDPULoadModel(&xAIProcCtx, "network");

	/**
	 * transfer quantization parametres included in AI model to the Audio DPU
	 */
	xAudioProcCtx.output_Q_offset = xAIProcCtx.input_Q_offset;
	xAudioProcCtx.output_Q_inv_scale = xAIProcCtx.input_Q_inv_scale;

    char pcDeviceId[64];
    size_t uxDevNameLen = KVStore_getString(CS_CORE_THING_NAME, pcDeviceId, 64);

    size_t uxTopicLen = 0;
	if (uxDevNameLen > 0)
	{
		sprintf(pcTopicString, "$aws/rules/msg_d2c_rpt/%s/2.1/0", pcDeviceId);
        uxTopicLen = strlen(pcTopicString);
	}

	if ((uxTopicLen == 0) || (uxTopicLen >= MQTT_PUBLICH_TOPIC_STR_LEN))
	{
		LogError("Failed to construct topic string. Please configure the device");
		while (true) {
			vTaskDelay( 10000 ); // wait forever
		}

		// xExitFlag = pdTRUE;
	}
	const char * inactive_position = "36.2409530, -115.0633452";
	const char * device_position = "41.8774330,-87.6389937"; // Avnet Chicago
	if (0 == strcmp("ml-ai-demo-01", pcDeviceId)) {
	    device_position = "36.0861112,-115.1786765"; // NE Frank Sinatra Dr and W Russell Rd
	} else if (0 == strcmp("ml-ai-demo-02", pcDeviceId)) {
	    device_position = "36.0863886,-115.1732692"; // NW Las Vegas Blvd and W Russell Rd
	} else if (0 == strcmp("ml-ai-demo-03", pcDeviceId)) {
	    device_position = "36.0898220,-115.1726040"; // NE Las Vegas Blvd and Four Seasons Dr
	} else if (0 == strcmp("ml-ai-demo-04", pcDeviceId)) {
	    device_position = "36.0934706,-115.1731671"; // SW Las Vegas Blvd and Mandalay Bay Rd
	} else if (0 == strcmp("ml-ai-demo-05", pcDeviceId)) {
	    device_position = "36.0933494,-115.1814609"; // SW W Hacienda Ave and Dean Martin Dr.
	} else if (0 == strcmp("ml-ai-demo-06", pcDeviceId)) {
	    device_position = "36.0861112,-115.1786765"; // NE Frank Sinatra Dr and W Russell Rd
	} else if (0 == strcmp("ml-ai-demo-07", pcDeviceId)) {
	    device_position = "36.0863886,-115.1732692"; // NW Las Vegas Blvd and W Russell Rd
	} else if (0 == strcmp("ml-ai-demo-08", pcDeviceId)) {
	    device_position = "36.0898220,-115.1726040"; // NE Las Vegas Blvd and Four Seasons Dr
	} else if (0 == strcmp("ml-ai-demo-09", pcDeviceId)) {
	    device_position = "36.0934706,-115.1731671"; // SW Las Vegas Blvd and Mandalay Bay Rd
	} else if (0 == strcmp("ml-ai-demo-10", pcDeviceId)) {
	    device_position = "36.0933494,-115.1814609"; // SW W Hacienda Ave and Dean Martin Dr.
	}

	subscribe_to_c2d_topic(pcDeviceId);

	vSleepUntilMQTTAgentReady();

	xAgentHandle = xGetMqttAgentHandle();

	LogDebug("start audio");
	if (BSP_AUDIO_IN_Record(0, pucAudioBuff, AUDIO_BUFF_SIZE) != BSP_ERROR_NONE)
	{
		LogError("AUDIO IN : FAILED.\n");
	}

	// trigger sending idle immediately if we don't detect:
	set_detected_never();
	bool idle_needs_sending = true;


	LogInfo("**** DEMO SOUNDS v%s ****", APP_VERSION);
	for (uint32_t clidx = 0; clidx < CTRL_X_CUBE_AI_MODE_CLASS_NUMBER; clidx++) {
		LogInfo("**** %s [%d]", sAiClassLabels[clidx], confidence_offsets[clidx]);
	}
	LogInfo("********");


	while (xExitFlag == pdFALSE)
	{
		TimeOut_t xTimeOut;

        xAudioProcCtx.S_Spectr.spectro_sum = 0;

		vTaskSetTimeOutState(&xTimeOut);

		if (xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotifiedValue, portMAX_DELAY) == pdTRUE) {
			/**
			 * Audio pre-processing on audio half buffer
			 */
			if (ulNotifiedValue & MIC_EVT_DMA_HALF)
				PreProc_DPU(&xAudioProcCtx, pucAudioBuff, pcSpectroGram);
			if (ulNotifiedValue & MIC_EVT_DMA_CPLT)
				PreProc_DPU(&xAudioProcCtx, pucAudioBuff + AUDIO_HALF_BUFF_SIZE, pcSpectroGram);

			/**
			 * AI processing
			 */
			AiDPUProcess(&xAIProcCtx, pcSpectroGram, pfAIOutput);
		}

		const char* detected_class = NULL;
		int confidence_score_percent;

		if (xAudioProcCtx.S_Spectr.spectro_sum > CTRL_X_CUBE_AI_SPECTROGRAM_SILENCE_THR) {
			do { // to easily step out
				detected_class = NULL;
				/**
				 * if not silence frame
				 */
				uint32_t max_idx = 0; // assume best is at index 0 and disprove
				float max_out = pfAIOutput[0] + ((float32_t)confidence_offsets[0]) / 100.0F;
				for (uint32_t i = 1; i < CTRL_X_CUBE_AI_MODE_CLASS_NUMBER; i++) {
					float32_t class_out = pfAIOutput[i]  + ((float32_t)confidence_offsets[i] / 100.0F);
					if (class_out > max_out) {
						max_idx = i;
						max_out = class_out;
					}
				}
				const char* c = sAiClassLabels[max_idx];

				confidence_score_percent = (int)(100.0 * max_out);
				if (confidence_score_percent > 100) {
					confidence_score_percent = 100;
				} else if (confidence_score_percent < 0) {
					confidence_score_percent = 0;
				}

				if (0 == strcmp("other", c)) {
					LogInfo("Detected \"other\" with score %s%d. Ignoring...",
							((confidence_score_percent < confidence_threshold) ? " " : "*"),
							confidence_score_percent
					);
					break;
				}

				if (confidence_score_percent < confidence_threshold) {
					LogInfo("Confidence is low for %s (%d<%d). Ignoring...",
							c,
							confidence_score_percent,
							confidence_threshold
					);
					break;
				}

				if (is_detection_blocked()) {
					LogInfo("Blocking %s with score %s%d...",
							c,
							((confidence_score_percent < confidence_threshold) ? " " : "*"),
							confidence_score_percent
					);
					break;
				}

				// we are good
				set_detected_now();
				detected_class = c;
			} while (false);  // to easily step out
		}

		size_t bytesWritten;
		if (detected_class) {
			idle_needs_sending = true;
			bytesWritten = (size_t) snprintf(payloadBuf, (size_t)MQTT_PUBLISH_MAX_LEN,
					"{\"d\":"\
					"[{\"d\":{\"version\":\"MLDEMO-" APP_VERSION "\",\"class\":\"%s\",\"confidence\":%d,\"position\":[%s]}}]"\
					",\"mt\":0}",
					detected_class,
					confidence_score_percent,
					device_position
			);
		} else if (idle_needs_sending && !is_detection_blocked()) {
			idle_needs_sending = false;
			bytesWritten = (size_t) snprintf(payloadBuf, (size_t)MQTT_PUBLISH_MAX_LEN,
					"{\"d\":"\
					"[{\"d\":{\"version\":\"MLDEMO-" APP_VERSION "\",\"class\":\"%s\",\"confidence\":%d,\"position\":[%s]}}]"\
					",\"mt\":0}",
					"not-active",
					100,
					inactive_position
			);
		} else {
			// do not send anything
			continue;
		}

		if (xIsMqttConnected() == pdTRUE) {
			if (bytesWritten < MQTT_PUBLISH_MAX_LEN) {
				xResult = prvPublishAndWaitForAck(xAgentHandle,
												  pcTopicString,
												  payloadBuf,
												  bytesWritten
				);
			} else if (bytesWritten > 0) {
				LogError("Not enough buffer space.");
			} else {
				LogError("MQTT Publish call failed.");
			}

			if (xResult == pdTRUE) {
				LogDebug(payloadBuf);
			}
		}
	}
}

/**
 * @brief  Manage the BSP audio in half transfer complete event.
 * @param  Instance Audio in instance.
 * @retval None.
 */

void BSP_AUDIO_IN_HalfTransfer_CallBack(uint32_t Instance)
{
	(void)Instance;
	assert_param(Instance == 0);
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	BaseType_t rslt = pdFALSE;
	rslt = xTaskNotifyFromISR(xMicTask,
							  MIC_EVT_DMA_HALF,
							  eSetBits,
							  &xHigherPriorityTaskWoken);
	configASSERT(rslt == pdTRUE);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief  Manage the BSP audio in transfer complete event.
 * @param  Instance Audio in instance.
 * @retval None.
 */
void BSP_AUDIO_IN_TransferComplete_CallBack(uint32_t Instance)
{
	(void)Instance;
	assert_param(Instance == 0);
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	BaseType_t rslt = pdFALSE;
	rslt = xTaskNotifyFromISR(xMicTask,
							  MIC_EVT_DMA_CPLT,
							  eSetBits,
							  &xHigherPriorityTaskWoken);
	configASSERT(rslt == pdTRUE);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
