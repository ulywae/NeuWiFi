/**
 * @file NeuWiFi_AP.cpp
 * @brief Access Point and Hybrid runtime implementation for NeuWiFiClass.
 *
 * This module implements all logic related to running the device as an Access Point (AP),
 * as well as operating in Hybrid mode where both Station and Access Point modes run simultaneously.
 * It handles configuration, startup sequence, IP addressing, DHCP control, and mode management.
 *
 * Core Responsibilities:
 * - AP static IP configuration including gateway and subnet settings
 * - Access Point mode startup and shutdown sequence
 * - Hybrid (STA + AP) concurrent mode initialization and management
 * - AP channel configuration and validation
 * - DHCP server control and IP assignment logic
 * - Safe mode switching and coexistence handling
 *
 * Runtime Features:
 * - Static IP assignment support for Access Point interface
 * - Full DHCP server management and control
 * - Stable Hybrid STA+AP coexistence on single radio hardware
 * - Smart AP channel synchronization logic
 * - Runtime-safe WiFi mode switching without resource conflict
 * - Automatic hostname assignment to both interfaces
 *
 * Design Goals:
 * - Deterministic AP lifecycle and state management
 * - Centralized WiFi configuration handling
 * - Minimal hidden side effects or unexpected state changes
 * - Production-grade embedded stability and fault tolerance
 * - Clear separation between configuration and execution
 *
 * @author Ulywae (@Neufa)
 * @date 2026
 * @version 1.1.0
 */

#include "wifi/_NeuWiFi.h"
#include "SystemState/SharedState.h"

// =====================================================
// STATIC AP IP CONFIGURATION
// =====================================================

/**
 * @brief Configure static IP settings for Access Point interface using string inputs.
 *
 * Sets fixed network addressing parameters to be used by the AP interface instead
 * of default dynamic settings. DHCP server will operate on this defined address range.
 *
 * Configures:
 * - AP IP address
 * - Subnet mask
 * - Gateway address
 *
 * Validation:
 * - All IP strings are validated for correct format
 * - Invalid or malformed IP strings are rejected immediately
 *
 * @param ip Null-terminated string containing desired AP IP address (e.g., "192.168.4.1")
 * @param subnet Null-terminated string containing subnet mask (e.g., "255.255.255.0")
 * @param gateway Null-terminated string containing gateway address (usually same as IP)
 *
 * @return true Configuration accepted and stored.
 * @return false Invalid format detected, configuration not saved.
 */
bool NeuWiFiClass::setAPIP(const char *ip,
                           const char *subnet,
                           const char *gateway)
{
    uint32_t check_ip = ipaddr_addr(ip);
    uint32_t check_subnet = ipaddr_addr(subnet);
    uint32_t check_gateway = ipaddr_addr(gateway);

    if (check_ip == IPADDR_NONE || check_subnet == IPADDR_NONE || check_gateway == IPADDR_NONE)
    {
        return false; // Invalid IP
    }

    _use_static_ap = true;

    _static_ip_ap.ip.addr = check_ip;
    _static_ip_ap.netmask.addr = check_subnet;
    _static_ip_ap.gw.addr = check_gateway;

    return true;
}

/**
 * @brief Configure static IP settings for Access Point using IPConfig structure.
 *
 * Alternative method to set static AP addressing using pre-validated configuration object.
 * Preferred when sharing configuration logic between STA and AP modes.
 *
 * @param config Reference to IPConfig object containing complete network addressing
 *
 * @return true Configuration accepted and stored.
 * @return false Configuration object contains invalid data or subnet settings.
 */
bool NeuWiFiClass::setAPIP(const IPConfig &config)
{
    if (config.getIP() == 0 || config.getSubnet() == 0 || !config.isValidSubnet())
        return false;

    _use_static_ap = true;

    _static_ip_ap.ip.addr = config.getLwIP_IP();
    _static_ip_ap.netmask.addr = config.getLwIP_Subnet();
    _static_ip_ap.gw.addr = config.getLwIP_Gateway();

    return true;
}

// =====================================================
// AP CHANNEL CONTROL
// =====================================================

/**
 * @brief Set operating WiFi channel for Access Point interface.
 *
 * Defines which frequency channel the AP will broadcast and operate on.
 * Used both in standalone AP mode and Hybrid mode for channel synchronization.
 *
 * Valid range:
 * - 1 to 13 (standard 2.4GHz channels)
 *
 * Safety:
 * - Invalid values (out of range) automatically fallback to channel 1
 *
 * @param channel Desired WiFi channel number
 */
void NeuWiFiClass::setChannelAP(uint8_t channel)
{
    if (channel < 1 || channel > 13)
        _ap_channel = 1;
    else
        _ap_channel = channel;
}

// =====================================================
// ACCESS POINT STARTUP
// =====================================================

/**
 * @brief Start WiFi subsystem in Access Point mode only.
 *
 * Initializes and starts the device as a standalone wireless access point
 * allowing other devices to connect and join the local network.
 *
 * Features:
 * - Complete AP runtime initialization sequence
 * - Automatic WPA2/Open authentication selection based on password length
 * - DHCP server integration and management
 * - Static IP support with correct ordering to ensure route regeneration
 * - Hostname configuration for network identification
 *
 * Runtime Notes:
 * - Existing WiFi state is safely stopped first to prevent conflicts
 * - AP network interface created lazily only when needed
 * - SharedState updated automatically with IP and readiness status
 * - Default SSID used if name parameter is empty or NULL
 *
 * @param name Null-terminated string for AP SSID (broadcast name)
 * @param pass Null-terminated string for password (minimum 8 characters for security)
 */
void NeuWiFiClass::startAP(
    const char *name,
    const char *pass)
{
    const char *ap_name = (name != nullptr && strlen(name) > 0) ? name : "Neu-AccessPoint";

    if (!_ap_netif)
    {
        _ap_netif = esp_netif_create_default_wifi_ap();
    }

    esp_wifi_stop();
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_netif_set_hostname(_ap_netif, _hostname);

    wifi_config_t ap_cfg = {};
    memset(&ap_cfg, 0, sizeof(wifi_config_t));
    strncpy((char *)ap_cfg.ap.ssid, ap_name, sizeof(ap_cfg.ap.ssid) - 1);
    ap_cfg.ap.ssid[sizeof(ap_cfg.ap.ssid) - 1] = '\0';

    if (pass && strlen(pass) >= 8)
    {
        strncpy((char *)ap_cfg.ap.password, pass, sizeof(ap_cfg.ap.password) - 1);
        ap_cfg.ap.password[sizeof(ap_cfg.ap.password) - 1] = '\0';
        ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }
    else
    {
        ap_cfg.ap.authmode = WIFI_AUTH_OPEN;
    }

    ap_cfg.ap.max_connection = _max_conn;
    ap_cfg.ap.channel = _ap_channel;

    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);

    // SAFE KEY: Execute Static IP set BEFORE Wi-Fi start so that LwIP regenerates routes correctly
    if (_use_static_ap)
    {
        esp_netif_dhcps_stop(_ap_netif);
        esp_netif_set_ip_info(_ap_netif, &_static_ip_ap);
        esp_netif_dhcps_start(_ap_netif);
    }

    esp_wifi_start();
}

/**
 * @brief Start concurrent Hybrid mode: both Station + Access Point active.
 *
 * Enables simultaneous operation where device connects to an external WiFi network
 * as a Station, while also broadcasting its own Access Point network.
 *
 * Features:
 * - Complete STA runtime initialization
 * - Complete AP runtime initialization
 * - WPA2/Open authentication support for both interfaces independently
 * - DHCP client (STA) and DHCP server (AP) integration
 * - Static IP support for both modes with independent configuration
 * - Hostname configuration applied to both interfaces
 * - Automatic power saving disabled for stable coexistence
 *
 * Runtime Notes:
 * - Existing WiFi state is safely stopped first
 * - STA and AP network interfaces created lazily
 * - Channel synchronization logic applies if enabled
 * - Connection attempt triggered automatically after startup
 * - SharedState updated automatically for both interfaces
 *
 * @param sSsid Null-terminated string: External network SSID to connect to
 * @param sPass Null-terminated string: Password for external network
 * @param aName Null-terminated string: SSID name for our own Access Point
 * @param aPass Null-terminated string: Password for our Access Point
 */
void NeuWiFiClass::startHybrid(
    const char *sSsid,
    const char *sPass,
    const char *aName,
    const char *aPass)
{
    if (sSsid == nullptr || strlen(sSsid) == 0)
        return;
    const char *ap_name = (aName != nullptr && strlen(aName) > 0) ? aName : "Neu-AP-Hybrid";

    if (!_sta_netif)
        _sta_netif = esp_netif_create_default_wifi_sta();
    if (!_ap_netif)
        _ap_netif = esp_netif_create_default_wifi_ap();

    esp_wifi_stop();
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_netif_set_hostname(_sta_netif, _hostname);
    esp_netif_set_hostname(_ap_netif, _hostname);

    wifi_config_t sta_cfg = {};
    wifi_config_t ap_cfg = {};
    memset(&sta_cfg, 0, sizeof(wifi_config_t));
    memset(&ap_cfg, 0, sizeof(wifi_config_t));

    strncpy((char *)sta_cfg.sta.ssid, sSsid, sizeof(sta_cfg.sta.ssid) - 1);
    sta_cfg.sta.ssid[sizeof(sta_cfg.sta.ssid) - 1] = '\0';
    if (sPass)
    {
        strncpy((char *)sta_cfg.sta.password, sPass, sizeof(sta_cfg.sta.password) - 1);
        sta_cfg.sta.password[sizeof(sta_cfg.sta.password) - 1] = '\0';
    }
    sta_cfg.sta.threshold.authmode = (sPass == nullptr || strlen(sPass) == 0) ? WIFI_AUTH_OPEN : _min_auth_sta;

    strncpy((char *)ap_cfg.ap.ssid, ap_name, sizeof(ap_cfg.ap.ssid) - 1);
    ap_cfg.ap.ssid[sizeof(ap_cfg.ap.ssid) - 1] = '\0';
    if (aPass && strlen(aPass) >= 8)
    {
        strncpy((char *)ap_cfg.ap.password, aPass, sizeof(ap_cfg.ap.password) - 1);
        ap_cfg.ap.password[sizeof(ap_cfg.ap.password) - 1] = '\0';
        ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }
    else
    {
        ap_cfg.ap.authmode = WIFI_AUTH_OPEN;
    }
    ap_cfg.ap.max_connection = _max_conn;
    ap_cfg.ap.channel = _ap_channel;

    esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);

    // SAFE KEY: Set Static IP info on LwIP interface before start switch button is pressed
    if (_use_static_sta)
    {
        esp_netif_dhcpc_stop(_sta_netif);
        esp_netif_set_ip_info(_sta_netif, &_static_ip_sta);
        esp_netif_set_dns_info(_sta_netif, ESP_NETIF_DNS_MAIN, &_dns_primary_sta);
        esp_netif_set_dns_info(_sta_netif, ESP_NETIF_DNS_BACKUP, &_dns_secondary_sta);
    }

    if (_use_static_ap)
    {
        esp_netif_dhcps_stop(_ap_netif);
        esp_netif_set_ip_info(_ap_netif, &_static_ip_ap);
        esp_netif_dhcps_start(_ap_netif);
    }

    esp_wifi_start();
    esp_wifi_set_ps(WIFI_PS_NONE);

    esp_wifi_connect();
}