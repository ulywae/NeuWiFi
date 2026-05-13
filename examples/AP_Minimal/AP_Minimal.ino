#include <NeuWiFi.h>

NeuWiFi wifi;

void setup()
{
    Serial.begin(115200);

    wifi.init();
    wifi.setHostname("neu-ap");

    wifi.startAP("NeuHotspot", "12345678");

    Serial.println("AP started");
    Serial.println(wifi.getAPIP());
}

void loop() {}