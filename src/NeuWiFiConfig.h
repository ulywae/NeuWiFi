/**
 * @file NeuWiFiConfig.h
 * @author Ulywae (@Neufa)
 * @brief Global compile-time configuration for the NeuWiFi framework.
 *
 * @details
 * This file centralizes all configurable limits used by the NeuWiFi ecosystem.
 *
 * The goal of this configuration layer is:
 * - deterministic memory usage
 * - reduced heap fragmentation
 * - embedded-safe allocation boundaries
 * - predictable runtime behavior
 * - compile-time tuning flexibility
 *
 * Every module inside NeuWiFi references these constants to control:
 * - HTTP route capacities
 * - WebSocket frame buffers
 * - DNS packet staging
 * - POST payload parsing
 * - Multicast DNS (mDNS) Static FreeRTOS Queue bounds
 * - Anti-Private DNS and IP-MAC Binding arrays
 * - internal runtime limits
 *
 * Users may override any value BEFORE including NeuWiFi.h:
 *
 * @code
 * #define USE_CLIENT_TRACKER
 * #define NEU_MDNS_QUEUE_LEN 10
 * #include "NeuWiFi.h"
 * @endcode
 *
 * @note
 * NeuWiFi intentionally uses bounded buffers and fixed capacities
 * wherever possible to preserve embedded runtime stability.
 *
 * @version 1.5
 * @date 2026
 */

#ifndef NEU_WIFI_CONFIG_H
#define NEU_WIFI_CONFIG_H

// =====================================================
// MEMORY PROFILE RECOMMENDATIONS
// =====================================================

/**
 * @brief Recommended memory profiles for various device workloads.
 *
 * @details
 * These are general recommendations for balancing memory usage,
 * networking throughput, and runtime stability.
 *
 * -----------------------------------------------------
 * SMALL DEVICE PROFILE
 * -----------------------------------------------------
 * Suitable for:
 * - simple IoT nodes
 * - sensor devices
 * - low-RAM applications
 *
 * Recommended:
 * - WS Buffer   : 128
 * - POST Buffer : 256
 *
 * -----------------------------------------------------
 * MEDIUM DEVICE PROFILE
 * -----------------------------------------------------
 * Suitable for:
 * - dashboards
 * - LED controllers
 * - device configuration portals
 *
 * Recommended:
 * - WS Buffer   : 512
 * - POST Buffer : 1024
 *
 * -----------------------------------------------------
 * HEAVY NETWORK PROFILE (Captive Portal / Mini MikroTik Mode)
 * -----------------------------------------------------
 * Suitable for:
 * - real-time streaming
 * - large JSON payloads
 * - advanced WebSocket dashboards
 * - multi-client aggressive captive portal hijacking
 *
 * Recommended:
 * - WS Buffer   : 1024+
 * - POST Buffer : 2048+
 *
 * @warning
 * Increasing buffers aggressively may increase:
 * - RAM usage
 * - FreeRTOS stack pressure
 * - packet staging overhead
 */

// =====================================================
// CAPACITY & BUFFER LIMITS
// =====================================================

/**
 * @defgroup NeuConfig_Limits Capacity and Buffer Limits
 * @brief Global compile-time limits for resource allocation.
 * @{
 */

// -----------------------------------------------------
// HTTP ROUTES
// -----------------------------------------------------

#if !defined NEU_MAX_HTTP_ROUTES

/**
 * @brief Maximum number of HTTP routes.
 *
 * @details
 * Controls how many HTTP endpoints can be registered
 * simultaneously inside the internal route table.
 *
 * NeuWiFi uses static route registration patterns to:
 * - reduce heap fragmentation
 * - improve runtime predictability
 * - simplify embedded memory behavior
 *
 * Default: 16
 */
#define NEU_MAX_HTTP_ROUTES 16

#endif

// -----------------------------------------------------
// WEBSOCKET ROUTES
// -----------------------------------------------------

#if !defined NEU_MAX_WS_ROUTES

/**
 * @brief Maximum number of WebSocket routes.
 *
 * @details
 * Defines the maximum number of independent
 * WebSocket endpoints allowed.
 *
 * Default: 4
 */
#define NEU_MAX_WS_ROUTES 4

#endif

// -----------------------------------------------------
// HTTP POST BUFFER
// -----------------------------------------------------

#if !defined NEU_SERVER_POST_BUF_SIZE

/**
 * @brief Maximum HTTP POST parsing buffer size.
 *
 * @details
 * Temporary stack buffer used during:
 * - POST body parsing
 * - form decoding
 * - query extraction
 *
 * Adjusted to 1024 bytes to securely handle modern browser
 * redirect cookies and form data submissions safely.
 *
 * Default: 1024 bytes
 */
#define NEU_SERVER_POST_BUF_SIZE 1024

#endif

// -----------------------------------------------------
// WEBSOCKET BUFFER
// -----------------------------------------------------

#if !defined NEU_SERVER_WS_BUF_SIZE

/**
 * @brief Internal WebSocket packet buffer size.
 *
 * @details
 * Defines the maximum payload size for incoming
 * WebSocket frames staged by the runtime wrapper.
 *
 * Larger values allow:
 * - bigger JSON payloads
 * - binary streaming
 * - dashboard synchronization
 *
 * Default: 512 bytes
 */
#define NEU_SERVER_WS_BUF_SIZE 512

#endif

// -----------------------------------------------------
// DNS BUFFER
// -----------------------------------------------------

#if !defined NEU_DNS_BUFFER_SIZE

/**
 * @brief DNS UDP packet buffer size.
 *
 * @details
 * Used by the captive portal DNS responder
 * for incoming and outgoing UDP DNS packets.
 *
 * 512 bytes is the standard DNS UDP payload limit
 * before packet fragmentation typically occurs.
 *
 * Default: 512 bytes
 */
#define NEU_DNS_BUFFER_SIZE 512

#endif

// -----------------------------------------------------
// MULTICAST DNS (mDNS) CONFIGURATIONS
// -----------------------------------------------------

#if !defined(NEU_MDNS_DEFAULT_INSTANCE)
/**
 * @brief Default instance name for mDNS service registration.
 * @details Fallback label used when the user omits the instance parameter during initiation.
 */
#define NEU_MDNS_DEFAULT_INSTANCE "Neu Service"
#endif

#if !defined(NEU_MDNS_HOST_MAX_LEN)
/**
 * @brief Maximum character length allowed for device hostnames.
 * @details Locks the static size of s_hostname and packet.hostname arrays.
 * Default: 64 bytes
 */
#define NEU_MDNS_HOST_MAX_LEN 64
#endif

#if !defined(NEU_MDNS_INSTANCE_MAX_LEN)
/**
 * @brief Maximum character length allowed for mDNS instance names.
 * @details Locks the static allocation size of s_instance_name array.
 * Default: 32 bytes
 */
#define NEU_MDNS_INSTANCE_MAX_LEN 32
#endif

#if !defined(NEU_MDNS_TEXT_MAX_LEN)
/**
 * @brief Maximum character length allowed for protocol descriptors.
 * @details Controls the buffer capacity for raw protocol strings (e.g., "_tcp").
 * Default: 64 bytes
 */
#define NEU_MDNS_TEXT_MAX_LEN 64
#endif

#if !defined(NEU_MDNS_SERVICE_MAX_LEN)
/**
 * @brief Maximum character length allowed for mDNS service identifier names.
 * @details Locks the static size configuration of s_service_name.
 * Default: 32 bytes
 */
#define NEU_MDNS_SERVICE_MAX_LEN 32
#endif

#if !defined(NEU_MDNS_QUEUE_LEN)
/**
 * @brief Total length slots for the static FreeRTOS Discovery Queue.
 * @details Determines the exact array multiplier sizing for s_queue_storage memory calculations.
 * Default: 5 slots
 */
#define NEU_MDNS_QUEUE_LEN 5
#endif

#if !defined(NEU_MDNS_MAX_TXT_ITEMS)
/**
 * @brief Maximum number of distinct key-value entries inside a single TXT Record.
 * @details Guards loop limits when parsing multidimensional metadata structures.
 * Default: 10 items
 */
#define NEU_MDNS_MAX_TXT_ITEMS 10
#endif

// -----------------------------------------------------
// CLIENT TRACKER / ANTI-PRIVATE DNS BUFFER ENGINE
// -----------------------------------------------------

#if !defined(NEU_MAX_TRACKED_CLIENTS)
/**
 * @brief Maximum concurrent tracked clients inside the network table.
 *
 * @details
 * Controls the internal Ring Buffer sizing for capturing dual identities
 * (IP-MAC Bindings) to prevent IP Spoofing and deploy aggressive captive redirection.
 *
 * Default: 32 clients
 */
#define NEU_MAX_TRACKED_CLIENTS 32
#endif

#if defined(USE_CLIENT_TRACKER)
/**
 * @brief Internal guard macro for enabling Client Binding array features.
 * @details Automatically generated if the user explicitly triggers USE_CLIENT_TRACKER.
 */
#define _USE_CLIENT_TRACKER
#endif

/** @} */ // end of NeuConfig_Limits

// =====================================================
// FEATURE FLAGS
// =====================================================

/**
 * @defgroup NeuConfig_Features Compile-Time Feature Flags
 * @brief Optional modular framework engines.
 * @{
 *
 * Enable modules before including NeuWiFi.h:
 *
 * @code
 * #define USE_HTTP_SERVER
 * #define USE_WEBSOCKET
 * #define USE_CLIENT_TRACKER
 * #include "NeuWiFi.h"
 * @endcode
 */

// #define USE_HTTP_SERVER
// #define USE_HTTP_CLIENT
// #define USE_WEBSOCKET
// #define USE_WEBSOCKET_SERVER

/** @} */ // end of NeuConfig_Features

// =====================================================
// END CONFIG
// =====================================================

#endif // NEU_WIFI_CONFIG_H
