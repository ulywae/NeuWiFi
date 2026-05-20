/**
 * @file _NeuWiFi.h
 * @brief Deterministic WiFi runtime engine for ESP32 systems.
 *
 * NeuWiFiClass provides a lightweight runtime-oriented WiFi abstraction
 * designed for embedded systems requiring:
 *
 * - deterministic lifecycle management
 * - STA / AP / Hybrid operation
 * - automatic reconnect orchestration
 * - static IP configuration
 * - smart AP channel tracking
 * - event-driven networking
 * - low-overhead runtime behavior
 *
 * Architecture Goals:
 * - Centralized WiFi orchestration
 * - Safe runtime teardown
 * - Self-contained reconnect engine
 * - Minimal hidden side effects
 * - Embedded production stability
 *
 * Supported Modes:
 * - Station (STA)
 * - Access Point (AP)
 * - Hybrid STA + AP
 *
 * Runtime Features:
 * - Auto reconnect task
 * - Event callback system
 * - Static IP + DNS support
 * - AP channel synchronization
 * - Scan engine
 * - Shared state integration
 *
 * Platform:
 * - ESP32 / ESP-IDF
 * - Arduino-ESP32 compatible
 */

#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>
#include <cstdint>

#include "wifi/NeuWiFiEvent.h"
#include "NeuWiFiTypes.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_SCAN_DONE_BIT BIT2

/**
 * @brief User WiFi event callback function.
 *
 * @param event_id Native ESP-IDF event identifier.
 * @param event Translated NeuWiFi event structure.
 */
typedef void (*WiFiCallback)(int32_t event_id, NeuWiFiEvent event);

/**
 * @class NeuWiFiClass
 * @brief Deterministic ESP32 WiFi runtime engine.
 *
 * Provides centralized management for:
 * - STA connection
 * - AP hosting
 * - hybrid WiFi operation
 * - reconnect orchestration
 * - WiFi scanning
 * - runtime network state
 *
 * Design Philosophy:
 * - Constructor performs no hardware action
 * - init() activates runtime resources
 * - stop() performs centralized teardown
 * - destructor acts as safety cleanup
 *
 * Runtime Safety:
 * - reconnect task guarded
 * - event handlers safely unregistered
 * - network interfaces safely destroyed
 * - reusable after stop()
 */
class NeuWiFiClass
{
private:
    /**
     * @brief Internal ESP-IDF event dispatcher.
     *
     * Converts native ESP events into NeuWiFi runtime events.
     *
     * @param arg User context pointer.
     * @param base Event base.
     * @param id Event identifier.
     * @param data Native event payload.
     */
    static void eventHandler(
        void *arg,
        esp_event_base_t base,
        int32_t id,
        void *data);

    esp_netif_t *_sta_netif = nullptr;
    esp_netif_t *_ap_netif = nullptr;
    EventGroupHandle_t _event_group = nullptr;
    bool _use_static_sta = false;
    bool _use_static_ap = false;

    static char _sta_ip_static_buffer[16];
    static char _ap_ip_static_buffer[16];

    esp_netif_ip_info_t _static_ip_sta;
    esp_netif_ip_info_t _static_ip_ap;

    esp_netif_dns_info_t _dns_primary_sta;
    esp_netif_dns_info_t _dns_secondary_sta;

    wifi_auth_mode_t _min_auth_sta = WIFI_AUTH_WPA2_PSK;

    char _hostname[32] = "Neu-Device";
    bool _auto_reconnect = true;

    uint32_t _reconnect_attempt = 0;
    TaskHandle_t _reconnect_task_handle = nullptr;
    static void reconnectTask(void *pvParameters);

    uint8_t _max_conn;
    uint8_t _ap_channel = 1;
    bool _smart_channel_tracking = true;

    /**
     * @brief Synchronize AP channel to STA network.
     *
     * Useful during hybrid mode operation.
     *
     * @param target_channel Desired WiFi channel.
     */
    void syncChannelToAP(uint8_t target_channel);

    WiFiCallback _callback = nullptr;

public:
    /**
     * @brief Construct a new NeuWiFiClass object.
     *
     * Constructor performs no hardware initialization.
     */
    NeuWiFiClass();

    /**
     * @brief Initialize WiFi runtime engine.
     *
     * Creates runtime infrastructure and event systems.
     *
     * @param max_conn Maximum AP client count.
     */
    void init(uint8_t max_conn = 4);

    /**
     * @brief Arduino-style alias for init().
     *
     * @param max_conn Maximum AP client count.
     */
    inline void begin(uint8_t max_conn = 4) { init(max_conn); }

    /**
     * @brief Stop and cleanup WiFi runtime.
     *
     * Safely destroys:
     * - tasks
     * - event handlers
     * - WiFi interfaces
     * - runtime state
     */
    void stop();

    /**
     * @brief Set the static IP configuration for the STA interface.
     *
     * @param ip Static IP address.
     * @param subnet Subnet mask.
     * @param gateway Gateway IP address.
     * @param dns_primary Primary DNS server IP.
     * @param dns_secondary Secondary DNS server IP.
     * @return true Success.
     * @return false Failed.
     */
    bool setSTAIP(const char *ip,
                  const char *subnet,
                  const char *gateway,
                  const char *dns_primary = "1.1.1.1",
                  const char *dns_secondary = "8.8.8.8");

    /**
     * @brief Configure static STA IP using IPConfig structure.
     *
     * @param config IP configuration.
     * @return true Success.
     * @return false Failed.
     */
    bool setSTAIP(const IPConfig &config);

    /**
     * @brief Configure AP network IP settings.
     *
     * @param ip AP IP address.
     * @param subnet AP subnet mask.
     * @param gateway AP gateway address.
     *
     * @return true Success.
     * @return false Failed.
     */
    bool setAPIP(const char *ip,
                 const char *subnet,
                 const char *gateway);

    /**
     * @brief Configure AP IP using IPConfig structure.
     *
     * @param config AP IP configuration.
     * @return true Success.
     * @return false Failed.
     */
    bool setAPIP(const IPConfig &config);

    /**
     * @brief Set device hostname.
     *
     * @param name Hostname string.
     */
    void setHostname(const char *name);

    /**
     * @brief Enable or disable automatic reconnect.
     *
     * @param enable Reconnect state.
     */
    void setAutoReconnect(bool enable)
    {
        _auto_reconnect = enable;
    }

    /**
     * @brief Set maximum AP client count.
     *
     * @param max Maximum client count.
     */
    void setMaxConnection(uint8_t max)
    {
        _max_conn = max;
    }

    /**
     * @brief Set AP channel.
     *
     * @param channel Channel number.
     */
    void setChannelAP(uint8_t channel);

    /**
     * @brief Set WiFi transmit power.
     *
     * @param dbm Transmit power in dBm.
     */
    void setTxPower(int8_t dbm);

    /**
     * @brief Set minimum STA security mode.
     *
     * @param mode WiFi authentication mode.
     */
    void setMinSecuritySTA(int mode)
    {
        _min_auth_sta = (wifi_auth_mode_t)mode;
    }

    /**
     * @brief Start STA connection.
     *
     * @param ssid Target WiFi SSID.
     * @param pass WiFi password.
     * @param timeout Connection timeout in seconds.
     *
     * @return true Connected successfully.
     * @return false Connection failed.
     */
    bool startSTA(
        const char *ssid,
        const char *pass,
        int timeout = 15);

    /**
     * @brief Start AP mode.
     *
     * @param name AP SSID.
     * @param pass AP password.
     */
    void startAP(
        const char *name,
        const char *pass);

    /**
     * @brief Start hybrid STA + AP mode.
     *
     * @param sSsid STA SSID.
     * @param sPass STA password.
     * @param aName AP SSID.
     * @param aPass AP password.
     */
    void startHybrid(
        const char *sSsid,
        const char *sPass,
        const char *aName,
        const char *aPass);

    /**
     * @brief Start WiFi scan.
     */
    void startScan();

    /**
     * @brief Check whether scan has completed.
     *
     * @return true Scan finished.
     * @return false Scan still running.
     */
    bool isScanDone() const;

    /**
     * @brief Retrieve WiFi scan results.
     *
     * @param results Output result array.
     * @param max Maximum result count.
     *
     * @return int Number of copied entries.
     */
    int getScanResults(WiFiScanResult *results,
                       int max);

    /**
     * @brief Register user WiFi event callback.
     *
     * @param cb Callback function.
     */
    void setHandler(WiFiCallback cb)
    {
        _callback = cb;
    }

    /**
     * @brief Check STA connection state.
     *
     * @return true STA connected.
     * @return false STA disconnected.
     */
    bool isConnected() const;

    void getSTAIP(char *dest, size_t max_len) const;
    const char *getSTAIP() const;
    void getAPIP(char *dest, size_t max_len) const;
    const char *getAPIP() const;
    void getSlaveMac(void *data, char *dest);
    const char *getEventString(int32_t event_id) const;
    uint32_t getSTAIPRaw() const;
    uint8_t getChannel() const;
    int8_t getTxPower() const;

    WiFiStatus getSTAStatus() const;
    WiFiStatus getAPStatus() const;

    int getMode() const;
    int getAuthMode() const;
};

/**
 * @brief Global NeuWiFi runtime instance.
 */
extern NeuWiFiClass NeuWiFi;