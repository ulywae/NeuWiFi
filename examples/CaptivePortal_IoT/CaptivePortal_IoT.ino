#define USE_HTTP_SERVER
#define USE_DNS_SERVER

#include <NeuWiFi.h>

NeuWiFi wifi;
NeuHttpServer server;
NeuDnsServer dns;

// Simple captive portal page
const char *portal_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Device Setup</title>
</head>
<body>
  <h2>Neu Device Setup</h2>
  <p>Connect your WiFi network</p>

  <form action="/save" method="GET">
    SSID:<br>
    <input name="ssid"><br><br>

    Password:<br>
    <input name="pass" type="password"><br><br>

    <button type="submit">Save</button>
  </form>
</body>
</html>
)rawliteral";

void handle_root(httpd_req_t *req)
{
    NeuHttpServer::sendText(req, portal_html);
}

void handle_save(httpd_req_t *req)
{
    char ssid[32];
    char pass[64];

    NeuHttpServer::getPostParam(req, "ssid", ssid, sizeof(ssid));
    NeuHttpServer::getPostParam(req, "pass", pass, sizeof(pass));

    Serial.print("Saved SSID: ");
    Serial.println(ssid);

    Serial.print("Saved PASS: ");
    Serial.println(pass);

    const char *resp =
        "<h3>Saved!</h3><p>Device will reconnect...</p>";

    NeuHttpServer::sendText(req, resp);
}

void setup()
{
    Serial.begin(115200);

    // Start AP mode (setup network)
    wifi.init();
    wifi.startAP("NeuSetup", "12345678");

    // HTTP server
    server.init(80);
    server.on("/", HTTP_GET, handle_root);
    server.on("/save", HTTP_GET, handle_save);

    // DNS captive portal (redirect all domains to ESP IP)
    dns.start();

    Serial.println("Captive Portal Ready");
    Serial.println(wifi.getAPIP());
}

void loop()
{
    // fully event-driven (no polling required)
}