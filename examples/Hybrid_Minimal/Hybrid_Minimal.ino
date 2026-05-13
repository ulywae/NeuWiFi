#include <NeuWiFi.h>

NeuWiFi wifi;

void setup()
{
    Serial.begin(115200);

    wifi.init();

    wifi.startHybrid(
        "YourSSID",
        "YourPASS",
        "NeuAP",
        "12345678");

    Serial.println("Hybrid mode active");
}

void loop() {}