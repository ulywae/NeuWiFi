/**
 * @file httpServer.h
 * @author Ulywae (@Neufa)
 * @brief Lightweight HTTP and WebSocket server for ESP32.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * NeuHttpServer is a lightweight abstraction layer built on top of
 * ESP-IDF HTTP Server.
 *
 * Features:
 * - Lightweight HTTP routing
 * - WebSocket support
 * - Static response helpers
 * - JSON response helpers
 * - Redirect utilities
 * - POST parameter parser
 * - Client management
 * - Broadcast messaging
 * - Error handling system
 *
 * Design goals:
 * - Beginner-friendly API
 * - Minimal runtime overhead
 * - Deterministic request handling
 * - Embedded-safe memory usage
 * - Easy WebSocket integration
 *
 * Intended for:
 * - ESP32
 * - ESP-IDF
 * - Embedded dashboards
 * - IoT control panels
 * - Realtime device communication
 */

#pragma once

#include "esp_http_server.h"
#include <string.h>
#include <stdlib.h>

#include "../../NeuWiFiConfig.h"

/**
 * @defgroup NeuWS_Event WebSocket Event Types
 * @brief Beginner-friendly WebSocket event abstraction.
 * @{
 */

/**
 * @brief WebSocket event identifiers.
 */
typedef enum
{
    /** @brief Client connected. */
    WS_EVENT_CONNECT,

    /** @brief Client disconnected. */
    WS_EVENT_DISCONNECT,

    /** @brief Text message received. */
    WS_EVENT_TEXT,

    /** @brief Binary packet received. */
    WS_EVENT_BINARY,

    /** @brief Ping frame received. */
    WS_EVENT_PING,

    /** @brief Pong frame received. */
    WS_EVENT_PONG,

    /** @brief WebSocket internal error. */
    WS_EVENT_ERROR

} NeuWSEvent;

/** @} */

// =====================================================
// Callback Types
// =====================================================

/**
 * @brief WebSocket callback signature.
 *
 * @param id Client socket identifier.
 * @param event WebSocket event type.
 * @param payload Incoming payload buffer.
 * @param len Payload size.
 */
typedef void (*NeuWSHandler)(int id,
                             NeuWSEvent event,
                             const char *payload,
                             size_t len);

/**
 * @brief HTTP route callback handler.
 *
 * @details
 * Beginner-friendly wrapper around ESP-IDF request pointer.
 *
 * @param req HTTP request object.
 */
typedef void (*NeuHandler)(httpd_req_t *req);

/**
 * @brief HTTP error callback handler.
 *
 * @param req HTTP request object.
 * @param error HTTP error code.
 */
typedef void (*NeuErrorHandler)(httpd_req_t *req,
                                httpd_err_code_t error);

/**
 * @class NeuHttpServer
 * @brief Lightweight HTTP/WebSocket server wrapper for ESP32.
 *
 * @details
 * NeuHttpServer simplifies ESP-IDF HTTP server usage by providing:
 *
 * - Simple route registration
 * - Built-in WebSocket support
 * - Easy response helpers
 * - Client management tools
 * - Unified error handling
 *
 * The architecture is designed for:
 * - Embedded systems
 * - Low memory usage
 * - Deterministic behavior
 * - Beginner-friendly APIs
 */
class NeuHttpServer
{
public:
    // =====================================================
    // Constructor
    // =====================================================

    /**
     * @brief Construct a new NeuHttpServer object.
     */
    NeuHttpServer();

    // =====================================================
    // Server Lifecycle
    // =====================================================

    /**
     * @brief Start HTTP server.
     *
     * @param port Server listening port.
     */
    void init(int port = 80);

    /**
     * @brief Arduino-style alias for init().
     *
     * @param port Server listening port.
     */
    inline void begin(int port = 80)
    {
        init(port);
    }

    /**
     * @brief Stop HTTP server.
     */
    void stop();

    // =====================================================
    // HTTP Routing
    // =====================================================

    /**
     * @brief Register HTTP route handler.
     *
     * @param uri Route URI.
     * @param method HTTP method.
     * @param handler Route callback handler.
     */
    void on(const char *uri,
            httpd_method_t method,
            NeuHandler handler);

    /**
     * @brief Register static content route.
     *
     * @details
     * Useful for:
     * - Simple HTML pages
     * - Static JSON
     * - Lightweight dashboards
     *
     * @param uri Route URI.
     * @param content Static response content.
     */
    void on(const char *uri,
            const char *content);

    // =====================================================
    // Error Handling
    // =====================================================

    /**
     * @brief Register custom HTTP error handler.
     *
     * @param handler User error callback.
     */
    void onError(NeuErrorHandler handler);

    // =====================================================
    // WebSocket Registration
    // =====================================================

    /**
     * @brief Register WebSocket endpoint.
     *
     * @param uri WebSocket URI.
     * @param handler WebSocket callback.
     */
    void onWS(const char *uri,
              NeuWSHandler handler);

    // =====================================================
    // Static HTTP Helpers
    // =====================================================

    /**
     * @brief Send plain text response.
     *
     * @param req HTTP request object.
     * @param content Text content.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t sendText(httpd_req_t *req,
                              const char *content);

    /**
     * @brief Send JSON response.
     *
     * @param req HTTP request object.
     * @param json JSON string.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t sendJSON(httpd_req_t *req,
                              const char *json);

    /**
     * @brief Send HTTP redirect response.
     *
     * @param req HTTP request object.
     * @param location Redirect destination.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t redirect(httpd_req_t *req,
                              const char *location);

    /**
     * @brief Retrieve POST parameter value.
     *
     * @details
     * Extracts POST field value from request body.
     *
     * @param req HTTP request object.
     * @param param Parameter name.
     * @param dest Destination buffer.
     * @param dest_len Destination buffer size.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t getPostParam(httpd_req_t *req,
                                  const char *param,
                                  char *dest,
                                  size_t dest_len);

    /**
     * @brief Send WebSocket text frame.
     *
     * @param req HTTP request object.
     * @param msg Text message.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t wsSendText(httpd_req_t *req,
                                const char *msg);

    /**
     * @brief Send WebSocket binary frame.
     *
     * @param req HTTP request object.
     * @param data Binary payload.
     * @param len Payload size.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t wsSendBin(httpd_req_t *req,
                               uint8_t *data,
                               size_t len);

    /**
     * @brief Convert HTTP error code to readable string.
     *
     * @param error HTTP error code.
     *
     * @return const char* Error description.
     */
    static const char *errorToString(httpd_err_code_t error);

    // =====================================================
    // WebSocket Instance Helpers
    // =====================================================

    /**
     * @brief Send WebSocket text message to client.
     *
     * @param id Client socket ID.
     * @param msg Text message.
     *
     * @return esp_err_t ESP-IDF status.
     */
    esp_err_t wsSendText(int id,
                         const char *msg);

    /**
     * @brief Send WebSocket binary packet to client.
     *
     * @param id Client socket ID.
     * @param data Binary payload.
     * @param len Payload size.
     *
     * @return esp_err_t ESP-IDF status.
     */
    esp_err_t wsSendBin(int id,
                        const uint8_t *data,
                        size_t len);

    /**
     * @brief Send WebSocket ping frame.
     *
     * @param id Client socket ID.
     *
     * @return esp_err_t ESP-IDF status.
     */
    esp_err_t wsSendPing(int id);

    /**
     * @brief Broadcast WebSocket message to all clients.
     *
     * @param msg Payload message.
     * @param isBinary True for binary frame.
     *
     * @return esp_err_t ESP-IDF status.
     */
    esp_err_t wsBroadcast(const char *msg,
                          bool isBinary = false);

    /**
     * @brief Send WebSocket data directly using socket FD.
     *
     * @param fd Socket file descriptor.
     * @param data Payload data.
     * @param len Payload size.
     * @param isBinary True for binary frame.
     *
     * @return esp_err_t ESP-IDF status.
     */
    esp_err_t wsSendToFd(int fd,
                         const uint8_t *data,
                         size_t len,
                         bool isBinary = false);

    // =====================================================
    // Client Management
    // =====================================================

    /**
     * @brief Disconnect all connected clients.
     */
    void closeAllClients();

    /**
     * @brief Retrieve connected client socket IDs.
     *
     * @param fds Destination buffer.
     * @param max_fds Maximum socket count.
     *
     * @return size_t Number of copied clients.
     */
    size_t getClients(int *fds,
                      size_t max_fds) const;

private:
    // =====================================================
    // Internal Runtime
    // =====================================================

    /** @brief Internal HTTP server handle. */
    httpd_handle_t _server = nullptr;

    /** @brief Internal HTTP server configuration. */
    httpd_config_t _config;

    /** @brief User-defined HTTP error handler. */
    static NeuErrorHandler _user_error_handler;

    // =====================================================
    // Internal Wrappers
    // =====================================================

    /**
     * @brief Static request dispatcher wrapper.
     *
     * @param req HTTP request object.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t _static_handler(httpd_req_t *req);

    /**
     * @brief Internal WebSocket dispatcher wrapper.
     *
     * @param req HTTP request object.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t _ws_internal_wrapper(httpd_req_t *req);

    /**
     * @brief Internal route execution handler.
     *
     * @param req HTTP request object.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t _internal_handler(httpd_req_t *req);

    /**
     * @brief Internal HTTP error bridge.
     *
     * @param req HTTP request object.
     * @param err HTTP error code.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t _internal_error_handler(httpd_req_t *req,
                                             httpd_err_code_t err);
};