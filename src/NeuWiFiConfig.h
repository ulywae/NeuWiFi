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
 * - internal runtime limits
 *
 * Users may override any value BEFORE including NeuWiFi.h:
 *
 * @code
 * #define NEU_SERVER_WS_BUF_SIZE 1024
 * #include "NeuWiFi.h"
 * @endcode
 *
 * @note
 * NeuWiFi intentionally uses bounded buffers and fixed capacities
 * wherever possible to preserve embedded runtime stability.
 *
 * @version 1.0
 * @date 2026
 */

#ifndef NEU_WIFI_CONFIG
#define NEU_WIFI_CONFIG

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
 * - WS Buffer   : 256
 * - POST Buffer : 512
 *
 * -----------------------------------------------------
 * HEAVY NETWORK PROFILE
 * -----------------------------------------------------
 * Suitable for:
 * - real-time streaming
 * - large JSON payloads
 * - advanced WebSocket dashboards
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

#ifndef NEU_MAX_HTTP_ROUTES

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

#ifndef NEU_MAX_WS_ROUTES

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

#ifndef NEU_SERVER_POST_BUF_SIZE

/**
 * @brief Maximum HTTP POST parsing buffer size.
 *
 * @details
 * Temporary stack buffer used during:
 * - POST body parsing
 * - form decoding
 * - query extraction
 *
 * Keep this value reasonably small to avoid:
 * - excessive stack usage
 * - task instability
 * - unnecessary RAM pressure
 *
 * Default: 512 bytes
 */
#define NEU_SERVER_POST_BUF_SIZE 512

#endif

// -----------------------------------------------------
// WEBSOCKET BUFFER
// -----------------------------------------------------

#ifndef NEU_SERVER_WS_BUF_SIZE

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
 * Smaller values reduce:
 * - RAM usage
 * - temporary packet staging overhead
 *
 * Default: 256 bytes
 */
#define NEU_SERVER_WS_BUF_SIZE 256

#endif

// -----------------------------------------------------
// DNS BUFFER
// -----------------------------------------------------

#ifndef NEU_DNS_BUFFER_SIZE

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
 * Increasing this value is usually unnecessary
 * for captive portal implementations.
 *
 * Default: 512 bytes
 */
#define NEU_DNS_BUFFER_SIZE 512

#endif

/** @} */ // end of NeuConfig_Limits

// =====================================================
// FEATURE FLAGS
// =====================================================

/**
 * @defgroup NeuConfig_Features Compile-Time Feature Flags
 * @brief Optional framework modules.
 * @{
 *
 * Enable modules before including NeuWiFi.h:
 *
 * @code
 * #define USE_HTTP_SERVER
 * #define USE_HTTP_CLIENT
 * #define USE_WEBSOCKET
 * #include "NeuWiFi.h"
 * @endcode
 */

// #define USE_HTTP_SERVER
// #define USE_HTTP_CLIENT
// #define USE_WEBSOCKET
// #define USE_WEBSOCKET_SERVER

/** @} */

// =====================================================
// END CONFIG
// =====================================================

#endif // NEU_WIFI_CONFIG