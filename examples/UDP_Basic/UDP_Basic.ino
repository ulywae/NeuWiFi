#include <NeuWiFi.h>

NeuWiFi wifi;
NeuUDP udp;

void onPacket(uint8_t *data, size_t len, const char *ip, uint16_t port)
{
    Serial.print("From: ");
    Serial.println(ip);

    Serial.write(data, len);
    Serial.println();
}

void setup()
{
    Serial.begin(115200);

    wifi.init();
    wifi.startSTA("YourSSID", "YourPASS", 10000);

    udp.begin(8888);
    udp.setCallback(onPacket);

    udp.send("192.168.1.10", 8888, (uint8_t *)"hello", 5);
}

void loop()
{
    udp._run(); // kalau kamu belum pakai task internal di wrapper
}