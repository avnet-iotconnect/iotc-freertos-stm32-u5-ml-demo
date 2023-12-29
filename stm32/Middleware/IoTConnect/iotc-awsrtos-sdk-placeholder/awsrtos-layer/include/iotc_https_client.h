/*
 * iotc_https_client.h
 *
 *  Created on: Nov 3, 2023
 *      Author: mgilhespie
 */

#ifndef LIB_IOTC_AWSRTOS_SDK_PLACEHOLDER_INCLUDE_IOTC_HTTPS_CLIENT_H_
#define LIB_IOTC_AWSRTOS_SDK_PLACEHOLDER_INCLUDE_IOTC_HTTPS_CLIENT_H_


typedef struct IotConnectHttpResponse {
    char *data; // add flexibility for future, but at this point we only have response data
} IotConnectHttpResponse;


void iotconnect_https_init(PkiObject_t root_ca);

// Helper to deal with http chunked transfers which are always returned by iotconnect services.
// Free data with iotconnect_free_https_response
/*
unsigned int iotconnect_https_request(
        IotConnectHttpResponse* response,
        const char *host,
        const char *path,
        const char *send_str
);
*/

int32_t iotc_send_http_request(IotConnectHttpResponse *iotc_response,
						  const char *server_host, int port,
		                  const char *method, const char *path,
						  char *user_buffer, size_t user_buffer_len);


void iotconnect_free_https_response(IotConnectHttpResponse* response);


#endif /* LIB_IOTC_AWSRTOS_SDK_PLACEHOLDER_INCLUDE_IOTC_HTTPS_CLIENT_H_ */
