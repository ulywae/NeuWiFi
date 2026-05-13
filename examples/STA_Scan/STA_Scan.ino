#include <NeuWiFi.h>

NeuWiFi wifi;

WiFiScanResult results[10];

void setup()
{
    Serial.begin(115200);

    wifi.init();

    Serial.println("Scanning...");

    wifi.startScan();
    delay(3000);

    int n = wifi.getScanResults(results, 10);

    for (int i = 0; i < n; i++)
    {
        Serial.print(results[i].ssid);
        Serial.print(" | RSSI: ");
        Serial.println(results[i].rssi);
    }
}

void loop() {}