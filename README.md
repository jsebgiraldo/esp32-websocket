# Websocket application

(See the README.md file in the upper level 'examples' directory for more information about examples.)
This example will shows how to set up and communicate over a websocket.

## How to Use Example

### Hardware Required

This example can be executed on any ESP32 board, the only required interface is WiFi and connection to internet or a local server.

### Configure the project

* Open the project configuration menu (`idf.py menuconfig`)
* Enable Component Config / HTTP Server / WebSocket Server support

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output

### Extensions used

For this example we used the "WebSocket Test Client" extension available at https://chrome.google.com/webstore/detail/websocket-test-client/fgponpodhbmadfljofbimhhlengambbn?hl=en

![extension](/doc/img/websocket_ext.PNG)

### How use

Through an STA enters the server's AP.

```c
// WiFi application settings
#define WIFI_AP_SSID				"WEBSOCKET"			// AP name
#define WIFI_AP_PASSWORD			"123456789"			// AP password

```

* **URL:** Enter the IP of the http server with the URI **/ws** and press **Open**.

```c
#define WIFI_AP_IP					"192.168.5.1"		// AP default IP
#define WIFI_AP_GATEWAY				"192.168.5.1"		// AP default Gateway (should be the same as the IP)
#define WIFI_AP_NETMASK				"255.255.255.0"		// AP netmask
```
<br>

* **END POINT :**

```c
httpd_uri_t ws = {
    .uri        = "/ws",
    .method     = HTTP_GET,
    .handler    = ws_handler,
    .user_ctx   = NULL,
    .is_websocket = true,
    .handle_ws_control_frames = true
};
httpd_register_uri_handler(http_server_handle, &ws);
```

* **REQUEST:** Enter a message to the server.
* **MESSAGE LOG:** Here you will receive the server's response.

![extension](/doc/img/websocket_client.PNG)



