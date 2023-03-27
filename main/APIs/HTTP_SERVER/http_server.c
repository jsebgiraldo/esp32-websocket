/*
 * http_server.c
 *
 *  Created on: Oct 20, 2021
 *      Author: kjagu
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_ota_ops.h"
#include "esp_wifi.h"
#include "sys/param.h"


#include "http_server.h"
#include "wifi_app.h"

#include <stdio.h>
#include <stdlib.h>

static const char TAG[] = "[http_server]";

#define HTTP_DEBUG_ENABLE
#ifdef HTTP_DEBUG_ENABLE
// Tag used for ESP serial console messages
	#define HTTP_DEBUG(...) ESP_LOGI(TAG,LOG_COLOR(LOG_COLOR_PURPLE) __VA_ARGS__)
#else
	#define HTTP_DEBUG(...)
#endif

// Max number clients for websocket communication.
static const size_t max_clients = 4;

// Wifi connect status
static int g_wifi_connect_status = NONE;

// Firmware update status
static int g_fw_update_status = OTA_UPDATE_PENDING;

// Local Time status
static bool g_is_local_time_set = false;


// HTTP server task handle
static httpd_handle_t http_server_handle = NULL;

// HTTP server monitor task handle
static TaskHandle_t task_http_server_monitor = NULL;

// Queue handle used to manipulate the main queue of events
static QueueHandle_t http_server_monitor_queue_handle;

/**
 * ESP32 timer configuration passed to esp_timer_create.
 */
const esp_timer_create_args_t fw_update_reset_args = {
		.callback = &http_server_fw_update_reset_callback,
		.arg = NULL,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "fw_update_reset"
};
esp_timer_handle_t fw_update_reset;

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;

	char *data;
};
	

void http_ws_server_send_messages(char * data)
{

	if (!http_server_handle) { // httpd might not have been created by now
		return;
	}

	size_t clients = max_clients;
    int    client_fds[max_clients];
	if (httpd_get_client_list(http_server_handle, &clients, client_fds) == ESP_OK) 
	{
		    for (size_t i=0; i < clients; ++i) 
			{
                int sock = client_fds[i];
                if (httpd_ws_get_fd_info(http_server_handle, sock) == HTTPD_WS_CLIENT_WEBSOCKET) {
                 
                    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
                    resp_arg->hd = http_server_handle;
                    resp_arg->fd = sock;
					resp_arg->data = data;


					httpd_handle_t hd = resp_arg->hd;
					int fd = resp_arg->fd;
					const char * data = resp_arg->data;;
					httpd_ws_frame_t ws_pkt;
					memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
					ws_pkt.payload = (uint8_t*)data;
					ws_pkt.len = strlen(data);
					ws_pkt.type = HTTPD_WS_TYPE_TEXT;

					httpd_ws_send_frame_async(hd, fd, &ws_pkt);
					free(resp_arg);

                }
            }
	} 
	else 
	{
        ESP_LOGE(TAG, "httpd_get_client_list failed!");
        return;
    }

}


/**
 * Checks the g_fw_update_status and creates the fw_update_reset timer if g_fw_update_status is true.
 */
static void http_server_fw_update_reset_timer(void)
{
	if (g_fw_update_status == OTA_UPDATE_SUCCESSFUL)
	{
		HTTP_DEBUG("http_server_fw_update_reset_timer: FW updated successful starting FW update reset timer");

		// Give the web page a chance to receive an acknowledge back and initialize the timer
		ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
		ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000));
	}
	else
	{
		HTTP_DEBUG("http_server_fw_update_reset_timer: FW update unsuccessful");
	}
}


void http_server_set_connect_status(http_server_wifi_connect_status_e wifi_connect_status)
{
	g_wifi_connect_status = wifi_connect_status;
}

/**
 * HTTP server monitor task used to track events of the HTTP server
 * @param pvParameters parameter which can be passed to the task.
 */
static void http_server_monitor(void *parameter)
{
	http_server_queue_message_t msg;

	for (;;)
	{
		if (xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
		{
			switch (msg.msgID)
			{
				case HTTP_MSG_WIFI_CONNECT_INIT:
					HTTP_DEBUG("HTTP_MSG_WIFI_CONNECT_INIT");
		
					http_server_set_connect_status(HTTP_WIFI_STATUS_CONNECTING);

					break;

				case HTTP_MSG_WIFI_CONNECT_SUCCESS:
					HTTP_DEBUG("HTTP_MSG_WIFI_CONNECT_SUCCESS");

					http_server_set_connect_status(HTTP_WIFI_STATUS_CONNECT_SUCCESS);

					break;

				case HTTP_MSG_WIFI_CONNECT_FAIL:
					HTTP_DEBUG("HTTP_MSG_WIFI_CONNECT_FAIL");

					http_server_set_connect_status(HTTP_WIFI_STATUS_CONNECT_FAILED);

					break;

				case HTTP_MSG_WIFI_USER_DISCONNECT:
					HTTP_DEBUG("HTTP_MSG_WIFI_USER_DISCONNECT");

					http_server_set_connect_status(HTTP_WIFI_STATUS_DISCONNECTED);

					break;

				case HTTP_MSG_OTA_UPDATE_SUCCESSFUL:
					HTTP_DEBUG("HTTP_MSG_OTA_UPDATE_SUCCESSFUL");
					g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
					http_server_fw_update_reset_timer();

					break;

				case HTTP_MSG_OTA_UPDATE_FAILED:
					HTTP_DEBUG("HTTP_MSG_OTA_UPDATE_FAILED");
					g_fw_update_status = OTA_UPDATE_FAILED;

					break;

				case HTTP_MSG_TIME_SERVICE_INITIALIZED:
					HTTP_DEBUG("HTTP_MSG_TIME_SERVICE_INITIALIZED");
					g_is_local_time_set = true;

					break;

				default:
					break;
			}
		}
	}
}


/*
 * This handler echos back the received ws data
 * and triggers an async send if certain message received
 */
static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    }
    return ret;
}

/**
 * Sets up the default httpd server configuration.
 * @return http server instance handle if successful, NULL otherwise.
 */
static httpd_handle_t http_server_configure(void)
{
	// Generate the default configuration
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	// Create HTTP server monitor task
	xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_MONITOR_PRIORITY, &task_http_server_monitor,HTTP_SERVER_MONITOR_CORE_ID);

	// Create the message queue
	http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));

	// The core that the HTTP server will run on
	config.core_id = HTTP_SERVER_TASK_CORE_ID;

	// Adjust the default priority to 1 less than the wifi application task
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;

	// Bump up the stack size (default is 4096)
	config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;

	// Increase uri handlers
	config.max_uri_handlers = 25;

	// Increase the timeout limits
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;

	config.max_open_sockets = max_clients;

	HTTP_DEBUG("http_server_configure: Starting server on port: '%d' with task priority: '%d'",
			config.server_port,
			config.task_priority);

	// Start the httpd server
	if (httpd_start(&http_server_handle, &config) == ESP_OK)
	{
		HTTP_DEBUG("http_server_configure: Registering URI handlers");

		httpd_uri_t ws = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = ws_handler,
        .user_ctx   = NULL,
        .is_websocket = true,
		.handle_ws_control_frames = true
		};
		httpd_register_uri_handler(http_server_handle, &ws);
	
		return http_server_handle;
	}

	return NULL;
}

void http_server_start(void)
{
	if (http_server_handle == NULL)
	{
		http_server_handle = http_server_configure();
	}
}


void http_server_stop(void)
{

	wifi_sta_list_t wifi_sta_list; 
	esp_wifi_ap_get_sta_list(&wifi_sta_list);

	if(wifi_sta_list.num == false) // Only stop server if it's connected one client.
	{
		if (http_server_handle)
		{
			httpd_stop(http_server_handle);
			HTTP_DEBUG("http_server_stop: stopping HTTP server");
			http_server_handle = NULL;
		}
		if (task_http_server_monitor)
		{
			vTaskDelete(task_http_server_monitor);
			HTTP_DEBUG("http_server_stop: stopping HTTP server monitor");
			task_http_server_monitor = NULL;
		}
	}
	
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
	
	http_server_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
	
}

void http_server_fw_update_reset_callback(void *arg)
{
	HTTP_DEBUG("http_server_fw_update_reset_callback: Timer timed-out, restarting the device");
	esp_restart();
}


// ------------------------------------------ * Websocket functions * --------------------------------

static QueueHandle_t ws_queue = NULL;


void ws_print(void *pvParameters)
{
	char buffer[255];
	
	for(;;)
	{
		if(xQueueReceive(ws_queue, &(buffer), (portTickType)portMAX_DELAY))
		{
			http_ws_server_send_messages(buffer);
		}
	}
	
}


int http_websocket_vprintf(const char *format, va_list args)
{
	static char vprintf_buff[255];
	int written = vsnprintf(vprintf_buff, sizeof(vprintf_buff), format, args);
	if (written > 0)
	{
		
		if(ws_queue != NULL)
		{
			xQueueSend( ws_queue, ( void * ) &vprintf_buff, ( TickType_t ) 0 );
		}
		printf(vprintf_buff);
	}
	return written;
}

void log_for_websocket_setup(void)
{
	ws_queue = xQueueCreate(50, sizeof(char)*255);

	xTaskCreatePinnedToCore(ws_print, "websocket", 2048, NULL, 12, NULL,0);

	esp_log_set_vprintf(http_websocket_vprintf); 
}
