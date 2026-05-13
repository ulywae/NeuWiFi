/**
 * @file NeuWiFi_Getter.cpp
 * @author Ulywae
 * @brief Getter utilities and runtime status implementation for NeuWiFi.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * This file contains:
 * - WiFi scan result extraction
 * - IP address retrieval
 * - MAC address formatting
 * - Runtime status reporting
 * - Channel and TX power queries
 * - WiFi mode helpers
 * - Authentication mode translation
 *
 * Design goals:
 * - Low-overhead status access
 * - Safe memory usage
 * - Defensive runtime validation
 * - Human-readable diagnostics
 */

#include "_NeuWiFi.h"

/**
 * @brief Convert raw MAC address to string.
 *
 * @details
 * Converts a 6-byte MAC address into:
 * XX:XX:XX:XX:XX:XX format.
 *
 * @param mac Source MAC bytes.
 * @param dest Destination string buffer.
 */
void formatMac(const uint8_t *mac,
               char *dest)
{
    sprintf(dest,
            "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0],
            mac[1],
            mac[2],
            mac[3],
            mac[4],
            mac[5]);
}

/**
 * @brief Retrieve WiFi scan results.
 *
 * @details
 * Copies scanned AP information into user buffer.
 *
 * Safety features:
 * - RAM overflow protection
 * - malloc failure guard
 * - Safe string copy
 * - Scan size clamping
 *
 * Returned fields:
 * - SSID
 * - RSSI
 * - Channel
 *
 * @param res Destination result array.
 * @param max Maximum entries to copy.
 *
 * @return Number of copied scan results.
 */
int NeuWiFi::getScanResults(WiFiScanResult *res,
                            int max)
{
    uint16_t found_ap = 0;

    esp_wifi_scan_get_ap_num(&found_ap);

    // =====================================================
    // Clamp Result Count
    // =====================================================

    /**
     * Prevent RAM overflow from excessive scan entries.
     */
    uint16_t n =
        (found_ap < max)
            ? found_ap
            : max;

    if (n == 0)
        return 0;

    // =====================================================
    // Allocate Temporary Buffer
    // =====================================================

    wifi_ap_record_t *rec =
        (wifi_ap_record_t *)malloc(
            sizeof(wifi_ap_record_t) * n);

    /**
     * Guard against allocation failure.
     */
    if (!rec)
        return 0;

    // =====================================================
    // Retrieve Scan Records
    // =====================================================

    if (esp_wifi_scan_get_ap_records(&n, rec) != ESP_OK)
    {
        free(rec);
        return 0;
    }

    // =====================================================
    // Copy Results Safely
    // =====================================================

    for (int i = 0; i < n; i++)
    {
        memset(res[i].ssid,
               0,
               sizeof(res[i].ssid));

        strncpy(res[i].ssid,
                (char *)rec[i].ssid,
                sizeof(res[i].ssid) - 1);

        res[i].rssi = rec[i].rssi;
        res[i].channel = rec[i].primary;
    }

    free(rec);

    return n;
}

/**
 * @brief Get STA IP address string.
 *
 * @details
 * Returns current STA interface IP.
 *
 * Fallback:
 * - "0.0.0.0" if unavailable.
 *
 * @return const char* IP string.
 */
const char *NeuWiFi::getSTAIP() const
{
    static char sta_ip_str[16];

    esp_netif_ip_info_t ip_info;

    if (_sta_netif &&
        esp_netif_get_ip_info(_sta_netif,
                              &ip_info) == ESP_OK)
    {
        esp_ip4addr_ntoa(&ip_info.ip,
                         sta_ip_str,
                         sizeof(sta_ip_str));

        return sta_ip_str;
    }

    return "0.0.0.0";
}

/**
 * @brief Get AP IP address string.
 *
 * @details
 * Returns current AP interface IP.
 *
 * Fallback:
 * - "0.0.0.0" if unavailable.
 *
 * @return const char* AP IP string.
 */
const char *NeuWiFi::getAPIP() const
{
    static char ap_ip_str[16];

    esp_netif_ip_info_t ip_info;

    if (_ap_netif &&
        esp_netif_get_ip_info(_ap_netif,
                              &ip_info) == ESP_OK)
    {
        esp_ip4addr_ntoa(&ip_info.ip,
                         ap_ip_str,
                         sizeof(ap_ip_str));

        return ap_ip_str;
    }

    return "0.0.0.0";
}

/**
 * @brief Extract connected station MAC address.
 *
 * @details
 * Converts AP station event MAC data into
 * readable string format.
 *
 * @param data AP station event pointer.
 * @param dest Destination string buffer.
 */
void NeuWiFi::getSlaveMac(void *data,
                          char *dest)
{
    if (!data || !dest)
        return;

    auto *event =
        (wifi_event_ap_staconnected_t *)data;

    sprintf(dest,
            "%02X:%02X:%02X:%02X:%02X:%02X",
            event->mac[0],
            event->mac[1],
            event->mac[2],
            event->mac[3],
            event->mac[4],
            event->mac[5]);
}

/**
 * @brief Convert event ID into readable text.
 *
 * @param event_id ESP event identifier.
 *
 * @return const char* Human-readable event name.
 */
const char *NeuWiFi::getEventString(int32_t event_id) const
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

/**
 * @brief Get current WiFi channel.
 *
 * @return uint8_t Primary WiFi channel.
 */
uint8_t NeuWiFi::getChannel() const
{
    uint8_t primary;

    wifi_second_chan_t second;

    esp_wifi_get_channel(&primary,
                         &second);

    return primary;
}

/**
 * @brief Get WiFi TX power.
 *
 * @details
 * ESP-IDF internally stores TX power
 * in 0.25 dBm units.
 *
 * This function converts it back to:
 * - standard dBm
 *
 * @return int8_t TX power in dBm.
 */
int8_t NeuWiFi::getTxPower() const
{
    int8_t power_val;

    if (esp_wifi_get_max_tx_power(&power_val) == ESP_OK)
    {
        return (power_val / 4);
    }

    return 0;
}

/**
 * @brief Get raw STA IP value.
 *
 * @details
 * Returns IPv4 address as uint32_t.
 *
 * Useful for:
 * - binary protocols
 * - low-level networking
 * - fast comparisons
 *
 * @return uint32_t Raw IPv4 value.
 */
uint32_t NeuWiFi::getSTAIPRaw() const
{
    esp_netif_ip_info_t ip_info;

    if (_sta_netif &&
        esp_netif_get_ip_info(_sta_netif,
                              &ip_info) == ESP_OK)
    {
        return ip_info.ip.addr;
    }

    return 0;
}

/**
 * @brief Get detailed STA runtime status.
 *
 * @details
 * Collects:
 * - IP information
 * - MAC address
 * - SSID
 * - BSSID
 * - RSSI
 * - TX power
 * - Channel
 * - Authentication mode
 *
 * @return WiFiStatus Runtime STA information.
 */
WiFiStatus NeuWiFi::getSTAStatus() const
{
    WiFiStatus status =
        {
            "0.0.0.0",
            "0.0.0.0",
            "0.0.0.0",
            "",
            "00:00:00:00:00:00",
            "00:00:00:00:00:00",
            0};

    esp_netif_ip_info_t ip_info;
    wifi_ap_record_t ap_info;
    uint8_t mac_raw[6];

    // =====================================================
    // Retrieve IP Information
    // =====================================================

    if (_sta_netif &&
        esp_netif_get_ip_info(_sta_netif,
                              &ip_info) == ESP_OK)
    {
        esp_ip4addr_ntoa(&ip_info.ip,
                         status.ip,
                         16);

        esp_ip4addr_ntoa(&ip_info.netmask,
                         status.netmask,
                         16);

        esp_ip4addr_ntoa(&ip_info.gw,
                         status.gateway,
                         16);
    }

    // =====================================================
    // Retrieve MAC Address
    // =====================================================

    esp_read_mac(mac_raw,
                 ESP_MAC_WIFI_STA);

    sprintf(status.mac,
            "%02X:%02X:%02X:%02X:%02X:%02X",
            mac_raw[0],
            mac_raw[1],
            mac_raw[2],
            mac_raw[3],
            mac_raw[4],
            mac_raw[5]);

    // =====================================================
    // Retrieve AP Connection Information
    // =====================================================

    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
    {
        strncpy(status.ssid,
                (char *)ap_info.ssid,
                32);

        status.rssi = ap_info.rssi;
        status.authmode = ap_info.authmode;
        status.channel = getChannel();
        status.tx_power = getTxPower();

        sprintf(status.bssid,
                "%02X:%02X:%02X:%02X:%02X:%02X",
                ap_info.bssid[0],
                ap_info.bssid[1],
                ap_info.bssid[2],
                ap_info.bssid[3],
                ap_info.bssid[4],
                ap_info.bssid[5]);
    }

    return status;
}

/**
 * @brief Get detailed AP runtime status.
 *
 * @details
 * Collects:
 * - AP IP information
 * - AP MAC address
 * - AP SSID
 *
 * @return WiFiStatus Runtime AP information.
 */
WiFiStatus NeuWiFi::getAPStatus() const
{
    WiFiStatus status =
        {
            "0.0.0.0",
            "0.0.0.0",
            "0.0.0.0",
            "",
            "00:00:00:00:00:00",
            "N/A"};

    esp_netif_ip_info_t ip_info;

    wifi_config_t conf;

    uint8_t mac_raw[6];

    // =====================================================
    // Retrieve AP IP Information
    // =====================================================

    if (_ap_netif &&
        esp_netif_get_ip_info(_ap_netif,
                              &ip_info) == ESP_OK)
    {
        esp_ip4addr_ntoa(&ip_info.ip,
                         status.ip,
                         16);

        esp_ip4addr_ntoa(&ip_info.netmask,
                         status.netmask,
                         16);

        esp_ip4addr_ntoa(&ip_info.gw,
                         status.gateway,
                         16);
    }

    // =====================================================
    // Retrieve AP MAC Address
    // =====================================================

    esp_read_mac(mac_raw,
                 ESP_MAC_WIFI_SOFTAP);

    formatMac(mac_raw,
              status.mac);

    // =====================================================
    // Retrieve AP SSID
    // =====================================================

    if (esp_wifi_get_config(WIFI_IF_AP,
                            &conf) == ESP_OK)
    {
        strncpy(status.ssid,
                (char *)conf.ap.ssid,
                32);
    }

    return status;
}

/**
 * @brief Get current WiFi mode string.
 *
 * @return const char* Current WiFi mode.
 */
const char *NeuWiFi::getModeString() const
{
    wifi_mode_t mode;

    if (esp_wifi_get_mode(&mode) != ESP_OK)
        return "OFF";

    switch (mode)
    {
    case WIFI_MODE_STA:
        return "STATION";

    case WIFI_MODE_AP:
        return "ACCESS_POINT";

    case WIFI_MODE_APSTA:
        return "HYBRID";

    default:
        return "OFF";
    }
}

/**
 * @brief Convert authentication mode into readable text.
 *
 * @param mode Authentication mode.
 *
 * @return const char* Authentication mode string.
 */
const char *NeuWiFi::getAuthModeString(wifi_auth_mode_t mode) const
{
    switch (mode)
    {
    case WIFI_AUTH_OPEN:
        return "OPEN";

    case WIFI_AUTH_WEP:
        return "WEP";

    case WIFI_AUTH_WPA_PSK:
        return "WPA_PSK";

    case WIFI_AUTH_WPA2_PSK:
        return "WPA2_PSK";

    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WPA_WPA2_PSK";

    case WIFI_AUTH_WPA3_PSK:
        return "WPA3_PSK";

    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "WPA2_WPA3_PSK";

    default:
        return "UNKNOWN";
    }
}