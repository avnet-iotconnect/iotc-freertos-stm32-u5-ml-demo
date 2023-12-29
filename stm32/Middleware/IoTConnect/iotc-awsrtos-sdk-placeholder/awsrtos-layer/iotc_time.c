/*
 * Copyright: Avnet 2023
 * Author: Marven Gilhespie mgilhespie@witekio.com
 * Creation: 05/10/23.
 */

#include "sntp.h"
#include <time.h>
#include "FreeRTOS.h"
#include "task.h"
#include "iotconnect_common.h"
#include "stdint.h"
#include "stdbool.h"
#include "logging_levels.h"
#include "logging.h"
#include "event_groups.h"
#include "sys_evt.h"

#define SNTP_SERVER_NAME					"pool.ntp.org"

#define TX_TIMER_TICKS_PER_SECOND			1000 // FIXME: Replace with configTICK_RATE_HZ

#ifndef IOTC_MTB_TIME_MAX_TRIES
#define IOTC_MTB_TIME_MAX_TRIES 			10
#endif

static volatile bool callback_received = false;	/* Indicate we have received a response and that the time has been set */
static time_t timenow = 0;
static uint32_t	unix_time_base;				/* System clock time offset for UTC.  */

/*
 * FIXME: 32-bit integers are used for some of these functions and for unix_time_base.
 */
uint32_t tx_time_get(void)
{
	return xTaskGetTickCount();
}

void set_time(uint32_t unix_seconds)
{
	uint32_t system_time_in_seconds = tx_time_get() / TX_TIMER_TICKS_PER_SECOND;
    unix_time_base = (unix_seconds - system_time_in_seconds);
}

int unix_time_get(uint32_t *unix_time)
{
    /* Return number of seconds since Unix Epoch (1/1/1970 00:00:00).  */
	*unix_time =  unix_time_base + (tx_time_get() / TX_TIMER_TICKS_PER_SECOND);
	return 0;
}

time_t time(time_t *t)
{
	uint32_t time_now;
	unix_time_get(&time_now);

	return (time_t) time_now;
}

void iotc_set_system_time_us(uint32_t sec, uint32_t us)
{
//    taskENTER_CRITICAL();
    set_time(sec);
    callback_received = true;
//    taskEXIT_CRITICAL();

    time_t timenow = time(NULL);
    LogInfo("Time set to: %s!\n", iotcl_to_iso_timestamp(timenow));

}

int iotc_stm_aws_time_obtain(const char *server)
{
    uint8_t reachable = 0;

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, server);
    sntp_init();

    timenow = time(NULL);
    for (int i = 0; (reachable = sntp_getreachability(0)) == 0 && i < IOTC_MTB_TIME_MAX_TRIES; i++) {
        vTaskDelay(1000);

        if (reachable) {
            break;
        }
    }

    if (!reachable) {
        LogWarn("Unable to get time!\n");
        return -1;
    }

    if (!callback_received) {
        LogWarn("No callback was received from SNTP module. Ensure that iotc_set_system_time_us is defined as SNTP_SET_SYSTEM_TIME_US callback!\n");
        return -1;
    }

    timenow = time(NULL);
    LogInfo("Time received from NTP. Time now: %s!\n", iotcl_to_iso_timestamp(timenow));

    return 0;
}



bool is_sntp_time_synced(void)
{
	return callback_received;
}


void sntp_task( void * pvParameters )
{
	LogInfo("Started SNTP task, wait for connection to network");

	while (xSystemEvents == NULL) {
		vTaskDelay(100);
	}

    ( void ) xEventGroupWaitBits( xSystemEvents,
                                  EVT_MASK_NET_CONNECTED,
                                  0x00,
                                  pdTRUE,
                                  portMAX_DELAY );

	LogInfo("syncing time using SNTP");

    iotc_stm_aws_time_obtain(SNTP_SERVER_NAME);

    while(1) {
    	vTaskDelay(5000);
    }
}

