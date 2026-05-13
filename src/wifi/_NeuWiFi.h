/**
 * @file _NeuWiFi.h
 * @author Ulywae
 * @brief Deterministic WiFi abstraction layer for ESP32.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * NeuWiFi is a lightweight and deterministic WiFi engine built on top of
 * ESP-IDF native networking APIs. It provides simplified STA/AP/Hybrid mode
 * management while preserving low-level control and runtime stability.
 *
 * Features:
 * - STA (Station) mode
 * - AP (Access Point) mode
 * - Hybrid STA + AP mode
 * - Static IP configuration
 * - WiFi scanning
 * - Runtime event callbacks
 * - TX power control
 * - Security filtering
 * - Detailed connection status
 * - Deterministic event handling
 *
 * Designed for:
 * - ESP32
 * - ESP-IDF
 * - Real-time embedded systems
 * - Deterministic runtime architectures
 */

#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"
#include <string.h>
#include <functional>

/**
 * @defgroup NeuWiFi_EventBits WiFi Event Bits
 * @brief Internal event flags used by NeuWiFi.
 * @{
 */

/** @brief WiFi successfully connected. */
#define WIFI_CONNECTED_BIT BIT0

/** @brief WiFi connection failed. */
#define WIFI_FAIL_BIT BIT1

/** @brief WiFi scan completed. */
#define WIFI_SCAN_DONE_BIT BIT2

/** @} */

/**
 * @brief Structure representing a scanned WiFi network.
 */
typedef struct
{
    /** @brief Network SSID. */
    char ssid[33];

    /** @brief RSSI signal strength in dBm. */
    int8_t rssi;

    /** @brief WiFi channel. */
    uint8_t channel;

} WiFiScanResult;

/**
 * @brief Detailed network information container.
 */
typedef struct
{
    /** @brief Local IP address. */
    char ip[16];

    /** @brief Network subnet mask. */
    char netmask[16];

    /** @brief Gateway IP address. */
    char gateway[16];

    /** @brief Connected SSID. */
    char ssid[33];

    /** @brief Device MAC address. */
    char mac[18];

    /** @brief Connected BSSID address. */
    char bssid[18];

    /** @brief Signal strength (RSSI). */
    int8_t rssi;

    /** @brief TX power level. */
    int8_t tx_power;

    /** @brief Current WiFi channel. */
    uint8_t channel;

    /** @brief Authentication mode. */
    uint32_t authmode;

} WiFiStatus;

/**
 * @brief Static IP configuration container.
 */
typedef struct
{
    /** @brief Static IP address. */
    char ip[16];

    /** @brief Subnet mask. */
    char netmask[16];

    /** @brief Gateway address. */
    char gateway[16];

    /** @brief DNS server address. */
    char dns[16];

} IPConfig;

/**
 * @brief Unified WiFi event callback type.
 *
 * @param event_id ESP event identifier.
 * @param event_data Raw event data pointer.
 */
using WiFiEventCallback = std::function<void(int32_t event_id, void *event_data)>;

/**
 * @class NeuWiFi
 * @brief Core WiFi engine for ESP32 networking.
 *
 * @details
 * NeuWiFi provides a deterministic wrapper around ESP-IDF WiFi APIs.
 * It simplifies WiFi management while maintaining low-level control
 * and predictable runtime behavior.
 *
 * Supported modes:
 * - STA
 * - AP
 * - STA + AP Hybrid
 *
 * Main capabilities:
 * - Static IP
 * - Auto reconnect
 * - Event-driven architecture
 * - Scan management
 * - Security filtering
 * - TX power control
 */
class NeuWiFi
{
private:
    /**
     * @brief Internal ESP-IDF event dispatcher.
     *
     * @param arg User argument pointer.
     * @param base Event base.
     * @param id Event identifier.
     * @param data Event payload.
     */
    static void eventHandler(void *arg,
                             esp_event_base_t base,
                             int32_t id,
                             void *data);

    /** @brief STA network interface handle. */
    esp_netif_t *_sta_netif = nullptr;

    /** @brief AP network interface handle. */
    esp_netif_t *_ap_netif = nullptr;

    /** @brief Internal event group handle. */
    EventGroupHandle_t _event_group = nullptr;

    /** @brief Static IP enable flags. */
    bool _use_static_sta = false,
         _use_static_ap = false;

    /** @brief Static IP configurations. */
    esp_netif_ip_info_t _static_ip_sta,
        _static_ip_ap;

    /** @brief STA DNS configuration. */
    esp_netif_dns_info_t _dns_sta;

    /** @brief Minimum allowed STA authentication mode. */
    wifi_auth_mode_t _min_auth_sta = WIFI_AUTH_WPA2_PSK;

    /** @brief Device hostname. */
    char _hostname[32] = "Neu-Device";

    /** @brief Auto reconnect state. */
    bool _auto_reconnect = true;

    /** @brief Maximum AP client connections. */
    uint8_t _max_conn;

    /** @brief AP channel number. */
    uint8_t _ap_channel = 1;

    /** @brief User-defined event callback. */
    WiFiEventCallback _user_callback = nullptr;

public:
    /**
     * @brief Construct a new NeuWiFi object.
     *
     * @param max_conn Maximum AP clients.
     */
    NeuWiFi(uint8_t max_conn = 4);

    // =====================================================
    // Lifecycle
    // =====================================================

    /**
     * @brief Initialize TCP/IP stack and WiFi driver.
     */
    void init();

    /**
     * @brief Arduino-style alias for init().
     */
    inline void begin() { init(); }

    /**
     * @brief Stop and deinitialize WiFi subsystem.
     */
    void stop();

    // =====================================================
    // Configuration
    // =====================================================

    /**
     * @brief Configure static STA IP settings.
     *
     * @param ip Static IP address.
     * @param subnet Subnet mask.
     * @param gateway Gateway IP.
     * @param dns DNS server address.
     */
    void setStaticSTA(const char *ip,
                      const char *subnet,
                      const char *gateway,
                      const char *dns = "8.8.8.8");

    /**
     * @brief Configure static STA using structure.
     *
     * @param config IP configuration structure.
     */
    void setStaticSTA(IPConfig config);

    /**
     * @brief Configure static AP IP settings.
     *
     * @param ip Static AP IP.
     * @param subnet Subnet mask.
     * @param gateway Gateway IP.
     */
    void setStaticAP(const char *ip,
                     const char *subnet,
                     const char *gateway);

    /**
     * @brief Configure static AP using structure.
     *
     * @param config IP configuration structure.
     */
    void setStaticAP(IPConfig config);

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
     * @brief Set maximum AP clients.
     *
     * @param max Maximum clients allowed (max 10).
     */
    void setMaxConnection(uint8_t max)
    {
        _max_conn = max;
    }

    /**
     * @brief Set AP channel.
     *
     * @param channel WiFi channel.
     */
    void setChannelAP(uint8_t channel);

    /**
     * @brief Set WiFi TX power.
     *
     * @param dbm Power level in dBm.
     */
    void setTxPower(int8_t dbm);

    /**
     * @brief Set minimum STA security level.
     *
     * @param mode Authentication mode.
     */
    void setMinSecuritySTA(wifi_auth_mode_t mode)
    {
        _min_auth_sta = mode;
    }

    // =====================================================
    // Operations
    // =====================================================

    /**
     * @brief Start STA mode and connect to AP.
     *
     * @param ssid Target SSID.
     * @param pass Target password.
     * @param timeout Connection timeout in seconds.
     *
     * @return true if connected successfully.
     * @return false if connection failed.
     */
    bool startSTA(const char *ssid,
                  const char *pass,
                  int timeout = 15);

    /**
     * @brief Start AP mode.
     *
     * @param name AP SSID.
     * @param pass AP password.
     */
    void startAP(const char *name,
                 const char *pass);

    /**
     * @brief Start Hybrid STA + AP mode.
     *
     * @param sSsid STA SSID.
     * @param sPass STA password.
     * @param aName AP SSID.
     * @param aPass AP password.
     */
    void startHybrid(const char *sSsid,
                     const char *sPass,
                     const char *aName,
                     const char *aPass);

    // =====================================================
    // Scanning
    // =====================================================

    /**
     * @brief Start asynchronous WiFi scan.
     */
    void startScan();

    /**
     * @brief Check scan completion state.
     *
     * @return true if scan completed.
     * @return false if scan still running.
     */
    bool isScanDone() const;

    /**
     * @brief Retrieve scan results.
     *
     * @param results Destination buffer.
     * @param max Maximum entries.
     *
     * @return Number of copied results.
     */
    int getScanResults(WiFiScanResult *results,
                       int max);

    // =====================================================
    // Callbacks
    // =====================================================

    /**
     * @brief Set user WiFi event callback.
     *
     * @param cb Callback function.
     */
    void setHandler(WiFiEventCallback cb)
    {
        _user_callback = cb;
    }

    // =====================================================
    // Status & Helpers
    // =====================================================

    /**
     * @brief Check STA connection state.
     *
     * @return true if connected.
     */
    bool isConnected() const;

    /**
     * @brief Get STA IP address string.
     *
     * @return const char* IP string.
     */
    const char *getSTAIP() const;

    /**
     * @brief Get AP IP address string.
     *
     * @return const char* IP string.
     */
    const char *getAPIP() const;

    /**
     * @brief Convert MAC data to string.
     *
     * @param data Raw MAC bytes.
     * @param dest Destination string buffer.
     */
    void getSlaveMac(void *data,
                     char *dest);

    /**
     * @brief Convert event ID to readable string.
     *
     * @param event_id Event identifier.
     *
     * @return const char* Event description.
     */
    const char *getEventString(int32_t event_id) const;

    /**
     * @brief Get raw STA IP address.
     *
     * @return uint32_t Raw IP value.
     */
    uint32_t getSTAIPRaw() const;

    /**
     * @brief Get current WiFi channel.
     *
     * @return uint8_t Channel number.
     */
    uint8_t getChannel() const;

    /**
     * @brief Get current TX power.
     *
     * @return int8_t TX power in dBm.
     */
    int8_t getTxPower() const;

    /**
     * @brief Get detailed STA status.
     *
     * @return WiFiStatus Structure containing STA info.
     */
    WiFiStatus getSTAStatus() const;

    /**
     * @brief Get detailed AP status.
     *
     * @return WiFiStatus Structure containing AP info.
     */
    WiFiStatus getAPStatus() const;

    /**
     * @brief Get current WiFi mode string.
     *
     * @return const char* Mode description.
     */
    const char *getModeString() const;

    /**
     * @brief Convert authentication mode to string.
     *
     * @param mode Authentication mode.
     *
     * @return const char* Human-readable auth mode.
     */
    const char *getAuthModeString(wifi_auth_mode_t mode) const;
};
