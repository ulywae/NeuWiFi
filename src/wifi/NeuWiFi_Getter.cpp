/**
 * @file NeuWiFi_Getter.cpp
 * @brief Runtime information, status retrieval, and utility helpers
 *        for NeuWiFiClass.
 *
 * This module contains all functions dedicated to reading, retrieving,
 * and formatting runtime information from the WiFi subsystem. It provides
 * a complete set of tools for status inspection, diagnostics, and data extraction
 * in a safe and embedded-friendly manner.
 *
 * Core Responsibilities:
 * - WiFi scan result extraction and formatting
 * - STA and AP IP address retrieval in multiple formats
 * - MAC address formatting and parsing utilities
 * - Runtime status inspection and snapshot generation
 * - Event ID translation to human-readable strings
 * - WiFi operating mode and authentication type queries
 *
 * Runtime Features:
 * - Lightweight runtime inspection with minimal overhead
 * - Allocation-safe scan extraction with proper memory management
 * - Static IP string caching for zero-allocation API usage
 * - Full STA/AP diagnostic snapshot generation
 * - Production-grade helper utilities safe for continuous polling
 *
 * Design Goals:
 * - Deterministic runtime queries with consistent return behavior
 * - Embedded-safe string handling (fixed buffers, bounds checking)
 * - Centralized WiFi diagnostics for easier debugging
 * - Low-overhead monitoring utilities suitable for background tasks
 * - Clear separation between raw data and formatted output
 *
 * @author Ulywae (@Neufa)
 * @date 2026
 * @version 1.1.0
 */

#include "wifi/_NeuWiFi.h"

// =====================================================
// INTERNAL MAC FORMATTER
// =====================================================

/**
 * @brief Convert raw binary MAC address into human-readable string format.
 *
 * Helper utility to transform 6-byte raw MAC data into standard colon-separated
 * string representation. Used internally by status functions and event handlers.
 *
 * Output format:
 * XX:XX:XX:XX:XX:XX
 *
 * @param mac Pointer to 6-byte raw MAC address source
 * @param dest Destination string buffer (minimum 18 bytes required)
 */
void formatMac(const uint8_t *mac, char *dest)
{
    if (!dest)
        return;
    snprintf(dest, 18,
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// =====================================================
// STATIC INTERNAL BUFFERS
// =====================================================

/**
 * @brief Internal static buffer for STA IP string storage.
 * @details Persistent storage used by parameter-less getSTAIP() method.
 *          Allows returning const char* without requiring external buffer management.
 */
char NeuWiFiClass::_sta_ip_static_buffer[16] = {0};
/**
 * @brief Internal static buffer for AP IP string storage.
 * @details Persistent storage used by parameter-less getAPIP() method.
 *          Allows returning const char* without requiring external buffer management.
 */
char NeuWiFiClass::_ap_ip_static_buffer[16] = {0};

// =====================================================
// WIFI SCAN RESULTS
// =====================================================

/**
 * @brief Retrieve and format WiFi scan results from driver cache.
 *
 * Extracts detected network list from ESP-IDF driver and copies into user-provided
 * structure array. Handles memory allocation safely and performs cleanup automatically.
 *
 * Features:
 * - Bounded copy protection to prevent buffer overflow
 * - Automatic scan completion flag clear after read
 * - Allocation-safe retrieval with internal free handling
 * - Partial copy support if buffer is smaller than result count
 *
 * Stored Fields in result structure:
 * - SSID (network name string)
 * - RSSI (signal strength in dBm)
 * - Channel (operating frequency channel)
 *
 * @param res Destination array of WiFiScanResult structures
 * @param max Maximum number of entries that fit in destination array
 *
 * @return Number of networks successfully copied into result array
 */
int NeuWiFiClass::getScanResults(WiFiScanResult *res, int max)
{
    if (!_event_group || !res || max <= 0)
        return 0;

    uint16_t number_of_ap = 0;
    esp_err_t err = esp_wifi_scan_get_ap_num(&number_of_ap);
    if (err != ESP_OK || number_of_ap == 0)
    {
        xEventGroupClearBits(_event_group, WIFI_SCAN_DONE_BIT);
        return 0;
    }

    if (number_of_ap > max)
        number_of_ap = max;

    wifi_ap_record_t *ap_records = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * number_of_ap);
    if (!ap_records)
    {
        xEventGroupClearBits(_event_group, WIFI_SCAN_DONE_BIT);
        return 0;
    }

    int copied = 0;
    err = esp_wifi_scan_get_ap_records(&number_of_ap, ap_records);
    if (err == ESP_OK)
    {
        for (int i = 0; i < number_of_ap; i++)
        {
            strncpy(res[i].ssid, (const char *)ap_records[i].ssid, sizeof(res[i].ssid) - 1);
            res[i].ssid[sizeof(res[i].ssid) - 1] = '\0';
            res[i].rssi = ap_records[i].rssi;
            res[i].channel = ap_records[i].primary;
            copied++;
        }
        // printf("[SCAN] Successfully copied %d network to application.\n", copied);
    }
    else
    {
        // printf("[SCAN] Failed to retrieve records from driver! Error: %s\n", esp_err_to_name(err));
    }

    free(ap_records);
    xEventGroupClearBits(_event_group, WIFI_SCAN_DONE_BIT);
    return copied;
}

// =====================================================
// STA IP GETTERS
// =====================================================

/**
 * @brief Retrieve current Station IP address into user-provided buffer.
 *
 * Queries network interface for current IP configuration and formats
 * address as human-readable string. Returns "0.0.0.0" if interface is inactive.
 *
 * @param dest Destination character buffer
 * @param max_len Size of destination buffer (minimum 16 bytes recommended)
 */
void NeuWiFiClass::getSTAIP(char *dest, size_t max_len) const
{
    if (dest == nullptr || max_len == 0)
        return;
    esp_netif_ip_info_t ip_info;
    if (_sta_netif && esp_netif_get_ip_info(_sta_netif, &ip_info) == ESP_OK)
    {
        char temp_ip[16];
        esp_ip4addr_ntoa(&ip_info.ip, temp_ip, sizeof(temp_ip));
        strncpy(dest, temp_ip, max_len - 1);
        dest[max_len - 1] = '\0';
    }
    else
    {
        strncpy(dest, "0.0.0.0", max_len - 1);
        dest[max_len - 1] = '\0';
    }
}

/**
 * @brief Retrieve current Station IP address using internal static buffer.
 *
 * Convenience method returning pointer to internal persistent buffer.
 * Safe to use in most scenarios, data remains valid until next call.
 *
 * @return Pointer to IP address string (internal storage)
 */
const char *NeuWiFiClass::getSTAIP() const
{
    getSTAIP(_sta_ip_static_buffer, sizeof(_sta_ip_static_buffer));
    return _sta_ip_static_buffer;
}

// =====================================================
// AP IP GETTERS
// =====================================================

/**
 * @brief Retrieve current Access Point IP address into user-provided buffer.
 *
 * Queries AP network interface for current IP configuration and formats
 * address as human-readable string. Returns "0.0.0.0" if AP is inactive.
 *
 * @param dest Destination character buffer
 * @param max_len Size of destination buffer (minimum 16 bytes recommended)
 */
void NeuWiFiClass::getAPIP(char *dest, size_t max_len) const
{
    if (dest == nullptr || max_len == 0)
        return;
    esp_netif_ip_info_t ip_info;
    if (_ap_netif && esp_netif_get_ip_info(_ap_netif, &ip_info) == ESP_OK)
    {
        char temp_ip[16];
        esp_ip4addr_ntoa(&ip_info.ip, temp_ip, sizeof(temp_ip));
        strncpy(dest, temp_ip, max_len - 1);
        dest[max_len - 1] = '\0';
    }
    else
    {
        strncpy(dest, "0.0.0.0", max_len - 1);
        dest[max_len - 1] = '\0';
    }
}

/**
 * @brief Retrieve current Access Point IP address using internal static buffer.
 *
 * Convenience method returning pointer to internal persistent buffer.
 * Safe to use in most scenarios, data remains valid until next call.
 *
 * @return Pointer to IP address string (internal storage)
 */
const char *NeuWiFiClass::getAPIP() const
{
    getAPIP(_ap_ip_static_buffer, sizeof(_ap_ip_static_buffer));
    return _ap_ip_static_buffer;
}

// =====================================================
// MAC EXTRACTION
// =====================================================

/**
 * @brief Extract station MAC address from AP connection event data.
 *
 * Parses raw event structure generated when a client connects to our Access Point
 * and formats MAC address into readable string.
 *
 * Used for:
 * - AP station connected event processing
 * - Runtime logging and auditing
 * - Device identification and tracking
 *
 * @param data Pointer to raw event data payload
 * @param dest Destination string buffer (minimum 18 bytes required)
 */
void NeuWiFiClass::getSlaveMac(void *data, char *dest)
{
    if (!data || !dest)
        return;

    auto *event = (wifi_event_ap_staconnected_t *)data;

    snprintf(dest, 18,
             "%02X:%02X:%02X:%02X:%02X:%02X",
             event->mac[0], event->mac[1], event->mac[2],
             event->mac[3], event->mac[4], event->mac[5]);
}

// =====================================================
// EVENT STRING TRANSLATION
// =====================================================

/**
 * @brief Translate raw WiFi event ID into human-readable description string.
 *
 * Converts numeric event identifiers received from ESP-IDF into clear text labels
 * useful for logging, debugging, or user interface display.
 *
 * @param event_id Raw ESP-IDF WiFi or IP event identifier
 *
 * @return Static string describing the event meaning
 */
const char *NeuWiFiClass::getEventString(int32_t event_id) const
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        return "Station mode started";

    case WIFI_EVENT_STA_CONNECTED:
        return "Connected to AP";

    case WIFI_EVENT_STA_DISCONNECTED:
        return "Disconnected from AP";

    case WIFI_EVENT_STA_AUTHMODE_CHANGE:
        return "Auth mode changed";

    case WIFI_EVENT_AP_START:
        return "Soft-AP started";

    case WIFI_EVENT_AP_STOP:
        return "Soft-AP stopped";

    case WIFI_EVENT_AP_STACONNECTED:
        return "New station connected";

    case WIFI_EVENT_AP_STADISCONNECTED:
        return "Station disconnected";

    case WIFI_EVENT_SCAN_DONE:
        return "WiFi scan completed";

    case IP_EVENT_STA_GOT_IP:
        return "Got IP address";

    default:
        return "Other WiFi event";
    }
}

// =====================================================
// CHANNEL & TX POWER
// =====================================================

/**
 * @brief Retrieve currently active WiFi operating channel.
 *
 * Queries driver for primary channel number in use. In hybrid mode,
 * returns current active channel after any synchronization changes.
 *
 * @return WiFi channel number (1-13), or 0 on error/inactive
 */
uint8_t NeuWiFiClass::getChannel() const
{
    uint8_t primary = 0;
    wifi_second_chan_t second;

    if (esp_wifi_get_channel(&primary, &second) == ESP_OK)
        return primary;

    return 0;
}

/**
 * @brief Retrieve current WiFi transmit power level in dBm.
 *
 * Reads configured maximum transmit power and converts from internal
 * ESP-IDF units (0.25dBm steps) back to standard dBm format.
 *
 * @return TX power level in dBm (range approx 2-20)
 */
int8_t NeuWiFiClass::getTxPower() const
{
    int8_t power_val;

    if (esp_wifi_get_max_tx_power(&power_val) == ESP_OK)
        return (power_val / 4);

    return 0;
}

// =====================================================
// RAW STA IP
// =====================================================

/**
 * @brief Retrieve raw binary Station IPv4 address.
 *
 * Returns IP address as 32-bit integer in LwIP byte order.
 * Useful for low-level processing, comparisons, or storage.
 *
 * @return IPv4 address in uint32_t format, 0 if inactive
 */
uint32_t NeuWiFiClass::getSTAIPRaw() const
{
    esp_netif_ip_info_t ip_info;
    if (_sta_netif && esp_netif_get_ip_info(_sta_netif, &ip_info) == ESP_OK)
        return ip_info.ip.addr;

    return 0;
}

// =====================================================
// STA STATUS SNAPSHOT
// =====================================================

/**
 * @brief Retrieve complete comprehensive Station runtime status snapshot.
 *
 * Collects all available information regarding current Station mode state
 * into a single structure. Performs dynamic queries to ESP-IDF runtime
 * to ensure data is always current.
 *
 * Collected Information:
 * - STA IP address string
 * - Subnet mask
 * - Gateway address
 * - Primary DNS server
 * - Secondary DNS server
 * - Connected AP SSID name
 * - Device STA MAC address
 * - Connected AP BSSID (MAC)
 * - RSSI signal strength value
 * - Current TX power level
 * - Operating channel
 * - Authentication mode
 *
 * Runtime Notes:
 * - Safe to call repeatedly or from background tasks
 * - Dynamically queries ESP-IDF runtime every call
 * - Returns zero-filled/empty structure on failure or inactive
 *
 * @return WiFiStatus Fully populated status structure
 */
WiFiStatus NeuWiFiClass::getSTAStatus() const
{
    WiFiStatus status = {
        .authmode = 0,
        .ip = {0},
        .netmask = {0},
        .gateway = {0},
        .dns_primary = {0},
        .dns_secondary = {0},
        .ssid = {},
        .mac = {0},
        .bssid = {0},
        .rssi = 0,
        .tx_power = 0,
        .channel = 0};

    esp_netif_ip_info_t ip_info;
    wifi_ap_record_t ap_info;
    uint8_t mac_raw[6];

    if (_sta_netif)
    {
        if (esp_netif_get_ip_info(_sta_netif, &ip_info) == ESP_OK)
        {
            esp_ip4addr_ntoa(&ip_info.ip, status.ip, sizeof(status.ip));
            esp_ip4addr_ntoa(&ip_info.netmask, status.netmask, sizeof(status.netmask));
            esp_ip4addr_ntoa(&ip_info.gw, status.gateway, sizeof(status.gateway));
        }

        esp_netif_dns_info_t dns_pri;
        if (esp_netif_get_dns_info(_sta_netif, ESP_NETIF_DNS_MAIN, &dns_pri) == ESP_OK)
        {
            esp_ip4addr_ntoa(ip_2_ip4(&dns_pri.ip), status.dns_primary, sizeof(status.dns_primary));
        }
        esp_netif_dns_info_t dns_sec;
        if (esp_netif_get_dns_info(_sta_netif, ESP_NETIF_DNS_BACKUP, &dns_sec) == ESP_OK)
        {
            esp_ip4addr_ntoa(ip_2_ip4(&dns_sec.ip), status.dns_secondary, sizeof(status.dns_secondary));
        }
    }

    esp_read_mac(mac_raw, ESP_MAC_WIFI_STA);
    formatMac(mac_raw, status.mac);

    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
    {
        memset(status.ssid, 0, sizeof(status.ssid));
        strncpy(status.ssid, (char *)ap_info.ssid, sizeof(status.ssid) - 1);

        status.rssi = ap_info.rssi;
        status.authmode = (int)ap_info.authmode;
        status.channel = getChannel();
        status.tx_power = getTxPower();

        formatMac(ap_info.bssid, status.bssid);
    }

    return status;
}

// =====================================================
// AP STATUS SNAPSHOT
// =====================================================

/**
 * @brief Retrieve complete comprehensive Access Point runtime status snapshot.
 *
 * Collects all available information regarding current Access Point mode state
 * into a single structure. Queries active AP runtime configuration and status.
 *
 * Collected Information:
 * - AP IP address string
 * - Subnet mask
 * - Gateway address
 * - AP SSID name
 * - Device AP MAC address
 * - Operating channel
 *
 * Runtime Notes:
 * - Queries active AP runtime state directly from driver
 * - Safe to call continuously or from background tasks
 * - Returns zero-filled/empty structure if AP is inactive or not started
 *
 * @return WiFiStatus Fully populated AP status structure
 */
WiFiStatus NeuWiFiClass::getAPStatus() const
{
    WiFiStatus status = {
        .authmode = 0,
        .ip = {0},
        .netmask = {0},
        .gateway = {0},
        .dns_primary = {0},
        .dns_secondary = {0},
        .ssid = {},
        .mac = {0},
        .bssid = {0},
        .rssi = 0,
        .tx_power = 0,
        .channel = 0};

    esp_netif_ip_info_t ip_info;
    wifi_config_t conf;
    uint8_t mac_raw[6];

    if (_ap_netif && esp_netif_get_ip_info(_ap_netif, &ip_info) == ESP_OK)
    {
        esp_ip4addr_ntoa(&ip_info.ip, status.ip, sizeof(status.ip));
        esp_ip4addr_ntoa(&ip_info.netmask, status.netmask, sizeof(status.netmask));
        esp_ip4addr_ntoa(&ip_info.gw, status.gateway, sizeof(status.gateway));
    }

    esp_read_mac(mac_raw, ESP_MAC_WIFI_SOFTAP);
    formatMac(mac_raw, status.mac);

    if (esp_wifi_get_config(WIFI_IF_AP, &conf) == ESP_OK)
    {
        memset(status.ssid, 0, sizeof(status.ssid));
        strncpy(status.ssid, (char *)conf.ap.ssid, sizeof(status.ssid) - 1);
        status.channel = conf.ap.channel;
    }

    return status;
}

// =====================================================
// WIFI MODE QUERY
// =====================================================

/**
 * @brief Retrieve current WiFi subsystem operating mode.
 *
 * Returns currently active operational mode of the WiFi driver.
 * Useful to determine if device is acting as station, access point, both, or idle.
 *
 * Possible return values:
 * - WIFI_MODE_NULL   : WiFi not initialized or stopped
 * - WIFI_MODE_STA    : Station mode only active
 * - WIFI_MODE_AP     : Access Point mode only active
 * - WIFI_MODE_APSTA  : Hybrid mode (both active)
 *
 * @return int Active WiFi mode constant
 */
int NeuWiFiClass::getMode() const
{
    wifi_mode_t mode;
    if (esp_wifi_get_mode(&mode) == ESP_OK)
        return (int)mode;

    return 0;
}

// =====================================================
// AUTHENTICATION MODE QUERY
// =====================================================

/**
 * @brief Retrieve authentication mode of currently connected AP.
 *
 * Returns security type used by the access point that the station is
 * currently connected to. Helps determine encryption level and capabilities.
 *
 * Example return values:
 * - WIFI_AUTH_OPEN        : No security / open network
 * - WIFI_AUTH_WPA_PSK     : WPA standard security
 * - WIFI_AUTH_WPA2_PSK    : WPA2 modern security
 * - WIFI_AUTH_WPA3_PSK    : WPA3 latest security
 *
 * @return int Authentication mode constant
 * @return -1 Station disconnected or query failed
 */
int NeuWiFiClass::getAuthMode() const
{
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
        return (int)ap_info.authmode;

    return -1;
}