/**
 * @file _NeuHttpServer.h
 * @brief Lightweight HTTP and WebSocket server wrapper for ESP-IDF
 * @details Provides an easy-to-use API to create HTTP servers, define routes,
 *          handle requests, serve static content, manage errors, and work with
 *          WebSocket connections. Built on top of ESP-IDF's HTTP server library,
 *          designed for embedded applications using Espressif hardware.
 */

#pragma once

#include "esp_http_server.h"
#include <lwip/sockets.h>
#include <string.h>
#include <stdlib.h>

#include "NeuWiFiConfig.h"
#include "NeuWiFiTypes.h"

/**
 * @typedef NeuWSHandler
 * @brief Callback function type for handling WebSocket events
 * @param id Unique client connection ID
 * @param event Event type (connect, data, disconnect, etc.)
 * @param payload Pointer to received data buffer
 * @param len Length of received data in bytes
 */
typedef void (*NeuWSHandler)(int id, NeuWSEvent event, const char *payload, size_t len);

/**
 * @typedef NeuHandler
 * @brief Callback function type for handling standard HTTP requests
 * @param req Pointer to HTTP request structure
 */
typedef void (*NeuHandler)(httpd_req_t *req);

/**
 * @typedef NeuErrorHandler
 * @brief Callback function type for handling HTTP server errors
 * @param req Pointer to HTTP request structure
 * @param error Error code representing the issue
 */
typedef void (*NeuErrorHandler)(httpd_req_t *req, httpd_err_code_t error);

/**
 * @class NeuHttpServer
 * @brief HTTP and WebSocket server management class
 * @details Encapsulates ESP-IDF http_server functionality, simplifies route definition,
 *          provides helper methods for common responses, and offers WebSocket communication
 *          features including broadcasting and targeted messaging.
 */
class NeuHttpServer
{
public:
    /**
     * @brief Constructor: Initializes internal server configuration and state
     */
    NeuHttpServer();

    /**
     * @brief Start the HTTP server on the specified port
     * @param port TCP port number to listen on (default: 80)
     */
    void init(int port = 80);

    /**
     * @brief Alias for init()
     * @param port TCP port number to listen on (default: 80)
     * @see init()
     */
    inline void begin(int port = 80) { init(port); }

    /**
     * @brief Stop the server and close all active connections
     */
    void stop();

    /**
     * @brief Register a dynamic route with custom handler
     * @param uri URI path to match
     * @param method HTTP method (GET, POST, PUT, etc.)
     * @param handler Function to execute when route is accessed
     */
    void on(const char *uri, httpd_method_t method, NeuHandler handler);

    /**
     * @brief Register a static content route
     * @param uri URI path to match
     * @param content Null-terminated string to serve as response
     */
    void on(const char *uri, const char *content);

    /**
     * @brief Set custom handler for server error events
     * @param handler Function to call when errors occur
     */
    void onError(NeuErrorHandler handler);

    /**
     * @brief Register a WebSocket route
     * @param uri URI path for WebSocket connection
     * @param handler Callback to handle WebSocket events and data
     */
    void onWS(const char *uri, NeuWSHandler handler);

    /**
     * @brief Send plain text response to HTTP request
     * @param req Pointer to HTTP request
     * @param content Null-terminated text string to send
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t sendText(httpd_req_t *req, const char *content);

    /**
     * @brief Send JSON formatted response
     * @param req Pointer to HTTP request
     * @param json Null-terminated JSON string
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t sendJSON(httpd_req_t *req, const char *json);

    /**
     * @brief Send HTTP redirect response
     * @param req Pointer to HTTP request
     * @param location Target URL or path to redirect to
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t redirect(httpd_req_t *req, const char *location);

    /**
     * @brief Retrieve value of a parameter from POST form data
     * @param req Pointer to HTTP request
     * @param param Name of the parameter to extract
     * @param dest Buffer to store the extracted value
     * @param dest_len Maximum size of destination buffer
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t getPostParam(httpd_req_t *req, const char *param, char *dest, size_t dest_len);

    /**
     * @brief Send text message over active WebSocket connection
     * @param req Pointer to HTTP request containing WebSocket context
     * @param msg Null-terminated text message
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t wsSendText(httpd_req_t *req, const char *msg);

    /**
     * @brief Send binary data over active WebSocket connection
     * @param req Pointer to HTTP request containing WebSocket context
     * @param data Pointer to binary data buffer
     * @param len Length of data in bytes
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t wsSendBin(httpd_req_t *req, uint8_t *data, size_t len);

    /**
     * @brief Convert HTTP error code to human-readable string
     * @param error Error code from httpd_err_code_t
     * @return Constant string describing the error
     */
    static const char *errorToString(httpd_err_code_t error);

    /**
     * @brief Send text message to specific WebSocket client by ID
     * @param id Client connection ID
     * @param msg Null-terminated text message
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t wsSendText(int id, const char *msg);

    /**
     * @brief Send binary data to specific WebSocket client by ID
     * @param id Client connection ID
     * @param data Pointer to binary data buffer
     * @param len Length of data in bytes
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t wsSendBin(int id, const uint8_t *data, size_t len);

    /**
     * @brief Send PING frame to specific WebSocket client
     * @param id Client connection ID
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t wsSendPing(int id);

    /**
     * @brief Broadcast data to all connected WebSocket clients
     * @param data Pointer to data buffer
     * @param len Length of data in bytes
     * @param isBinary True for binary data, false for text data
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t wsBroadcast(const uint8_t *data, size_t len, bool isBinary = false);

    /**
     * @brief Broadcast text message to all connected WebSocket clients
     * @param msg Null-terminated text message
     * @return ESP_OK on success, error code otherwise
     */
    inline esp_err_t wsBroadcast(const char *msg)
    {
        if (!msg)
            return ESP_FAIL;
        return wsBroadcast((const uint8_t *)msg, strlen(msg), false);
    }

    /**
     * @brief Send data to client identified by file descriptor
     * @param fd Socket file descriptor of target client
     * @param data Pointer to data buffer
     * @param len Length of data in bytes
     * @param isBinary True for binary data, false for text data
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t wsSendToFd(int fd, const uint8_t *data, size_t len, bool isBinary = false);

    /**
     * @brief Disconnect all currently connected clients
     */
    void closeAllClients();

    /**
     * @brief Get list of file descriptors for all connected clients
     * @param fds Array to store file descriptors
     * @param max_fds Maximum number of entries the array can hold
     * @return Actual number of client file descriptors stored
     */
    size_t getClients(int *fds, size_t max_fds) const;

private:
    httpd_handle_t _server = nullptr; /**< Handle to underlying HTTP server instance */
    httpd_config_t _config;           /**< Server configuration parameters */

    static NeuErrorHandler _user_error_handler; /**< User-defined error callback */

    /**
     * @brief Internal wrapper for registered route handlers
     * @param req Pointer to HTTP request
     * @return ESP_OK or error code
     */
    static esp_err_t _static_handler(httpd_req_t *req);

    /**
     * @brief Internal handler for WebSocket events and data
     * @param req Pointer to HTTP request
     * @return ESP_OK or error code
     */
    static esp_err_t _ws_internal_wrapper(httpd_req_t *req);

    /**
     * @brief Main internal dispatcher for HTTP requests
     * @param req Pointer to HTTP request
     * @return ESP_OK or error code
     */
    static esp_err_t _internal_handler(httpd_req_t *req);

    /**
     * @brief Global internal error handler
     * @param req Pointer to HTTP request
     * @param err Error code
     * @return ESP_OK or error code
     */
    static esp_err_t _internal_error_handler(httpd_req_t *req, httpd_err_code_t err);

    /**
     * @brief Get client's IP address as 32-bit integer
     * @param req Pointer to HTTP request
     * @return IPv4 address in network byte order
     */
    static uint32_t getClientIP(httpd_req_t *req);

    int _port;        /**< TCP port number server listens on */
    bool _is_running; /**< Server running state flag */

    /**
     * @struct SavedRoute
     * @brief Stores information about a registered HTTP route
     */
    struct SavedRoute
    {
        const char *uri;            /**< URI path */
        httpd_method_t method;      /**< HTTP method */
        NeuHandler handler;         /**< Custom request handler */
        const char *static_content; /**< Static content to serve */
    };

    SavedRoute _saved_routes[NEU_MAX_HTTP_ROUTES]; /**< Array of registered HTTP routes */
    size_t _route_count = 0;                       /**< Number of registered routes */

    /**
     * @struct SavedWSRoute
     * @brief Stores information about a registered WebSocket route
     */
    struct SavedWSRoute
    {
        const char *uri;      /**< URI path */
        NeuWSHandler handler; /**< WebSocket event handler */
    };

    SavedWSRoute _saved_ws_routes[NEU_MAX_WS_ROUTES]; /**< Array of registered WebSocket routes */
    size_t _ws_route_count = 0;                       /**< Number of registered WebSocket routes */

    /**
     * @struct ws_broadcast_ctx_t
     * @brief Context data for broadcast worker task
     */
    struct ws_broadcast_ctx_t
    {
        NeuHttpServer *server_ptr; /**< Pointer to server instance */
        uint8_t *data;             /**< Data to broadcast */
        size_t len;                /**< Length of data */
        bool is_binary;            /**< Data type flag */
    };

    /**
     * @brief Worker function for asynchronous broadcast operation
     * @param arg Context pointer (ws_broadcast_ctx_t)
     */
    static void _ws_broadcast_worker(void *arg);

    /**
     * @brief Main server task function
     * @param pvParameters Task parameter (instance pointer)
     */
    static void _serverTask(void *pvParameters);
};