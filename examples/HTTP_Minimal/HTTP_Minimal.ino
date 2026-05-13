#define USE_HTTP_SERVER
#include <NeuWiFi.h>

NeuWiFi wifi;
NeuHttpServer server;

// Simple HTTP handler
void handle_root(httpd_req_t *req)
{
    const char *html =
        "<h1>NeuWiFi HTTP Minimal</h1>"
        "<p>ESP32 Web Server is running.</p>";

    NeuHttpServer::sendText(req, html);
}

void setup()
{
    Serial.begin(115200);

    wifi.init();
    wifi.startAP("NeuHTTP", "12345678");

    server.init(80);

    // Register route
    server.on("/", HTTP_GET, handle_root);

    Serial.println("HTTP Server Ready");
    Serial.println(wifi.getAPIP());
}

void loop()
{
    // event-driven, no polling needed
}