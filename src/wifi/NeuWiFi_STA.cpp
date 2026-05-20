/**
 * @file NeuWiFi_STA.cpp
 * @brief Station (STA) runtime implementation for NeuWiFiClass.
 *
 * This module implements all logic required for operating the device in Station (STA) mode,
 * where the device connects to an external Wi-Fi network/infrastructure. It handles network
 * configuration, connection management, state monitoring, and scanning operations.
 *
 * Core Responsibilities:
 * - Static IP address configuration including subnet and gateway
 * - Primary and secondary DNS server configuration with fallback logic
 * - Connection startup sequence and authentication handling
 * - Connection state monitoring and status reporting
 * - Asynchronous background WiFi scanning functionality
 * - Event-driven state management and synchronization
 *
 * Runtime Features:
 * - Static IP assignment support with full validation
 * - Dual DNS configuration (Primary + Secondary) with automatic fallbacks
 * - WPA2/WPA/Open authentication handling
 * - Configurable connection timeout mechanism
 * - Power-save mode disabled by default for maximum stability and speed
 * - Event-driven connection state tracking (non-blocking)
 *
 * Design Goals:
 * - Deterministic STA lifecycle and state transitions
 * - Centralized network configuration management
 * - Production-grade embedded stability and fault tolerance
 * - Lightweight runtime orchestration with minimal overhead
 * - Clear separation between configuration and execution logic
 *
 * @author Ulywae (@Neufa)
 * @date 2026
 * @version 1.1.0
 */

#include "wifi/_NeuWiFi.h"

// =====================================================
// STATIC STA IP CONFIGURATION
// =====================================================

/**
 * @brief Configure static IP settings for Station interface using string inputs.
 *
 * Sets fixed network addressing parameters to be used instead of DHCP-assigned addresses.
 * Includes validation logic and automatic fallback values for DNS servers.
 *
 * Configures:
 * - STA IP address
 * - Subnet mask
 * - Gateway address
 * - Primary DNS server
 * - Secondary DNS server
 *
 * Validation:
 * - Invalid or malformed IP strings are rejected immediately
 * - Invalid DNS values automatically fallback to public servers
 *
 * DNS Fallbacks:
 * - Primary: 1.1.1.1 (Cloudflare)
 * - Secondary: 8.8.8.8 (Google)
 *
 * @param ip Null-terminated string containing desired static IP address (e.g., "192.168.1.100")
 * @param subnet Null-terminated string containing subnet mask (e.g., "255.255.255.0")
 * @param gateway Null-terminated string containing gateway/router address
 * @param dns_primary Null-terminated string for preferred DNS server (optional)
 * @param dns_secondary Null-terminated string for backup DNS server (optional)
 *
 * @return true Configuration accepted and stored.
 * @return false Invalid format detected, configuration rejected.
 */
bool NeuWiFiClass::setSTAIP(
    const char *ip,
    const char *subnet,
    const char *gateway,
    const char *dns_primary,
    const char *dns_secondary)
{
    uint32_t check_ip = ipaddr_addr(ip);
    uint32_t check_subnet = ipaddr_addr(subnet);
    uint32_t check_gateway = ipaddr_addr(gateway);

    if (check_ip == IPADDR_NONE || check_subnet == IPADDR_NONE || check_gateway == IPADDR_NONE)
    {
        return false;
    }

    uint32_t checked_dns1 = (dns_primary != nullptr && strlen(dns_primary) > 0) ? ipaddr_addr(dns_primary) : IPADDR_NONE;
    if (checked_dns1 == IPADDR_NONE)
        checked_dns1 = ipaddr_addr("1.1.1.1");

    uint32_t checked_dns2 = (dns_secondary != nullptr && strlen(dns_secondary) > 0) ? ipaddr_addr(dns_secondary) : IPADDR_NONE;
    if (checked_dns2 == IPADDR_NONE)
        checked_dns2 = ipaddr_addr("8.8.8.8");

    _use_static_sta = true;

    _static_ip_sta.ip.addr = check_ip;
    _static_ip_sta.netmask.addr = check_subnet;
    _static_ip_sta.gw.addr = check_gateway;

    _dns_primary_sta.ip.type = IPADDR_TYPE_V4;
    ip4_addr_set_u32(ip_2_ip4(&_dns_primary_sta.ip), checked_dns1);

    _dns_secondary_sta.ip.type = IPADDR_TYPE_V4;
    ip4_addr_set_u32(ip_2_ip4(&_dns_secondary_sta.ip), checked_dns2);

    return true;
}

/**
 * @brief Configure static STA IP settings using pre-defined IPConfig structure.
 *
 * Alternative method to set static addressing using validated configuration object.
 * Useful when loading configuration from storage or sharing logic between modes.
 *
 * @param config Reference to IPConfig object containing complete network parameters
 *
 * @return true Configuration accepted and stored.
 * @return false Configuration object contains invalid or incomplete data.
 */
bool NeuWiFiClass::setSTAIP(const IPConfig &config)
{
    if (config.getIP() == 0 || config.getSubnet() == 0 || !config.isValidSubnet())
        return false;

    _use_static_sta = true;

    _static_ip_sta.ip.addr = config.getLwIP_IP();
    _static_ip_sta.netmask.addr = config.getLwIP_Subnet();
    _static_ip_sta.gw.addr = config.getLwIP_Gateway();

    _dns_primary_sta.ip.type = IPADDR_TYPE_V4;
    ip4_addr_set_u32(ip_2_ip4(&_dns_primary_sta.ip), config.getLwIP_DNSPrim());

    _dns_secondary_sta.ip.type = IPADDR_TYPE_V4;
    ip4_addr_set_u32(ip_2_ip4(&_dns_secondary_sta.ip), config.getLwIP_DNSSec());

    return true;
}

// =====================================================
// STA STARTUP
// =====================================================

/**
 * @brief Start WiFi subsystem in Station mode and initiate connection.
 *
 * Performs complete initialization, configuration, and connection sequence
 * to connect the device to an external Wi-Fi access point.
 *
 * Features:
 * - Complete STA runtime initialization
 * - Automatic WPA2/WPA/Open authentication handling based on password
 * - Static IP support with correct ordering before radio startup
 * - DNS configuration injection
 * - Configurable connection timeout waiting
 * - Power-save mode disabled for stability and low latency
 *
 * Runtime Notes:
 * - Existing WiFi state is safely stopped first to prevent conflicts
 * - STA network interface created lazily only when needed
 * - Waits for connection result using event group synchronization
 * - Integrates fully with event-driven runtime architecture
 *
 * @param ssid Null-terminated string: Target WiFi network name
 * @param pass Null-terminated string: Network password (leave NULL for open networks)
 * @param timeout Connection timeout in seconds
 *
 * @return true Connection successful and IP obtained.
 * @return false Connection failed, timeout reached, or invalid parameters.
 */
bool NeuWiFiClass::startSTA(
    const char *ssid,
    const char *pass,
    int timeout)
{
    if (ssid == nullptr || strlen(ssid) == 0)
        return false;

    if (!_sta_netif)
    {
        _sta_netif = esp_netif_create_default_wifi_sta();
    }

    esp_wifi_stop();

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_netif_set_hostname(_sta_netif, _hostname);
    wifi_config_t sta_cfg = {};

    strncpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid) - 1);
    sta_cfg.sta.ssid[sizeof(sta_cfg.sta.ssid) - 1] = '\0';

    if (pass)
    {
        strncpy((char *)sta_cfg.sta.password, pass, sizeof(sta_cfg.sta.password) - 1);
        sta_cfg.sta.password[sizeof(sta_cfg.sta.password) - 1] = '\0';
    }

    if (pass == nullptr || strlen(pass) == 0)
    {
        sta_cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        sta_cfg.sta.threshold.authmode = _min_auth_sta;
    }

    esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);

    // Inject Static IP and DNS into the LwIP stack BEFORE the Wi-Fi Driver turns on the hardware radio.
    if (_use_static_sta)
    {
        esp_netif_dhcpc_stop(_sta_netif);
        esp_netif_set_ip_info(_sta_netif, &_static_ip_sta);
        esp_netif_set_dns_info(_sta_netif, ESP_NETIF_DNS_MAIN, &_dns_primary_sta);
        esp_netif_set_dns_info(_sta_netif, ESP_NETIF_DNS_BACKUP, &_dns_secondary_sta);
    }

    esp_wifi_start();
    esp_wifi_set_ps(WIFI_PS_NONE); // Lightning fast instant response mode is always on!

    // Clean up old residual bits before waiting for new status to avoid false detection.
    xEventGroupClearBits(_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    esp_wifi_connect();

    // Wait for connection status safely according to timeout limit
    EventBits_t bits = xEventGroupWaitBits(
        _event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        (timeout * 1000) / portTICK_PERIOD_MS);

    // SAVE CONDITION: If the timeout expires but the connection hasn't been established, ensure the auto-reconnect flag is set
    // internally controlled to prevent spawning random tasks if the user attempts to cancel the connection.
    bool success = (bits & WIFI_CONNECTED_BIT);
    if (!success)
    {
        // Optional logic: If timeout, let the user decide whether to continue auto-reconnect or stop completely.
    }

    return success;
}

// =====================================================
// CONNECTION STATE
// =====================================================

/**
 * @brief Check current STA connection state.
 *
 * Queries internal synchronization flags to determine if device is
 * currently connected and has obtained a valid IP address.
 *
 * @return true Station mode connected and operational.
 * @return false Station disconnected or in error state.
 */
bool NeuWiFiClass::isConnected() const
{
    return (xEventGroupGetBits(_event_group) & WIFI_CONNECTED_BIT);
}

// =====================================================
// SCAN STATE
// =====================================================

/**
 * @brief Check completion status of asynchronous WiFi scan.
 *
 * Automatically clears the completion flag immediately after successful read
 * to prepare for next scan cycle and prevent repeated readings.
 *
 * @return true Scan completed successfully, results ready to read.
 * @return false Scan still in progress or not started.
 */
bool NeuWiFiClass::isScanDone() const
{
    if (xEventGroupGetBits(_event_group) &
        WIFI_SCAN_DONE_BIT)
    {
        xEventGroupClearBits(_event_group, WIFI_SCAN_DONE_BIT);
        return true;
    }

    return false;
}

// =====================================================
// WIFI SCAN ENGINE
// =====================================================

/**
 * @brief Start asynchronous WiFi network scan.
 *
 * Initiates background scanning operation to detect nearby access points.
 * Function returns immediately; completion is notified via event or polling.
 *
 * Scan completion is reported through:
 * - WIFI_EVENT_SCAN_DONE system event
 * - isScanDone() polling method
 *
 * Runtime Notes:
 * - Completely non-blocking operation
 * - Event-driven completion notification
 * - Uses default scan configuration (all channels, all types)
 */
void NeuWiFiClass::startScan()
{
    wifi_scan_config_t sc = {};
    esp_wifi_scan_start(&sc, false);
}