/**
 * @file NeuWiFi.h
 * @author Ulywae (@Neufa)
 * @brief Professional ESP32 networking framework for ESP-IDF.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * NeuWiFi is a modular networking framework
 * built specifically for ESP32 and ESP-IDF.
 *
 * Core features:
 * - WiFi STA mode
 * - WiFi Access Point mode
 * - Hybrid AP + STA mode
 * - Static IP configuration
 * - WiFi event callbacks
 * - Network scanning
 * - UDP communication
 * - Lightweight HTTP server
 * - WebSocket support
 * - Captive portal DNS
 * - HTTP client support
 *
 * Architecture goals:
 * - Lightweight runtime
 * - Deterministic behavior
 * - Embedded-safe networking
 * - Modular feature system
 * - Beginner-friendly API
 * - Production-grade reliability
 *
 * Optional modules are enabled using:
 * - USE_HTTP_SERVER
 * - USE_WEBSOCKET_SERVER
 * - USE_WEBSOCKET
 * - USE_HTTP_CLIENT
 *
 * Framework structure:
 * NeuWiFi
 * ├── WiFi Core
 * ├── UDP Runtime
 * ├── HTTP Server
 * ├── WebSocket Engine
 * ├── DNS Captive Portal
 * └── HTTP Client
 */

#ifndef NEU_WIFI_H
#define NEU_WIFI_H

// =====================================================
// Platform Validation
// =====================================================

#if !defined(ESP32)

/**
 * @brief Platform protection.
 *
 * NeuWiFi currently supports:
 * - ESP32
 * - ESP-IDF runtime
 */
#error "Platform not supported! NeuWiFi currently only supports ESP32."

#endif

// =====================================================
// Core Networking Modules
// =====================================================

/**
 * @brief Main WiFi engine.
 *
 * Provides:
 * - STA mode
 * - AP mode
 * - Hybrid AP+STA mode
 * - Static IP
 * - WiFi scanning
 * - Event callbacks
 */
#include "wifi/_NeuWiFi.h"

/**
 * @brief Lightweight UDP runtime.
 *
 * Provides:
 * - UDP send/receive
 * - Broadcast
 * - Multicast
 * - Callback-based networking
 */
#include "udp/UDP.h"

// =====================================================
// HTTP / WebSocket Modules
// =====================================================

/**
 * Optional modules:
 * - HTTP Server
 * - WebSocket Server
 * - Captive Portal DNS
 *
 * Enabled only when requested
 * to reduce firmware size.
 */
#if defined(USE_HTTP_SERVER) ||      \
    defined(USE_WEBSOCKET_SERVER) || \
    defined(USE_WEBSOCKET)

/**
 * @brief Lightweight HTTP/WebSocket server.
 */
#include "http/httpServer/httpServer.h"

/**
 * @brief Lightweight captive portal DNS server.
 */
#include "dns/dnsServer.h"

#endif

// =====================================================
// HTTP Client Module
// =====================================================

/**
 * Optional HTTP client module.
 *
 * Provides:
 * - HTTP GET
 * - HTTP POST
 * - HTTPS support
 * - Custom headers
 * - Lightweight response handling
 */
#if defined(USE_HTTP_CLIENT)

/**
 * @brief Lightweight HTTP client runtime.
 */
#include "http/httpClient/httpClient.h"

#endif

#endif // NEU_WIFI_H