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

![extension](\doc\img\websocket_ext.PNG)

### How use

Through an STA enters the server's AP.

![ssid](\doc\img\ssid.PNG)

* **URL:** Enter the IP of the http server with the URI **/ws** and press **Open**.

![extension](\doc\img\ip_gateway.PNG) ![extension](\doc\img\ws_uri.PNG)


* **REQUEST:** Enter a message to the server.
* **MESSAGE LOG:** Here you will receive the server's response.

![extension](\doc\img\websocket_client.PNG)



