/**
 * @file NeuWiFi_STA.cpp
 * @author Ulywae
 * @brief Station mode implementation for NeuWiFi.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * This file contains:
 * - STA mode initialization
 * - Static IP configuration
 * - WiFi connection management
 * - Scan management
 * - Connection state helpers
 *
 * Design goals:
 * - Deterministic STA behavior
 * - Runtime-safe connection handling
 * - Defensive configuration validation
 * - Embedded-friendly memory usage
 * - Low-latency WiFi operation
 */

#include "_NeuWiFi.h"

/**
 * @brief Configure static IP settings for STA mode.
 *
 * @details
 * Enables static IPv4 configuration for the
 * station interface.
 *
 * Configured parameters:
 * - Static IP
 * - Subnet mask
 * - Gateway
 * - DNS server
 *
 * @param ip Static IP address.
 * @param subnet Network subnet mask.
 * @param gateway Gateway address.
 * @param dns DNS server address.
 */
void NeuWiFi::setStaticSTA(const char *ip,
                           const char *subnet,
                           const char *gateway,
                           const char *dns)
{
    _use_static_sta = true;

    // =====================================================
    // Store Static IP Configuration
    // =====================================================

    _static_ip_sta.ip.addr =
        ipaddr_addr(ip);

    _static_ip_sta.netmask.addr =
        ipaddr_addr(subnet);

    _static_ip_sta.gw.addr =
        ipaddr_addr(gateway);

    // =====================================================
    // Configure DNS
    // =====================================================

    _dns_sta.ip.u_addr.ip4.addr =
        ipaddr_addr(dns);

    _dns_sta.ip.type =
        IPADDR_TYPE_V4;
}

/**
 * @brief Configure static STA using IPConfig structure.
 *
 * @details
 * Automatically falls back to:
 * - 8.8.8.8
 *
 * if DNS field is empty.
 *
 * @param config IP configuration structure.
 */
void NeuWiFi::setStaticSTA(IPConfig config)
{
    /**
     * DNS fallback protection.
     */
    const char *targetDns =
        (strlen(config.dns) > 0)
            ? config.dns
            : "8.8.8.8";

    setStaticSTA(config.ip,
                 config.netmask,
                 config.gateway,
                 targetDns);
}

/**
 * @brief Start Station mode and connect to AP.
 *
 * @details
 * Initializes WiFi STA mode and performs
 * synchronous connection wait using Event Groups.
 *
 * Features:
 * - Static IP support
 * - Safe credential validation
 * - Auto OPEN-mode fallback
 * - Authentication threshold filtering
 * - Timeout-based connection wait
 * - Power-save disabled for stability
 *
 * Runtime protections:
 * - Null SSID guard
 * - Empty SSID guard
 * - Safe string copy
 * - Deterministic connection state tracking
 *
 * @param ssid Target SSID.
 * @param pass Target password.
 * @param timeout Timeout in seconds.
 *
 * @return true if connection succeeded.
 * @return false if connection failed or timed out.
 */
bool NeuWiFi::startSTA(const char *ssid,
                       const char *pass,
                       int timeout)
{
    // =====================================================
    // Input Validation
    // =====================================================

    /**
     * Prevent invalid STA configuration.
     */
    if (ssid == nullptr ||
        strlen(ssid) == 0)
    {
        return false;
    }

    // =====================================================
    // Reset Previous WiFi State
    // =====================================================

    /**
     * Ensure clean runtime state before reconnecting.
     */
    esp_wifi_stop();

    // =====================================================
    // Initialize STA Network Interface
    // =====================================================

    if (!_sta_netif)
    {
        _sta_netif =
            esp_netif_create_default_wifi_sta();
    }

    // =====================================================
    // Apply Static IP Configuration
    // =====================================================

    if (_use_static_sta)
    {
        /**
         * Stop DHCP client before assigning static IP.
         */
        esp_netif_dhcpc_stop(_sta_netif);

        esp_netif_set_ip_info(_sta_netif,
                              &_static_ip_sta);

        esp_netif_set_dns_info(_sta_netif,
                               ESP_NETIF_DNS_MAIN,
                               &_dns_sta);
    }

    // =====================================================
    // Apply Hostname
    // =====================================================

    esp_netif_set_hostname(_sta_netif,
                           _hostname);

    // =====================================================
    // Prepare STA Configuration
    // =====================================================

    wifi_config_t sta_cfg = {};

    /**
     * Safe SSID copy.
     */
    strncpy((char *)sta_cfg.sta.ssid,
            ssid,
            32);

    /**
     * Safe password copy.
     */
    if (pass)
    {
        strncpy((char *)sta_cfg.sta.password,
                pass,
                64);
    }

    // =====================================================
    // Authentication Logic
    // =====================================================

    /**
     * Empty password automatically switches
     * authentication threshold to OPEN.
     */
    if (pass == nullptr ||
        strlen(pass) == 0)
    {
        sta_cfg.sta.threshold.authmode =
            WIFI_AUTH_OPEN;
    }
    else
    {
        sta_cfg.sta.threshold.authmode =
            _min_auth_sta;
    }

    // =====================================================
    // Execute STA Startup
    // =====================================================

    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_wifi_set_config(WIFI_IF_STA,
                        &sta_cfg);

    esp_wifi_start();

    /**
     * Disable power-saving for:
     * - lower latency
     * - stable streaming
     * - deterministic networking
     */
    esp_wifi_set_ps(WIFI_PS_NONE);

    // =====================================================
    // Wait for Connection Result
    // =====================================================

    EventBits_t bits =
        xEventGroupWaitBits(
            _event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            (timeout * 1000) / portTICK_PERIOD_MS);

    return (bits & WIFI_CONNECTED_BIT);
}

/**
 * @brief Check current STA connection state.
 *
 * @details
 * Reads internal event flags to determine
 * connection state.
 *
 * @return true if STA connected.
 * @return false otherwise.
 */
bool NeuWiFi::isConnected() const
{
    return (
        xEventGroupGetBits(_event_group) &
        WIFI_CONNECTED_BIT);
}

/**
 * @brief Check scan completion state.
 *
 * @details
 * Detects whether asynchronous scan
 * operation has completed.
 *
 * Automatically clears scan flag after read.
 *
 * @return true if scan completed.
 * @return false otherwise.
 */
bool NeuWiFi::isScanDone() const
{
    if (xEventGroupGetBits(_event_group) &
        WIFI_SCAN_DONE_BIT)
    {
        /**
         * Auto-clear completion flag.
         */
        xEventGroupClearBits(_event_group,
                             WIFI_SCAN_DONE_BIT);

        return true;
    }

    return false;
}

/**
 * @brief Start asynchronous WiFi scan.
 *
 * @details
 * Starts non-blocking WiFi scan using
 * default scan configuration.
 *
 * Scan completion is later reported through:
 * - WIFI_EVENT_SCAN_DONE
 * - isScanDone()
 */
void NeuWiFi::startScan()
{
    wifi_scan_config_t sc = {};

    /**
     * Start asynchronous scan.
     */
    esp_wifi_scan_start(&sc,
                        false);
}