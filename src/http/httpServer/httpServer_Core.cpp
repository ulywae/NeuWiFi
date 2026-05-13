/**
 * @file httpServer_Core.cpp
 * @author Ulywae
 * @brief Core runtime implementation for NeuHttpServer.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * This file contains:
 * - HTTP server initialization
 * - Route registration
 * - Internal request dispatching
 * - Static content serving
 * - Client session management
 * - Runtime shutdown handling
 *
 * Design goals:
 * - Lightweight runtime overhead
 * - Deterministic request handling
 * - Safe embedded memory usage
 * - Beginner-friendly routing
 * - Clean ESP-IDF abstraction
 */

#include "httpServer.h"

/**
 * @brief Construct a new NeuHttpServer object.
 *
 * @details
 * Initializes internal HTTP server configuration
 * using ESP-IDF default settings.
 *
 * Runtime configuration:
 * - Increased stack size
 * - Dynamic route capacity
 * - LRU session purge enabled
 *
 * The constructor automatically attempts
 * to start the server immediately.
 */
NeuHttpServer::NeuHttpServer()
{
    // =====================================================
    // Load Default HTTP Configuration
    // =====================================================

    _config = HTTPD_DEFAULT_CONFIG();

    /**
     * Increase worker stack size for:
     * - JSON handling
     * - WebSocket processing
     * - dynamic routes
     */
    _config.stack_size = 8192;

    /**
     * Total route capacity:
     * HTTP + WebSocket routes.
     */
    _config.max_uri_handlers =
        NEU_MAX_HTTP_ROUTES +
        NEU_MAX_WS_ROUTES;

    /**
     * Enable automatic least-recently-used
     * session cleanup.
     */
    _config.lru_purge_enable = true;

    // =====================================================
    // Start HTTP Server
    // =====================================================

    if (httpd_start(&_server,
                    &_config) != ESP_OK)
        _server = nullptr;
}

/**
 * @brief Initialize HTTP server.
 *
 * @details
 * Starts the HTTP server on the selected port.
 *
 * Safety features:
 * - Duplicate initialization protection
 *
 * @param port HTTP listening port.
 */
void NeuHttpServer::init(int port)
{
    /**
     * Prevent duplicate server startup.
     */
    if (_server)
        return;

    _config.server_port = port;

    httpd_start(&_server,
                &_config);
}

/**
 * @brief Stop HTTP server safely.
 *
 * @details
 * Stops all active sessions and releases
 * internal HTTP server resources.
 */
void NeuHttpServer::stop()
{
    if (_server)
    {
        httpd_stop(_server);

        _server = nullptr;
    }
}

// =====================================================
// HTTP Route Registration
// =====================================================

/**
 * @brief Register static GET route.
 *
 * @details
 * Creates lightweight static content endpoint.
 *
 * Useful for:
 * - Simple HTML pages
 * - JSON endpoints
 * - Diagnostics pages
 * - Embedded dashboards
 *
 * @param uri Route URI.
 * @param content Static response content.
 */
void NeuHttpServer::on(const char *uri,
                       const char *content)
{
    if (!_server || !content)
        return;

    /**
     * Configure static route handler.
     */
    httpd_uri_t route =
        {
            .uri = uri,
            .method = HTTP_GET,
            .handler = _static_handler,
            .user_ctx = (void *)content};

    httpd_register_uri_handler(_server,
                               &route);
}

/**
 * @brief Internal static content dispatcher.
 *
 * @details
 * Sends static text response stored
 * inside route user context.
 *
 * @param req HTTP request object.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::_static_handler(httpd_req_t *req)
{
    return NeuHttpServer::sendText(
        req,
        (const char *)req->user_ctx);
}

/**
 * @brief Register dynamic HTTP route.
 *
 * @details
 * Registers user-defined request callback
 * using internal middleware dispatcher.
 *
 * Architecture:
 * User Handler
 * ↓
 * Internal Wrapper
 * ↓
 * ESP-IDF HTTP Server
 *
 * @param uri Route URI.
 * @param method HTTP method.
 * @param handler User callback.
 */
void NeuHttpServer::on(const char *uri,
                       httpd_method_t method,
                       NeuHandler handler)
{
    if (!_server)
        return;

    /**
     * Configure route middleware.
     */
    httpd_uri_t route =
        {
            .uri = uri,
            .method = method,

            /**
             * Internal dispatcher wrapper.
             */
            .handler = _internal_handler,

            /**
             * Store user callback safely.
             */
            .user_ctx = (void *)handler};

    httpd_register_uri_handler(_server,
                               &route);
}

/**
 * @brief Internal route execution bridge.
 *
 * @details
 * Extracts user callback from:
 * - request user_ctx
 *
 * then safely executes it.
 *
 * This wrapper allows beginner-friendly:
 * void handler(req)
 *
 * API design without exposing ESP-IDF
 * return management complexity.
 *
 * @param req HTTP request object.
 *
 * @return esp_err_t Always returns ESP_OK.
 */
esp_err_t NeuHttpServer::_internal_handler(httpd_req_t *req)
{
    /**
     * Recover user callback.
     */
    NeuHandler user_func =
        (NeuHandler)req->user_ctx;

    /**
     * Execute user route safely.
     */
    if (user_func)
        user_func(req);

    /**
     * Internal framework handles return state.
     */
    return ESP_OK;
}

// =====================================================
// Client Management
// =====================================================

/**
 * @brief Disconnect all connected clients.
 *
 * @details
 * Iterates through active HTTP sessions
 * and requests graceful socket closure.
 *
 * Features:
 * - Dynamic client enumeration
 * - Runtime-safe cleanup
 * - Allocation failure protection
 */
void NeuHttpServer::closeAllClients()
{
    if (!_server)
        return;

    // =====================================================
    // Allocate Client FD Buffer
    // =====================================================

    size_t max =
        _config.max_open_sockets;

    int *fds =
        (int *)malloc(sizeof(int) * max);

    /**
     * Allocation failure protection.
     */
    if (!fds)
        return;

    // =====================================================
    // Retrieve Active Clients
    // =====================================================

    size_t count = max;

    if (httpd_get_client_list(_server,
                              &count,
                              fds) == ESP_OK)
    {
        for (size_t i = 0; i < count; i++)
        {
            /**
             * Trigger graceful session closure.
             */
            httpd_sess_trigger_close(
                _server,
                fds[i]);
        }
    }

    free(fds);
}

/**
 * @brief Retrieve active client socket IDs.
 *
 * @details
 * Copies active client file descriptors
 * into user-provided buffer.
 *
 * Useful for:
 * - WebSocket broadcasting
 * - Diagnostics
 * - Session monitoring
 * - Manual socket management
 *
 * @param fds Destination FD buffer.
 * @param max_fds Maximum buffer size.
 *
 * @return size_t Number of copied clients.
 */
size_t NeuHttpServer::getClients(int *fds,
                                 size_t max_fds) const
{
    if (!_server)
        return 0;

    size_t count = max_fds;

    if (httpd_get_client_list(_server,
                              &count,
                              fds) == ESP_OK)
    {
        return count;
    }

    return 0;
}