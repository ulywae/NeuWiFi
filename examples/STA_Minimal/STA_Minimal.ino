#include <NeuWiFi.h>

NeuWiFi wifi;

void setup()
{
    Serial.begin(115200);

    wifi.init();
    wifi.setHostname("neu-device");

    bool ok = wifi.startSTA("YourSSID", "YourPASS", 10000);

    if (ok)
    {
        Serial.println("WiFi connected!");
        Serial.println(wifi.getSTAIP());
    }
    else
    {
        Serial.println("WiFi failed to connect");
    }
}

void loop()
{
    // event-driven core, no blocking logic needed
}