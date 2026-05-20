#ifndef NEU_WIFI_EVENT_H
#define NEU_WIFI_EVENT_H

#include <cstdint>
#include "esp_wifi.h"

/**
 * @class NeuWiFiEvent
 * @brief Unified wrapper for ESP-IDF WiFi event system.
 *
 * This class provides a high-level abstraction over raw ESP-IDF WiFi events,
 * allowing structured access to event data such as connection state,
 * SSID, RSSI, IP address, scan results, and disconnect reasons.
 *
 * It is designed to decouple application logic from ESP-IDF event structs
 * while maintaining zero-copy access where possible.
 */
class NeuWiFiEvent
{
private:
    int32_t _id; /**< Event ID from ESP-IDF */
    void *_raw;  /**< Raw pointer to ESP-IDF event data */

    static char _ip_str_buffer[16];   /**< Internal buffer for IP string */
    static char _ssid_str_buffer[33]; /**< Internal buffer for SSID string */
    static char _mac_str_buffer[18];  /**< Internal buffer for MAC string */

public:
    /**
     * @brief Construct event wrapper from ESP-IDF event data.
     *
     * @param id Event ID (WIFI_EVENT / IP_EVENT)
     * @param data Raw event pointer from ESP-IDF callback
     */
    NeuWiFiEvent(int32_t id, void *data);

    /**
     * @brief Check if station is connected.
     * @return true if connected event
     */
    bool isConnected();

    /**
     * @brief Check if station is disconnected.
     * @return true if disconnected event
     */
    bool isDisconnected();

    /**
     * @brief Check if WiFi scan process is completed.
     * @return true if scan done event
     */
    bool isScanningDone();

    /**
     * @brief Check if SoftAP is active.
     * @return true if AP started event
     */
    bool isHotspotActive();

    /**
     * @brief Get human-readable event name.
     * @return Event name string
     */
    const char *getEventName();

    /**
     * @brief Get disconnect reason code from ESP-IDF.
     * @return Reason code integer
     */
    int getReasonCode();

    /**
     * @brief Get SSID associated with event.
     * @return SSID string
     */
    const char *getSSID();

    /**
     * @brief Get WiFi channel.
     * @return Channel number
     */
    int getChannel();

    /**
     * @brief Get RSSI signal strength.
     * @return RSSI value (dBm)
     */
    int8_t getRSSI();

    /**
     * @brief Get assigned IP address as string.
     * @return IP address string
     */
    const char *getIPAddress();

    /**
     * @brief Get disconnect reason code (extended version).
     * @return ESP-IDF disconnect reason
     */
    int getDisconnectReasonCode();

    /**
     * @brief Get human-readable disconnect reason text.
     * @return Reason string
     */
    const char *getDisconnectReasonText();

    /**
     * @brief Get MAC address of connected peer.
     * @return MAC string
     */
    const char *getConnectedMacAddress();

    /**
     * @brief Get Association ID (AID).
     * @return AID value
     */
    int getAID();

    /**
     * @brief Get number of scan results.
     * @return Scan count
     */
    int getScanCount();

    /**
     * @brief Get raw scan status value.
     * @return Scan status bitmap
     */
    uint32_t getScanStatus();

    /**
     * @brief Get lowest RSSI detected during scan.
     * @return RSSI threshold value
     */
    int32_t getBssLowRSSI();
};

#endif