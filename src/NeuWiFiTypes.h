/**
 * @file NeuWiFiConfig.h
 * @brief Core type definitions for NeuWiFi framework.
 * @author Ulywae
 *
 * Updated: 2026
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "IPConfig.h"

/**
 * @struct WiFiScanResult
 * @brief Lightweight WiFi scan result container.
 *
 * Optimized with packed attribute to save RAM
 * when creating a large scan result array.
 */
typedef struct __attribute__((packed))
{
    char ssid[33];   /**< @brief SSID (network name) of the Access Point. */
    int8_t rssi;     /**< @brief Signal strength in dBm. */
    uint8_t channel; /**< @brief Primary WiFi channel used by the Access Point. */
} WiFiScanResult;

/**
 * @struct WiFiStatus
 * @brief Detailed runtime WiFi network status information.
 *
 * Variable grouping is reordered from largest data type
 * to smallest for efficient memory alignment on 32-bit processors.
 */
typedef struct
{
    int authmode;           /**< @brief WiFi authentication mode. */
    char ip[16];            /**< @brief Device IPv4 address. */
    char netmask[16];       /**< @brief Network subnet mask. */
    char gateway[16];       /**< @brief Default gateway address. */
    char dns_primary[16];   /**< @brief Primary DNS server address. */
    char dns_secondary[16]; /**< @brief Secondary DNS server address. */
    char ssid[33];          /**< @brief Active or connected SSID name. */
    char mac[18];           /**< @brief Device MAC address (XX:XX:XX:XX:XX:XX). */
    char bssid[18];         /**< @brief Access Point BSSID. */
    int8_t rssi;            /**< @brief Current signal strength in dBm. */
    int8_t tx_power;        /**< @brief WiFi transmit power in dBm. */
    uint8_t channel;        /**< @brief Active WiFi channel. */
} WiFiStatus;

/**
 * @struct HttpHeader
 * @brief Lightweight HTTP header key-value container.
 */
struct HttpHeader
{
    const char *key;   /**< @brief HTTP header field name. */
    const char *value; /**< @brief HTTP header field value. */
};

/**
 * @enum NeuWSEvent
 * @brief WebSocket event type definitions.
 */
typedef enum
{
    WS_EVENT_CONNECT,    /**< @brief WebSocket client successfully connected. */
    WS_EVENT_DISCONNECT, /**< @brief WebSocket client disconnected. */
    WS_EVENT_TEXT,       /**< @brief Text message received. */
    WS_EVENT_BINARY,     /**< @brief Binary message received. */
    WS_EVENT_PING,       /**< @brief Ping frame received. */
    WS_EVENT_PONG,       /**< @brief Pong frame received. */
    WS_EVENT_ERROR       /**< @brief WebSocket error event. */
} NeuWSEvent;

class IPConfig;