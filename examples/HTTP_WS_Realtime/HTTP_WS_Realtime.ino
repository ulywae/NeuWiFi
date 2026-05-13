#define USE_HTTP_SERVER
#include <NeuWiFi.h>

NeuWiFi wifi;
NeuHttpServer server;

const char *html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>NeuWiFi WS Demo</title>
</head>
<body>
<h2>NeuWiFi Realtime WS</h2>

<input id="msg" placeholder="type..." />
<button onclick="send()">Send</button>

<pre id="log"></pre>

<script>
const ws = new WebSocket("ws://" + location.host + "/ws");

ws.onopen = () => log("connected");
ws.onmessage = (e) => log("rx: " + e.data);

function send() {
  const msg = document.getElementById("msg").value;
  ws.send(msg);
  log("tx: " + msg);
}

function log(t) {
  document.getElementById("log").innerText += t + "\n";
}
</script>

</body>
</html>
)rawliteral";

void wsHandler(int id, NeuWSEvent event, const char *payload, size_t len)
{
    switch (event)
    {
    case WS_EVENT_CONNECT:
        Serial.println("WS Client Connected");
        server.wsSendText(id, "welcome");
        break;

    case WS_EVENT_TEXT:
        Serial.print("WS RX: ");
        Serial.println(payload);

        // echo + transform realtime
        server.wsSendText(id, payload);
        break;

    case WS_EVENT_DISCONNECT:
        Serial.println("WS Client Disconnected");
        break;

    default:
        break;
    }
}

void setup()
{
    Serial.begin(115200);

    wifi.init();
    wifi.startAP("NeuDemo", "12345678");

    server.init(80);

    // HTTP endpoint simple
    server.on("/", HTTP_GET, [](httpd_req_t *req)
              { NeuHttpServer::sendText(req, html); });

    // WebSocket endpoint
    server.onWS("/ws", wsHandler);

    Serial.println("HTTP + WS Ready");
    Serial.println(wifi.getAPIP());
}

void loop()
{
    // fully event-driven, no polling needed
}