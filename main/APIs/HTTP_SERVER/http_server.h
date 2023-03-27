/*
 * http_server.h
 *
 *  Created on: Apr 04, 2022
 *      Author: Juan Sebastian Giraldo Duque
 */

#ifndef MAIN_HTTP_SERVER_H_
#define MAIN_HTTP_SERVER_H_

#define OTA_UPDATE_PENDING 		0
#define OTA_UPDATE_SUCCESSFUL	1
#define OTA_UPDATE_FAILED		-1

// HTTP Server task
#define HTTP_SERVER_TASK_STACK_SIZE			10240
#define HTTP_SERVER_TASK_PRIORITY			21
#define HTTP_SERVER_TASK_CORE_ID			1

// HTTP Server Monitor task
#define HTTP_SERVER_MONITOR_STACK_SIZE		4096
#define HTTP_SERVER_MONITOR_PRIORITY		3
#define HTTP_SERVER_MONITOR_CORE_ID			1

/**
 * Connection status for Wifi
 */
typedef enum http_server_wifi_connect_status
{
	NONE = 0,
	HTTP_WIFI_STATUS_CONNECTING,
	HTTP_WIFI_STATUS_CONNECT_FAILED,
	HTTP_WIFI_STATUS_CONNECT_SUCCESS,
	HTTP_WIFI_STATUS_DISCONNECTED,
} http_server_wifi_connect_status_e;

/**
 * Messages for the HTTP monitor
 */
typedef enum http_server_message
{
	HTTP_MSG_WIFI_CONNECT_INIT = 0,
	HTTP_MSG_WIFI_CONNECT_SUCCESS,
	HTTP_MSG_WIFI_CONNECT_FAIL,
	HTTP_MSG_WIFI_USER_DISCONNECT,
	HTTP_MSG_OTA_UPDATE_SUCCESSFUL,
	HTTP_MSG_OTA_UPDATE_FAILED,
	HTTP_MSG_TIME_SERVICE_INITIALIZED,
} http_server_message_e;

/**
 * Structure for the message queue
 */
typedef struct http_server_queue_message
{
	http_server_message_e msgID;
} http_server_queue_message_t;

/**
 * Sends a message to the queue
 * @param msgID message ID from the http_server_message_e enum.
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE.
 * @note Expand the parameter list based on your requirements e.g. how you've expanded the http_server_queue_message_t.
 */
BaseType_t http_server_monitor_send_message(http_server_message_e msgID);

/**
 * Starts the HTTP server.
 */
void http_server_start(void);

/**
 * Stops the HTTP server.
 */
void http_server_stop(void);

/**
 * Timer callback function which calls esp_restart upon successful firmware update.
 */
void http_server_fw_update_reset_callback(void *arg);


/**
 * @brief Sent menssage to client from server.
 * 
 */

void ws_print(void *pvParameters);

int http_websocket_vprintf(const char *format, va_list args);


void log_for_websocket_setup(void);

void http_ws_server_send_messages(char * data);

void http_server_set_connect_status(http_server_wifi_connect_status_e wifi_connect_status);

#endif /* MAIN_HTTP_SERVER_H_ */
