/**
 * @file NeuWiFi_AP.cpp
 * @author Ulywae
 * @brief Access Point and Hybrid mode implementation for NeuWiFi.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * This file contains:
 * - AP mode initialization
 * - Hybrid STA + AP mode
 * - Static AP configuration
 * - Channel management
 *
 * The implementation prioritizes:
 * - Runtime safety
 * - Deterministic behavior
 * - Defensive validation
 * - ESP-IDF native performance
 */

#include "_NeuWiFi.h"

/**
 * @brief Configure static IP settings for AP mode.
 *
 * @details
 * Enables static IP mode for the internal AP interface.
 * DHCP server will later use this configuration during AP startup.
 *
 * @param ip Static IP address.
 * @param subnet Network subnet mask.
 * @param gateway Gateway address.
 */
void NeuWiFi::setStaticAP(const char *ip,
                          const char *subnet,
                          const char *gateway)
{
    _use_static_ap = true;

    _static_ip_ap.ip.addr = ipaddr_addr(ip);
    _static_ip_ap.netmask.addr = ipaddr_addr(subnet);
    _static_ip_ap.gw.addr = ipaddr_addr(gateway);
}

/**
 * @brief Configure static AP using IPConfig structure.
 *
 * @param config IP configuration container.
 */
void NeuWiFi::setStaticAP(IPConfig config)
{
    setStaticAP(config.ip,
                config.netmask,
                config.gateway);
}

/**
 * @brief Set AP WiFi channel.
 *
 * @details
 * Includes runtime safety validation.
 * Valid 2.4GHz channels:
 * - 1 to 13 (global)
 * - 14 (Japan only, intentionally excluded)
 *
 * Invalid values automatically fallback to channel 1.
 *
 * @param channel WiFi channel number.
 */
void NeuWiFi::setChannelAP(uint8_t channel)
{
    // Safety guard for invalid channels
    if (channel < 1 || channel > 13)
        _ap_channel = 1;
    else
        _ap_channel = channel;
}

/**
 * @brief Start Access Point mode.
 *
 * @details
 * Initializes standalone AP mode using ESP-IDF native APIs.
 *
 * Features:
 * - Automatic fallback AP name
 * - Static IP support
 * - DHCP server handling
 * - Password safety validation
 * - WPA2/Open auto-selection
 *
 * Security behavior:
 * - Password length < 8 → OPEN mode
 * - Password length >= 8 → WPA2-PSK
 *
 * @param name AP SSID name.
 * @param pass AP password.
 */
void NeuWiFi::startAP(const char *name,
                      const char *pass)
{
    /**
     * @brief Fallback AP name protection.
     *
     * Prevents invalid empty SSID configuration.
     */
    const char *ap_name =
        (name != nullptr && strlen(name) > 0)
            ? name
            : "Neu-AccessPoint";

    /**
     * @brief Ensure previous WiFi state is stopped before reconfiguration.
     */
    esp_wifi_stop();

    /**
     * @brief Create AP network interface if not already initialized.
     */
    if (!_ap_netif)
        _ap_netif = esp_netif_create_default_wifi_ap();

    // =====================================================
    // Static IP Configuration
    // =====================================================

    if (_use_static_ap)
    {
        /**
         * Stop DHCP server before modifying IP configuration.
         */
        esp_netif_dhcps_stop(_ap_netif);

        /**
         * Apply static IP configuration.
         */
        esp_netif_set_ip_info(_ap_netif,
                              &_static_ip_ap);

        /**
         * Restart DHCP server after configuration update.
         */
        esp_netif_dhcps_start(_ap_netif);
    }

    /**
     * Apply hostname.
     */
    esp_netif_set_hostname(_ap_netif,
                           _hostname);

    // =====================================================
    // AP Configuration
    // =====================================================

    wifi_config_t ap_cfg = {};

    /**
     * Safe SSID copy.
     */
    strncpy((char *)ap_cfg.ap.ssid,
            ap_name,
            32);

    /**
     * Safe password copy.
     */
    if (pass)
    {
        strncpy((char *)ap_cfg.ap.password,
                pass,
                64);
    }

    /**
     * Security fallback protection.
     *
     * WPA2 requires minimum 8 characters.
     * Invalid passwords automatically downgrade to OPEN mode.
     */
    ap_cfg.ap.authmode =
        (strlen(pass) < 8)
            ? WIFI_AUTH_OPEN
            : WIFI_AUTH_WPA2_PSK;

    /**
     * Maximum simultaneous clients.
     */
    ap_cfg.ap.max_connection = _max_conn;

    /**
     * Selected AP channel.
     */
    ap_cfg.ap.channel = _ap_channel;

    // =====================================================
    // Execute AP Startup
    // =====================================================

    esp_wifi_set_mode(WIFI_MODE_AP);

    esp_wifi_set_config(WIFI_IF_AP,
                        &ap_cfg);

    esp_wifi_start();
}

/**
 * @brief Start Hybrid STA + AP mode.
 *
 * @details
 * Enables simultaneous:
 * - Station mode (STA)
 * - Access Point mode (AP)
 *
 * Hybrid mode is useful for:
 * - Local provisioning
 * - IoT bridge systems
 * - SoftAP fallback access
 * - Simultaneous cloud + local control
 *
 * Features:
 * - Static IP support
 * - Safe configuration validation
 * - Automatic AP fallback naming
 * - Runtime-safe reinitialization
 * - Power-save disabled for stability
 *
 * @param sSsid Target STA SSID.
 * @param sPass Target STA password.
 * @param aName AP SSID name.
 * @param aPass AP password.
 */
void NeuWiFi::startHybrid(const char *sSsid,
                          const char *sPass,
                          const char *aName,
                          const char *aPass)
{
    // =====================================================
    // Input Validation
    // =====================================================

    /**
     * Minimal STA SSID validation.
     * Hybrid mode requires STA target SSID.
     */
    if (sSsid == nullptr || strlen(sSsid) == 0)
        return;

    /**
     * AP fallback name protection.
     */
    const char *ap_name =
        (aName != nullptr && strlen(aName) > 0)
            ? aName
            : "Neu-AP-Hybrid";

    // =====================================================
    // Reset Previous State
    // =====================================================

    /**
     * Stop previous WiFi state before switching mode.
     */
    esp_wifi_stop();

    // =====================================================
    // Network Interface Initialization
    // =====================================================

    if (!_sta_netif)
        _sta_netif = esp_netif_create_default_wifi_sta();

    if (!_ap_netif)
        _ap_netif = esp_netif_create_default_wifi_ap();

    // =====================================================
    // Static IP Logic
    // =====================================================

    if (_use_static_sta)
    {
        /**
         * Disable DHCP client before applying static IP.
         */
        esp_netif_dhcpc_stop(_sta_netif);

        esp_netif_set_ip_info(_sta_netif,
                              &_static_ip_sta);

        esp_netif_set_dns_info(_sta_netif,
                               ESP_NETIF_DNS_MAIN,
                               &_dns_sta);
    }

    if (_use_static_ap)
    {
        /**
         * Disable DHCP server before AP IP update.
         */
        esp_netif_dhcps_stop(_ap_netif);

        esp_netif_set_ip_info(_ap_netif,
                              &_static_ip_ap);

        /**
         * Restart DHCP server.
         */
        esp_netif_dhcps_start(_ap_netif);
    }

    // =====================================================
    // Hardware Configuration
    // =====================================================

    wifi_config_t sta_cfg = {};
    wifi_config_t ap_cfg = {};

    /**
     * Safe STA credential copy.
     */
    strncpy((char *)sta_cfg.sta.ssid,
            sSsid,
            32);

    if (sPass)
    {
        strncpy((char *)sta_cfg.sta.password,
                sPass,
                64);
    }

    /**
     * Safe AP credential copy.
     */
    strncpy((char *)ap_cfg.ap.ssid,
            ap_name,
            32);

    if (aPass)
    {
        strncpy((char *)ap_cfg.ap.password,
                aPass,
                64);
    }

    /**
     * AP security protection.
     */
    ap_cfg.ap.authmode =
        (aPass == nullptr || strlen(aPass) < 8)
            ? WIFI_AUTH_OPEN
            : WIFI_AUTH_WPA2_PSK;

    /**
     * Maximum AP clients.
     */
    ap_cfg.ap.max_connection = _max_conn;

    // =====================================================
    // Execute Hybrid Startup
    // =====================================================

    esp_wifi_set_mode(WIFI_MODE_APSTA);

    esp_wifi_set_config(WIFI_IF_STA,
                        &sta_cfg);

    esp_wifi_set_config(WIFI_IF_AP,
                        &ap_cfg);

    esp_wifi_start();

    /**
     * Disable WiFi power saving.
     *
     * Hybrid systems generally require:
     * - Lower latency
     * - Stable packet timing
     * - Reduced reconnect jitter
     */
    esp_wifi_set_ps(WIFI_PS_NONE);
}