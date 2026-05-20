/**
 * @file NeuHttpServer_Core.cpp
 * @brief Core implementation of the Neu HTTP Server subsystem.
 *
 * Manages server lifecycle, configuration, routing, request handling,
 * and integration with system state. Features automatic start/stop based
 * on network status, static and dynamic route registration, WebSocket support,
 * and client tracking integration with SharedState.
 */

#include "http/_NeuHttpServer.h"
#include "SystemState/SharedState.h"

#include <sys/socket.h>
#include <netinet/in.h>

/**
 * @brief Global singleton instance pointer used for task access.
 */
static NeuHttpServer *_global_server_instance = nullptr;

/**
 * @brief Constructor for NeuHttpServer.
 *
 * Initializes server configuration with optimized defaults:
 * - Stack size: 8192 bytes
 * - Maximum routes: Defined by NEU_MAX_HTTP_ROUTES + NEU_MAX_WS_ROUTES + 12 (for error handlers)
 * - LRU purge enabled for memory efficiency
 * - Pinned to CPU core 1
 *
 * Sets initial state values and stores this instance as the global reference.
 */
NeuHttpServer::NeuHttpServer()
{
    _config = HTTPD_DEFAULT_CONFIG();

    _config.stack_size = 8192;
    _config.max_uri_handlers = NEU_MAX_HTTP_ROUTES + NEU_MAX_WS_ROUTES + 12;
    _config.lru_purge_enable = true;
    _config.core_id = 1;

    _server = nullptr;
    _is_running = false;
    _port = 80;
    _route_count = 0;
    _ws_route_count = 0;

    _global_server_instance = this;
}

/**
 * @brief Passive initialize the HTTP server and start its management task.
 * The server will automatically start/stop based on network events (STA/AP ready/down).
 *
 * @param port TCP port number to listen on (default: 80)
 */
void NeuHttpServer::init(int port)
{
    _port = port;
    _config.server_port = port;

    if (_is_running)
        return;
    _is_running = true;

    // ─────────────────────────────────────────────────────────
    // EVENT BRIDGE LAYER: Server On-Off Controlled by Network Events
    // ─────────────────────────────────────────────────────────
    SharedState::getInstance().onEvent([](SharedState::Event e)
                                       {
        if (_global_server_instance == nullptr) return;

        switch (e)
        {
            case SharedState::Event::STA_READY:
            case SharedState::Event::AP_READY:
            case SharedState::Event::MODE_CHANGED:
                // If the network is ready/changing mode, make sure the old server is off first, then restart it.
                if (_global_server_instance->_server != nullptr) {
                    httpd_stop(_global_server_instance->_server);
                    _global_server_instance->_server = nullptr;
                }
                
                if (httpd_start(&_global_server_instance->_server, &_global_server_instance->_config) == ESP_OK)
                {
                    // 1. Register all error handler traps (Including 404 hijackers)
                    httpd_err_code_t err_codes[] = {
                        HTTPD_500_INTERNAL_SERVER_ERROR, HTTPD_501_METHOD_NOT_IMPLEMENTED,
                        HTTPD_505_VERSION_NOT_SUPPORTED, HTTPD_400_BAD_REQUEST,
                        HTTPD_401_UNAUTHORIZED,          HTTPD_403_FORBIDDEN,
                        HTTPD_404_NOT_FOUND,            HTTPD_405_METHOD_NOT_ALLOWED,
                        HTTPD_408_REQ_TIMEOUT,           HTTPD_411_LENGTH_REQUIRED,
                        HTTPD_414_URI_TOO_LONG,          HTTPD_431_REQ_HDR_FIELDS_TOO_LARGE
                    };

                    for (size_t i = 0; i < sizeof(err_codes) / sizeof(err_codes[0]); i++) {
                        httpd_register_err_handler(_global_server_instance->_server, err_codes[i], &NeuHttpServer::_internal_error_handler);
                    }

                    // 2. Register all user-defined static/dynamic page routes that have been reserved
                    for (size_t i = 0; i < _global_server_instance->_route_count; i++) {
                        httpd_uri_t route = {
                            .uri = _global_server_instance->_saved_routes[i].uri,
                            .method = _global_server_instance->_saved_routes[i].method,
                            .handler = (_global_server_instance->_saved_routes[i].static_content != nullptr) ? _static_handler : _internal_handler,
                            .user_ctx = (_global_server_instance->_saved_routes[i].static_content != nullptr) ? (void *)_global_server_instance->_saved_routes[i].static_content : (void *)_global_server_instance->_saved_routes[i].handler
                        };
                        httpd_register_uri_handler(_global_server_instance->_server, &route);
                    }

                    // 3. Register all WebSocket routes
                    for (size_t i = 0; i < _global_server_instance->_ws_route_count; i++) {
                        httpd_uri_t ws_route = {
                            .uri = _global_server_instance->_saved_ws_routes[i].uri,
                            .method = HTTP_GET,
                            .handler = _ws_internal_wrapper, // Wrapper internal handler will call the user-defined WS handler stored in user_ctx
                            .user_ctx = (void *)_global_server_instance->_saved_ws_routes[i].handler,
                            .is_websocket = true,
                            .handle_ws_control_frames = true
                        };
                        httpd_register_uri_handler(_global_server_instance->_server, &ws_route);
                    }
                }
                break;

            case SharedState::Event::STA_DOWN:
            case SharedState::Event::AP_DOWN:
                // If one of the networks is down and there are no routes left, turn off the server
                if (!SharedState::getInstance().isNetworkReady() && _global_server_instance->_server != nullptr) {
                    httpd_stop(_global_server_instance->_server);
                    _global_server_instance->_server = nullptr;
                }
                // If there are still routes remaining in Hybrid mode, the MODE_CHANGED event above will regenerate them
                break;
                
            default:
                break;
        } });
}

/**
 * @brief Stop the HTTP server and reset internal state.
 *
 * Stops the server if running, frees resources, and resets route counters.
 * Does not remove registered routes — they will be re-applied when server restarts.
 */
void NeuHttpServer::stop()
{
    _is_running = false;
    if (_server)
    {
        httpd_stop(_server);
        _server = nullptr;
    }
    _route_count = 0;
    _ws_route_count = 0;
}

/**
 * @brief Register a static content route (HTTP GET only).
 *
 * When accessed, this URI returns fixed text/HTML content directly.
 *
 * @param uri URI path to register (e.g., "/about")
 * @param content Null-terminated string to return as response body
 */
void NeuHttpServer::on(const char *uri, const char *content)
{
    if (!content || !uri)
        return;
    if (_route_count < NEU_MAX_HTTP_ROUTES)
    {
        _saved_routes[_route_count] = {
            .uri = uri,
            .method = HTTP_GET,
            .handler = nullptr,
            .static_content = content};
        _route_count++;
    }
}

/**
 * @brief Register a dynamic route with custom handler function.
 *
 * Supports all HTTP methods. User-provided function is executed on request.
 *
 * @param uri URI path to register
 * @param method HTTP method (GET, POST, PUT, DELETE, etc.)
 * @param handler Function to execute when this URI is requested
 */
void NeuHttpServer::on(const char *uri, httpd_method_t method, NeuHandler handler)
{
    if (!handler || !uri)
        return;
    if (_route_count < NEU_MAX_HTTP_ROUTES)
    {
        _saved_routes[_route_count] = {
            .uri = uri,
            .method = method,
            .handler = handler,
            .static_content = nullptr};
        _route_count++;
    }
}

// =====================================================
// INTERNAL UTILITY: Accurate Client IP Extraction
// =====================================================

/**
 * @brief Retrieve raw IPv4 address of connected client from socket.
 *
 * Uses getpeername() on the underlying socket descriptor to get accurate
 * client IP address in network byte order (same format as DHCP/ESP-NETIF).
 *
 * @param req Pointer to HTTP request structure
 * @return uint32_t Client IPv4 address as uint32_t, 0 on failure or invalid request
 */
uint32_t NeuHttpServer::getClientIP(httpd_req_t *req)
{
    if (!req)
        return 0;
    int sockfd = httpd_req_to_sockfd(req);
    if (sockfd < 0)
        return 0;

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (getpeername(sockfd, (struct sockaddr *)&addr, &addr_len) == 0)
    {
        return addr.sin_addr.s_addr;
    }
    return 0;
}

// =====================================================
// AUTOMATED WRAPPER: Static Content Route Handler
// =====================================================

/**
 * @brief Internal handler for static content routes.
 *
 * Sends predefined text content. Also performs client tracking update:
 * when a client successfully loads a page, it is removed from DNS redirection
 * tracking in SharedState.
 *
 * @param req Pointer to HTTP request structure
 * @return esp_err_t ESP_OK always
 */
esp_err_t NeuHttpServer::_static_handler(httpd_req_t *req)
{
    uint32_t client_ip = NeuHttpServer::getClientIP(req);
    if (client_ip != 0)
    {
        SharedState::getInstance().clearClientFromTracker(client_ip);
    }
    return NeuHttpServer::sendText(req, (const char *)req->user_ctx);
}

// =====================================================
// AUTOMATED WRAPPER: Custom User Route Handler
// =====================================================

/**
 * @brief Internal wrapper for user-defined dynamic route handlers.
 *
 * Executes the user-provided function. Also updates client tracking:
 * successful request clears client from DNS redirection list.
 *
 * @param req Pointer to HTTP request structure
 * @return esp_err_t ESP_OK always
 */
esp_err_t NeuHttpServer::_internal_handler(httpd_req_t *req)
{
    // Identify client making custom request
    uint32_t client_ip = NeuHttpServer::getClientIP(req);
    if (client_ip != 0)
    {
        // Immediately clear client from tracking system as they have successfully made a request
        SharedState::getInstance().clearClientFromTracker(client_ip);
    }

    NeuHandler user_func = (NeuHandler)req->user_ctx;
    if (user_func)
    {
        user_func(req);
    }
    return ESP_OK;
}

/**
 * @brief Force-close all currently connected client sessions.
 *
 * Retrieves list of active client sockets and triggers immediate closure.
 * Useful during reconfiguration, shutdown, or network mode changes.
 */
void NeuHttpServer::closeAllClients()
{
    if (!_server)
        return;
    size_t max = _config.max_open_sockets;
    int *fds = (int *)malloc(sizeof(int) * max);
    if (!fds)
        return;

    size_t count = max;
    if (httpd_get_client_list(_server, &count, fds) == ESP_OK)
    {
        for (size_t i = 0; i < count; i++)
        {
            httpd_sess_trigger_close(_server, fds[i]);
        }
    }
    free(fds);
}

/**
 * @brief Get list of file descriptors for currently connected clients.
 *
 * Fills provided buffer with active socket IDs.
 *
 * @param fds Pointer to integer buffer to store socket descriptors
 * @param max_fds Maximum number of entries buffer can hold
 * @return size_t Number of valid entries written to buffer, 0 if server not running
 */
size_t NeuHttpServer::getClients(int *fds, size_t max_fds) const
{
    if (!_server)
        return 0;
    size_t count = max_fds;
    if (httpd_get_client_list(_server, &count, fds) == ESP_OK)
        return count;
    return 0;
}
