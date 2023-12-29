//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by Marven Gilhespie <mgilhespie@witekio.com> on 24/10/23
//

#ifndef IOTCONNECT_H
#define IOTCONNECT_H

///#include <iotc_auth_driver.h>
#include <stddef.h>
//#include "nx_api.h"
//#include "nxd_dns.h"
#include "iotconnect_event.h"
#include "iotconnect_telemetry.h"
#include "iotconnect_discovery.h" // for sync enums
#include "iotconnect_lib.h"
#include "PkiObject.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UNDEFINED,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    // MQTT_FAILED, this status is not applicable to AzureRTOS implementation
    // TODO: Sync statuses etc.
} IotConnectConnectionStatus;

typedef enum {
    IOTC_KEY,		// Symmetric key
	IOTC_X509, 		// Private key and ceritificate
} IotConnectAuthType;

typedef void (*IotConnectStatusCallback)(IotConnectConnectionStatus data);

typedef struct {
	const char *host;
	const char *telemetry_cd;		// Device template "cd"
} IotConnectAwsrtosConfig;



typedef struct {
    IotConnectAuthType type;

    PkiObject_t https_root_ca;
    PkiObject_t mqtt_root_ca;

    union { // union because we may support different types of auth
    	struct
    	{
    		PkiObject_t device_cert;
    		PkiObject_t device_key;
    	} cert_info;
    } data;
} IotConnectAuth;

typedef struct {
    char *env;    // Environment name. Contact your representative for details.
    char *cpid;   // Settings -> Company Profile.
    char *duid;   // Name of the device.
    IotConnectAuth auth_info;
    IotclOtaCallback ota_cb; // callback for OTA events.
    IotclCommandCallback cmd_cb; // callback for command events.
    IotclMessageCallback msg_cb; // callback for ALL messages, including the specific ones like cmd or ota callback.
    IotConnectStatusCallback status_cb; // callback for connection status
} IotConnectClientConfig;


IotConnectClientConfig *iotconnect_sdk_init_and_get_config();

int iotconnect_sdk_init(IotConnectAwsrtosConfig *config);

IotclSyncResult iotconnect_get_last_sync_result();

bool iotconnect_sdk_is_connected();

IotclConfig *iotconnect_sdk_get_lib_config();

void iotconnect_sdk_send_packet(const char *data);


/* NoteL: Neither IotConnectSdk_receive nor iotconnect_sdk_poll are used by this
 * STM U5 AWS implementation.  An internal thread handles the receipt of messages
 * on the subscribed topic
 */

// Receive loop hook forever-blocking for for C2D messages.
// Either call this function, or IoTConnectSdk_Poll()
void iotconnect_sdk_receive();

// Receive poll hook for for C2D messages.
// Either call this function, or IotConnectSdk_Receive()
// Set wait_time to a multiple of NX_IP_PERIODIC_RATE
void iotconnect_sdk_poll(int wait_time_ms);

void iotconnect_sdk_disconnect();

#ifdef __cplusplus
}
#endif

#endif
