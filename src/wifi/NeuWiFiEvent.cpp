/**
 * @file NeuWiFiEvent.cpp
 * @brief Implementation of unified ESP-IDF WiFi event wrapper.
 *
 * This file translates raw ESP-IDF WiFi/IP events into a structured,
 * human-readable abstraction layer.
 *
 * It provides:
 * - Event classification
 * - Safe parsing of raw event structs
 * - String conversion utilities (SSID, IP, MAC)
 * - Reason decoding for disconnect events
 */

#include "wifi/NeuWiFiEvent.h"
#include <cstring>
#include <cstdio>

char NeuWiFiEvent::_ip_str_buffer[16] = {0};
char NeuWiFiEvent::_ssid_str_buffer[33] = {0};
char NeuWiFiEvent::_mac_str_buffer[18] = {0};

#if defined(CONFIG_IDF_TARGET_ESP32) || defined(ESP32)
#define NEU_HAS_BSS_RSSI_LOW 1
#else
#define NEU_HAS_BSS_RSSI_LOW 0
#endif

NeuWiFiEvent::NeuWiFiEvent(int32_t id, void *data)
    : _id(id), _raw(data) {}

int NeuWiFiEvent::getScanCount()
{
    if (_id == WIFI_EVENT_SCAN_DONE && _raw != nullptr)
        return ((wifi_event_sta_scan_done_t *)_raw)->number;

    return 0;
}

int NeuWiFiEvent::getReasonCode()
{
    if (_id == WIFI_EVENT_STA_DISCONNECTED && _raw != nullptr)
        return ((wifi_event_sta_disconnected_t *)_raw)->reason;

    return 0;
}

bool NeuWiFiEvent::isConnected() { return (_id == IP_EVENT_STA_GOT_IP); }

bool NeuWiFiEvent::isDisconnected() { return (_id == WIFI_EVENT_STA_DISCONNECTED); }

bool NeuWiFiEvent::isScanningDone() { return (_id == WIFI_EVENT_SCAN_DONE); }

bool NeuWiFiEvent::isHotspotActive() { return (_id == WIFI_EVENT_AP_START); }

const char *NeuWiFiEvent::getEventName()
{
    switch (_id)
    {
    case WIFI_EVENT_SCAN_DONE:
        return "SCAN_DONE";
    case WIFI_EVENT_STA_START:
        return "STA_START";
    case WIFI_EVENT_STA_CONNECTED:
        return "STA_CONNECTED";
    case WIFI_EVENT_STA_DISCONNECTED:
        return "STA_DISCONNECTED";
    case WIFI_EVENT_AP_START:
        return "AP_START";
    case WIFI_EVENT_AP_STACONNECTED:
        return "AP_STACONNECTED";
    case WIFI_EVENT_AP_STADISCONNECTED:
        return "AP_STADISCONNECTED";
    case IP_EVENT_STA_GOT_IP:
        return "IP_STA_GOT_IP";
    case WIFI_EVENT_STA_BSS_RSSI_LOW:
        return "STA_BSS_RSSI_LOW";
    default:
        return "OTHER_WIFI_EVENT";
    }
}

uint32_t NeuWiFiEvent::getScanStatus()
{
    if (_id == WIFI_EVENT_SCAN_DONE && _raw != nullptr)
        return ((wifi_event_sta_scan_done_t *)_raw)->status;

    return 1;
}

const char *NeuWiFiEvent::getSSID()
{
    std::memset(_ssid_str_buffer, 0, sizeof(_ssid_str_buffer));

    if (_id == WIFI_EVENT_STA_CONNECTED && _raw != nullptr)
    {
        auto *info = (wifi_event_sta_connected_t *)_raw;
        uint8_t len = (info->ssid_len > 32) ? 32 : info->ssid_len;
        std::memcpy(_ssid_str_buffer, info->ssid, len);
        return _ssid_str_buffer;
    }
    else if (_id == WIFI_EVENT_STA_DISCONNECTED && _raw != nullptr)
    {
        auto *info = (wifi_event_sta_disconnected_t *)_raw;
        uint8_t len = (info->ssid_len > 32) ? 32 : info->ssid_len;
        std::memcpy(_ssid_str_buffer, info->ssid, len);
        return _ssid_str_buffer;
    }

    return "";
}

int NeuWiFiEvent::getChannel()
{
    if (_id == WIFI_EVENT_STA_CONNECTED && _raw != nullptr)
        return ((wifi_event_sta_connected_t *)_raw)->channel;

    return 0;
}

int NeuWiFiEvent::getDisconnectReasonCode()
{
    if (_id == WIFI_EVENT_STA_DISCONNECTED && _raw != nullptr)
        return ((wifi_event_sta_disconnected_t *)_raw)->reason;

    return 0;
}

const char *NeuWiFiEvent::getDisconnectReasonText()
{
    int reason = getDisconnectReasonCode();

    switch (reason)
    {
    case 201:
        return "SSID not found (NO_AP_FOUND)";
    case 15:
        return "4-way handshake timeout (possible wrong password)";
    case 202:
        return "Authentication failure";
    case 3:
        return "User initiated disconnect";
    default:
        return "Connection interrupted or router issue";
    }
}

int8_t NeuWiFiEvent::getRSSI()
{
    if (_id == WIFI_EVENT_STA_DISCONNECTED && _raw != nullptr)
        return ((wifi_event_sta_disconnected_t *)_raw)->rssi;

    return 0;
}

const char *NeuWiFiEvent::getIPAddress()
{
    if (_id == IP_EVENT_STA_GOT_IP && _raw != nullptr)
    {
        auto *ip_info = (ip_event_got_ip_t *)_raw;
        esp_ip4addr_ntoa(&ip_info->ip_info.ip, _ip_str_buffer, sizeof(_ip_str_buffer));
        return _ip_str_buffer;
    }
    return "0.0.0.0";
}

const char *NeuWiFiEvent::getConnectedMacAddress()
{
    if (_raw == nullptr)
        return "00:00:00:00:00:00";

    uint8_t *mac_ptr = nullptr;

    if (_id == WIFI_EVENT_AP_STACONNECTED)
        mac_ptr = ((wifi_event_ap_staconnected_t *)_raw)->mac;
    else if (_id == WIFI_EVENT_AP_STADISCONNECTED)
        mac_ptr = ((wifi_event_ap_stadisconnected_t *)_raw)->mac;
    else if (_id == WIFI_EVENT_AP_PROBEREQRECVED)
        mac_ptr = ((wifi_event_ap_probe_req_rx_t *)_raw)->mac;

    if (mac_ptr)
    {
        std::snprintf(_mac_str_buffer, sizeof(_mac_str_buffer),
                      "%02X:%02X:%02X:%02X:%02X:%02X",
                      mac_ptr[0], mac_ptr[1], mac_ptr[2],
                      mac_ptr[3], mac_ptr[4], mac_ptr[5]);

        return _mac_str_buffer;
    }

    return "00:00:00:00:00:00";
}

int NeuWiFiEvent::getAID()
{
    if (_id == WIFI_EVENT_AP_STACONNECTED && _raw != nullptr)
        return ((wifi_event_ap_staconnected_t *)_raw)->aid;

    else if (_id == WIFI_EVENT_AP_STADISCONNECTED && _raw != nullptr)
        return ((wifi_event_ap_stadisconnected_t *)_raw)->aid;

    return 0;
}

int32_t NeuWiFiEvent::getBssLowRSSI()
{
#if NEU_HAS_BSS_RSSI_LOW

    if (_id == WIFI_EVENT_STA_BSS_RSSI_LOW && _raw != nullptr)
    {
        auto *evt = (wifi_event_bss_rssi_low_t *)_raw;
        return evt->rssi;
    }

#endif

    return 0;
}