//
// Copyright: Avnet 2023
// Created by Marven Gilhespie
//

#include "logging_levels.h"
/* define LOG_LEVEL here if you want to modify the logging level from the default */
#define LOG_LEVEL    LOG_INFO
#include "logging.h"

/* Standard includes. */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "kvstore.h"
#include "mbedtls_transport.h"

#include "sys_evt.h"

/* MQTT library includes. */
#include "core_mqtt.h"

/* MQTT agent include. */
#include "core_mqtt_agent.h"
#include "mqtt_agent_task.h"

/* Subscription manager header include. */
#include "subscription_manager.h"

/* Sensor includes */
#include "b_u585i_iot02a_motion_sensors.h"

//Iotconnect
#include "iotconnect.h"
#include "iotconnect_lib.h"
#include "iotconnect_telemetry.h"
#include "iotconnect_event.h"
#include <config/iotconnect_config.h>

// BSP-Specific
#include "stm32u5xx.h"
#include "b_u585i_iot02a.h"

// Constants
#define APP_VERSION 			"01.00.00"		// Version string in telemetry data
#define MQTT_PUBLISH_PERIOD_MS 	( 3000 )		// Size of statically allocated buffers for holding topic names and payloads.
// Temperature Simulation Parameters
static bool simulateTemperature = false;
static float presetTempF = 65.0f;  // Preset temperature
static bool targetTemperatureReached = false;
static bool grillState = false; // False means grill is off
static int evenSurface = 1; // true means grill is on an even surface
static float currentTempF = 65.0f; // Initialize with start temperature

// @brief	IOTConnect configuration defined by application
static IotConnectAwsrtosConfig awsrtos_config;

// Prototypes
static BaseType_t init_sensors( void );
static char* create_telemetry_json(IotclMessageHandle msg, BSP_MOTION_SENSOR_Axes_t accel_data, BSP_MOTION_SENSOR_Axes_t gyro_data, BSP_MOTION_SENSOR_Axes_t mag_data, float simulatedTempF);
static void on_command(IotclEventData data);
static void on_ota(IotclEventData data);
static void command_status(IotclEventData data, bool status, const char *command_name, const char *message);
static float simulateTemperatureRise(void);

static float simulateTemperatureRise() {
    if (!simulateTemperature) {
        return currentTempF; // Return the current temperature
    }

    float rateOfChange = 20.0f; // Rate of temperature change per second
//    float interval = 2.0f; // Interval in seconds between function calls
    float tempDifference = presetTempF - currentTempF;
//    float tempChange = rateOfChange * interval;

    if (fabs(tempDifference) < rateOfChange) {
        currentTempF = presetTempF;
        targetTemperatureReached = true; // Target temperature reached
    } else {
        currentTempF += (tempDifference > 0) ? rateOfChange : -rateOfChange;
        targetTemperatureReached = false; // Still transitioning
    }
    return currentTempF;
}
/* @brief	Main IoT-Connect application task
 *
 * @param	pvParameters, argument passed by xTaskCreate
 *
 * This is started by the initialization code in app_main.c which first performs board and
 * networking initialization
 */
void iotconnect_app( void * pvParameters )
{
	(void) pvParameters;
    static bool ledState = false;

    if (init_sensors() != pdTRUE) {
        LogError("Error while initializing motion sensors.");
        vTaskDelete(NULL);
    }

    // Get some settings from non-volatile storage.  These can be set on the command line
    // using the conf command.

    char *device_id = KVStore_getStringHeap(CS_CORE_THING_NAME, NULL);   // Device ID
    // char *cpid = KVStore_getStringHeap(CS_IOTC_CPID, NULL);
    // char *iotc_env = KVStore_getStringHeap(CS_IOTC_ENV, NULL);

    if (device_id == NULL) {
    	LogError("IOTC configuration thing_name is not set\n");
		vTaskDelete(NULL);
    }

    // IoT-Connect configuration setup

    IotConnectClientConfig *config = iotconnect_sdk_init_and_get_config();
    //config->cpid = cpid;
	//config->env = iotc_env;
    config->cpid = "<none>";
	config->env = "<none>";
	config->duid = device_id;
	config->cmd_cb = on_command;
	config->ota_cb = on_ota;
	config->status_cb = NULL;
	config->auth_info.type = IOTC_X509;
	// use TLS_ROOT_CA_CERT_LABEL just to bypass it for now
    config->auth_info.https_root_ca              = xPkiObjectFromLabel( TLS_ROOT_CA_CERT_LABEL ); // xPkiObjectFromLabel( TLS_HTTPS_ROOT_CA_CERT_LABEL );
    config->auth_info.mqtt_root_ca               = xPkiObjectFromLabel( TLS_ROOT_CA_CERT_LABEL );
    config->auth_info.data.cert_info.device_cert = xPkiObjectFromLabel( TLS_CERT_LABEL );
    config->auth_info.data.cert_info.device_key  = xPkiObjectFromLabel( TLS_KEY_PRV_LABEL );;

#if defined(IOTCONFIG_USE_DISCOVERY_SYNC)
    // Get MQTT configuration from discovery and sync
    iotconnect_sdk_init(NULL);
#else
    // Not using Discovery and Sync so some additional settings, are obtained from the CLI,
    char *mqtt_endpoint_url = KVStore_getStringHeap(CS_CORE_MQTT_ENDPOINT, NULL);
    char *telemetry_cd = KVStore_getStringHeap(CS_IOTC_CD, NULL);

    if (mqtt_endpoint_url == NULL || telemetry_cd == NULL) {
    	LogError ("IOTC configuration, mqtt_endpoint, telemetry_cd not set");
    	vTaskDelete( NULL );
    }

    awsrtos_config.host = mqtt_endpoint_url;
	awsrtos_config.telemetry_cd = telemetry_cd;
	iotconnect_sdk_init(&awsrtos_config);
#endif

    while (1) {
        int32_t sensor_error = BSP_ERROR_NONE;
        BSP_MOTION_SENSOR_Axes_t xAcceleroAxes, xGyroAxes, xMagnetoAxes;

        sensor_error = BSP_MOTION_SENSOR_GetAxes(0, MOTION_GYRO, &xGyroAxes);
        sensor_error |= BSP_MOTION_SENSOR_GetAxes(0, MOTION_ACCELERO, &xAcceleroAxes);
        sensor_error |= BSP_MOTION_SENSOR_GetAxes(1, MOTION_MAGNETO, &xMagnetoAxes);

        if (xAcceleroAxes.x > 100 || xAcceleroAxes.x < -100) {
            grillState = false;  // Turn off the grill
            presetTempF = 65.0f; // Reset temperature
            evenSurface = 0; // Set even-surface telemetry to false
            BSP_LED_Off(LED_GREEN);
            // Log and handle this event
            LogInfo("Unsafe accelerometer reading detected. Grill turned off for safety.");
        } else {
            evenSurface = 1; // Set even-surface to true after consecutive safe readings
             }

        if (sensor_error == BSP_ERROR_NONE) {
            float simulatedTempF = simulateTemperatureRise();  // Get simulated temperature

            IotclMessageHandle message = iotcl_telemetry_create();
            char* json_message = create_telemetry_json(message, xAcceleroAxes, xGyroAxes, xMagnetoAxes, simulatedTempF);

            if (json_message == NULL) {
                LogError("Could not create telemetry data\n");
                vTaskDelete(NULL);
            }

            iotconnect_sdk_send_packet(json_message);  // Send telemetry data
            iotcl_destroy_serialized(json_message);
        }

            BSP_LED_On(LED_RED); // Keep the LED on when the target temperature is reached
        }
//        TickType_t currentTime = xTaskGetTickCount();
    if (currentTempF <= 65.0f) {
            BSP_LED_Off(LED_RED); // Turn off the red LED
    } else if (!targetTemperatureReached) {
    	ledState = !ledState;        // Toggle the LED state
    	if (ledState) {
    		BSP_LED_On(LED_RED);
    	} else {
    		BSP_LED_Off(LED_RED);
    	}
    } else {
    	BSP_LED_On(LED_RED); // Keep the LED on when the target temperature is reached
    }

        vTaskDelay(pdMS_TO_TICKS(MQTT_PUBLISH_PERIOD_MS));
    }
}


/* @brief	Initialize the dev board's accelerometer, gyro and magnetometer sensors
 */
static BaseType_t init_sensors( void )
{
    int32_t lBspError = BSP_ERROR_NONE;

    /* Gyro + Accelerometer*/
    lBspError = BSP_MOTION_SENSOR_Init( 0, MOTION_GYRO | MOTION_ACCELERO );
    lBspError |= BSP_MOTION_SENSOR_Enable( 0, MOTION_GYRO );
    lBspError |= BSP_MOTION_SENSOR_Enable( 0, MOTION_ACCELERO );
    lBspError |= BSP_MOTION_SENSOR_SetOutputDataRate( 0, MOTION_GYRO, 1.0f );
    lBspError |= BSP_MOTION_SENSOR_SetOutputDataRate( 0, MOTION_ACCELERO, 1.0f );

    /* Magnetometer */
    lBspError |= BSP_MOTION_SENSOR_Init( 1, MOTION_MAGNETO );
    lBspError |= BSP_MOTION_SENSOR_Enable( 1, MOTION_MAGNETO );
    lBspError |= BSP_MOTION_SENSOR_SetOutputDataRate( 1, MOTION_MAGNETO, 1.0f );
  	BSP_LED_Off(LED_GREEN); // Ensure the grill switch is off at startup 
    return( lBspError == BSP_ERROR_NONE ? pdTRUE : pdFALSE );
}

/*
static void send_faux_telemetry_data(IotclMessageHandle msg, BSP_MOTION_SENSOR_Axes_t* accel) {
	int32_t x = abs(accel->z);
	int32_t y = abs(accel->x);
	int32_t z = abs(accel->y);
	z = abs(z - 1000); // subtract 1G from z axis which could also be negative, so we flip
	int32_t combined = x + y + z;

	// check for trigger
	if (combined > 100) {
	    iotcl_telemetry_set_number(msg, "temperature", 123.0F);
	} else {
		iotcl_telemetry_set_number(msg, "temperature", 10.0F);
	}
}
*/
/* @brief 	Create JSON message containing telemetry data to publish
 *
 */
static char *create_telemetry_json(IotclMessageHandle msg, BSP_MOTION_SENSOR_Axes_t accel_data,
								BSP_MOTION_SENSOR_Axes_t gyro_data, BSP_MOTION_SENSOR_Axes_t mag_data, float simulatedTempF) {


    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    iotcl_telemetry_add_with_iso_time(msg, NULL);

//    send_faux_telemetry_data(msg,  &accel_data);

    iotcl_telemetry_set_number(msg, "gyro_x", gyro_data.x);
    iotcl_telemetry_set_number(msg, "gyro_y", gyro_data.y);
    iotcl_telemetry_set_number(msg, "gyro_z", gyro_data.z);

    iotcl_telemetry_set_number(msg, "accelerometer_x", accel_data.x);
    iotcl_telemetry_set_number(msg, "accelerometer_y", accel_data.y);
    iotcl_telemetry_set_number(msg, "accelerometer_z", accel_data.z);

#if 0
    iotcl_telemetry_set_number(msg, "magnetometer_x", mag_data.x);
    iotcl_telemetry_set_number(msg, "magnetometer_y", mag_data.y);
    iotcl_telemetry_set_number(msg, "magnetometer_z", mag_data.z);
#endif

    iotcl_telemetry_set_string(msg, "version", APP_VERSION);

    // Add the simulated temperature to the telemetry data
    iotcl_telemetry_set_number(msg, "simulated_temp", simulatedTempF);
    iotcl_telemetry_set_number(msg, "set_temp", presetTempF);
    iotcl_telemetry_set_bool(msg, "grill_state", grillState);
    iotcl_telemetry_set_number(msg, "even_surface", evenSurface);
    const char* str = iotcl_create_serialized_string(msg, false);

	if (str == NULL) {
		LogInfo( "serialized_string is NULL");
	}

	iotcl_telemetry_destroy(msg);
    return (char* )str;
}


/* @brief	Callback when a a cloud-to-device command is received on the subscribed MQTT topic
 */
static void on_command(IotclEventData data) {
	if (data == NULL ) {
		LogWarn("on_command called with data = NULL");
		return;
	}

	char *command = iotcl_clone_command(data);

    if (NULL != command) {
    	LogInfo("Received command: %s", command);

    	if(NULL != strstr(command, "led-red") ) {
			if (NULL != strstr(command, "on")) {
				LogInfo("led-red on");
				BSP_LED_On(LED_RED);
			} else {
				LogInfo("led-red off");
				BSP_LED_Off(LED_RED);
			}
			command_status(data, true, command, "OK");
		} else if(NULL != strstr(command, "led-green") ) {
			if (NULL != strstr(command, "on")) {
				BSP_LED_On(LED_GREEN);
			} else {
				BSP_LED_Off(LED_GREEN);
			}
			command_status(data, true, command, "OK");
		} else if (NULL != strstr(command, "set-temp")) {
			float tempVal;
			LogInfo("Command received: '%s'", command);
			int sscanfResult = sscanf(command, "set-temp %f", &tempVal);
//			LogInfo("sscanf result: %d, parsed value: %f", sscanfResult, tempVal);

			if (sscanfResult == 1) {
				if (grillState) {  // Check if grill is on

								if (sscanf(command, "set-temp %f", &tempVal) == 1) {
									presetTempF = tempVal;
									simulateTemperature = true;  // Start simulation
//									simStartTime = xTaskGetTickCount();
									LogInfo("Temperature set to %.2f", presetTempF);
									command_status(data, true, command, "Temperature set");
								} else {
									command_status(data, false, command, "Invalid temperature value");
								}
							} else {
								// Handle the case when grill is off
								LogInfo("Grill is off. Cannot set temperature.");
								command_status(data, false, command, "Grill is off. Cannot set temperature.");
							}
			} else {
			    LogInfo("Failed to parse set-temp command");
			}
		} else if (strstr(command, "grill_switch")) {
		            if (strstr(command, "on")) {
		                grillState = true;
		                BSP_LED_On(LED_GREEN);  // Turn on green LED
		                command_status(data, true, "grill_switch", "Grill is ON");
		            } else if (strstr(command, "off")) {
		                grillState = false;
		                BSP_LED_Off(LED_GREEN);  // Turn off green LED
		                presetTempF = 65.0f; // Reset temperature
		                command_status(data, true, "grill_switch", "Grill is OFF");
		            } else {
		                command_status(data, false, "grill_switch", "Invalid command value");
		            }
		} else {
			LogInfo("command not recognized");
			command_status(data, false, command, "Not implemented");
		}
        free((void*) command);
    } else {
		LogInfo("No command, internal error");
        command_status(data, false, "?", "Internal error");
    }
}


/* @brief	Generate a command acknowledgement message and publish it on the events topic
 *
 */
static void command_status(IotclEventData data, bool status, const char *command_name, const char *message) {
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, status, message);				// defined in iotc-c-lib iotconnect_evvent.c

    if (ack == NULL) {
    	LogInfo("command: no ack required");
    	return;
    }

	LogInfo("command: %s status=%s: %s\r\n", command_name, status ? "OK" : "Failed", message);
	LogInfo("Sent CMD ack: %s\r\n", ack);
	vTaskDelay(100);
	iotconnect_sdk_send_packet(ack);
	free((void*) ack);
}

static bool is_app_version_same_as_ota(const char *version) {
    return strcmp(APP_VERSION, version) == 0;
}

static bool app_needs_ota_update(const char *version) {
    return strcmp(APP_VERSION, version) < 0;
}

static void on_ota(IotclEventData data) {
    const char *message = NULL;
    char *url = iotcl_clone_download_url(data, 0);
    bool success = false;
    if (NULL != url) {
    	LogInfo("Download URL is: %s\r\n", url);
        const char *version = iotcl_clone_sw_version(data);
        if (!version) return; // TODO: figure this out

        if (is_app_version_same_as_ota(version)) {
        	LogInfo("OTA request for same version %s. Sending success\r\n", version);
            success = true;
            message = "Version is matching";
        } else if (app_needs_ota_update(version)) {
        	LogInfo("OTA update is required for version %s.\r\n", version);
            success = false;
            message = "Not implemented";
        } else {
        	LogInfo("Device firmware version %s is newer than OTA version %s. Sending failure\r\n", APP_VERSION,
                    version);
            // Not sure what to do here. The app version is better than OTA version.
            // Probably a development version, so return failure?
            // The user should decide here.
            success = false;
            message = "Device firmware version is newer";
        }

        free((void*) url);
        free((void*) version);
    } else {
        // compatibility with older events
        // This app does not support FOTA with older back ends, but the user can add the functionality
        const char *command = iotcl_clone_command(data);
        if (NULL != command) {
            // URL will be inside the command
        	LogInfo("Command is: %s\r\n", command);
            message = "Old back end URLS are not supported by the app";
            free((void*) command);
        }
    }
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, success, message);
    if (NULL != ack) {
    	LogInfo("Sent OTA ack: %s\r\n", ack);
        iotconnect_sdk_send_packet(ack);
        free((void*) ack);
    }
}

