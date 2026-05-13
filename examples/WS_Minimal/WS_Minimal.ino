#define USE_HTTP_SERVER
#include <NeuWiFi.h>

NeuWiFi wifi;
NeuHttpServer server;

// --- WebSocket handler (simple function style) ---
void ws_handler(int id, NeuWSEvent event, const char *payload, size_t len)
{
    switch (event)
    {
    case WS_EVENT_CONNECT:
        Serial.println("Client connected");
        server.wsSendText(id, "hello from ESP32");
        break;

    case WS_EVENT_TEXT:
        Serial.print("RX: ");
        Serial.println(payload);

        // simple echo
        server.wsSendText(id, payload);
        break;

    case WS_EVENT_DISCONNECT:
        Serial.println("Client disconnected");
        break;

    default:
        break;
    }
}

void setup()
{
    Serial.begin(115200);

    wifi.init();
    wifi.startAP("NeuWS", "12345678");

    server.init(80);

    // WebSocket endpoint
    server.onWS("/ws", ws_handler);

    Serial.println("WS Minimal Ready");
    Serial.println(wifi.getAPIP());
}

void loop()
{
    // fully event-driven, no polling required
}