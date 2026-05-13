/**
 * @file httpServer_WS.cpp
 * @author Ulywae
 * @brief WebSocket runtime implementation for NeuHttpServer.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * This file contains:
 * - WebSocket route registration
 * - Frame parsing
 * - WebSocket event dispatching
 * - Async socket transmission
 * - Broadcast system
 * - Ping/Pong handling
 * - Client connection management
 *
 * Design goals:
 * - Lightweight runtime overhead
 * - Deterministic packet handling
 * - Embedded-safe memory usage
 * - Beginner-friendly WebSocket API
 * - Fully asynchronous transmission support
 */

#include "httpServer.h"

// =====================================================
// WebSocket Registration
// =====================================================

/**
 * @brief Register WebSocket endpoint.
 *
 * @details
 * Creates WebSocket-capable URI route and
 * attaches internal dispatcher wrapper.
 *
 * Features:
 * - Automatic control frame handling
 * - Beginner-friendly callback abstraction
 * - Async transmission compatible
 *
 * @param uri WebSocket endpoint URI.
 * @param handler User WebSocket callback.
 */
void NeuHttpServer::onWS(const char *uri,
                         NeuWSHandler handler)
{
    if (!_server || !handler)
        return;

    /**
     * Configure WebSocket route.
     */
    httpd_uri_t ws_route =
        {
            .uri = uri,
            .method = HTTP_GET,

            /**
             * Internal middleware dispatcher.
             */
            .handler = _ws_internal_wrapper,

            /**
             * Store user callback.
             */
            .user_ctx = (void *)handler,

            /**
             * Enable WebSocket mode.
             */
            .is_websocket = true,

            /**
             * Allow automatic control frame handling.
             */
            .handle_ws_control_frames = true};

    httpd_register_uri_handler(_server,
                               &ws_route);
}

// =====================================================
// Internal WebSocket Dispatcher
// =====================================================

/**
 * @brief Internal WebSocket middleware wrapper.
 *
 * @details
 * Handles:
 * - Connection events
 * - Incoming frames
 * - Control frames
 * - Disconnect events
 * - Payload dispatching
 *
 * Event flow:
 * ESP-IDF
 * ↓
 * Internal Wrapper
 * ↓
 * Beginner-friendly callback API
 *
 * @param req HTTP/WebSocket request object.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::_ws_internal_wrapper(httpd_req_t *req)
{
    // =====================================================
    // Retrieve Client Socket ID
    // =====================================================

    int fd =
        httpd_req_to_sockfd(req);

    /**
     * Recover user callback.
     */
    NeuWSHandler user_func =
        (NeuWSHandler)req->user_ctx;

    // =====================================================
    // Connection Event
    // =====================================================

    /**
     * HTTP_GET means:
     * WebSocket handshake completed.
     */
    if (req->method == HTTP_GET)
    {
        if (user_func)
        {
            user_func(fd,
                      WS_EVENT_CONNECT,
                      NULL,
                      0);
        }

        return ESP_OK;
    }

    // =====================================================
    // Receive Incoming Frame
    // =====================================================

    uint8_t buf[NEU_SERVER_WS_BUF_SIZE];

    httpd_ws_frame_t ws_pkt;

    memset(&ws_pkt,
           0,
           sizeof(httpd_ws_frame_t));

    ws_pkt.payload = buf;

    esp_err_t ret =
        httpd_ws_recv_frame(
            req,
            &ws_pkt,
            NEU_SERVER_WS_BUF_SIZE - 1);

    // =====================================================
    // Receive Error Handling
    // =====================================================

    if (ret != ESP_OK)
    {
        if (user_func)
        {
            user_func(fd,
                      WS_EVENT_DISCONNECT,
                      NULL,
                      0);
        }

        return ret;
    }

    // =====================================================
    // Dispatch Packet Events
    // =====================================================

    if (user_func)
    {
        switch (ws_pkt.type)
        {
            // -------------------------------------------------
            // TEXT FRAME
            // -------------------------------------------------

        case HTTPD_WS_TYPE_TEXT:

            /**
             * Null terminate text payload
             * for safe string handling.
             */
            buf[ws_pkt.len] = '\0';

            user_func(fd,
                      WS_EVENT_TEXT,
                      (const char *)buf,
                      ws_pkt.len);

            break;

            // -------------------------------------------------
            // BINARY FRAME
            // -------------------------------------------------

        case HTTPD_WS_TYPE_BINARY:

            user_func(fd,
                      WS_EVENT_BINARY,
                      (const char *)buf,
                      ws_pkt.len);

            break;

            // -------------------------------------------------
            // PING FRAME
            // -------------------------------------------------

        case HTTPD_WS_TYPE_PING:
        {
            /**
             * Auto-respond with PONG.
             */
            httpd_ws_frame_t pong_pkt =
                {
                    .type = HTTPD_WS_TYPE_PONG};

            httpd_ws_send_frame(req,
                                &pong_pkt);

            user_func(fd,
                      WS_EVENT_PING,
                      NULL,
                      0);

            break;
        }

            // -------------------------------------------------
            // PONG FRAME
            // -------------------------------------------------

        case HTTPD_WS_TYPE_PONG:

            user_func(fd,
                      WS_EVENT_PONG,
                      (const char *)buf,
                      ws_pkt.len);

            break;

            // -------------------------------------------------
            // CLOSE FRAME
            // -------------------------------------------------

        case HTTPD_WS_TYPE_CLOSE:

            user_func(fd,
                      WS_EVENT_DISCONNECT,
                      NULL,
                      0);

            break;

        default:
            break;
        }
    }

    return ESP_OK;
}

// =====================================================
// Static WebSocket Helpers
// =====================================================

/**
 * @brief Send WebSocket text frame.
 *
 * @param req WebSocket request object.
 * @param msg Text message.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::wsSendText(httpd_req_t *req,
                                    const char *msg)
{
    httpd_ws_frame_t ws_pkt;

    memset(&ws_pkt,
           0,
           sizeof(httpd_ws_frame_t));

    ws_pkt.payload =
        (uint8_t *)msg;

    ws_pkt.len =
        strlen(msg);

    ws_pkt.type =
        HTTPD_WS_TYPE_TEXT;

    return httpd_ws_send_frame(req,
                               &ws_pkt);
}

/**
 * @brief Send WebSocket binary frame.
 *
 * @param req WebSocket request object.
 * @param data Binary payload.
 * @param len Payload length.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::wsSendBin(httpd_req_t *req,
                                   uint8_t *data,
                                   size_t len)
{
    httpd_ws_frame_t ws_pkt;

    memset(&ws_pkt,
           0,
           sizeof(httpd_ws_frame_t));

    ws_pkt.payload = data;

    ws_pkt.len = len;

    ws_pkt.type =
        HTTPD_WS_TYPE_BINARY;

    return httpd_ws_send_frame(req,
                               &ws_pkt);
}

// =====================================================
// Instance WebSocket Helpers
// =====================================================

/**
 * @brief Send text frame using client socket ID.
 *
 * @param id Client socket ID.
 * @param msg Text message.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::wsSendText(int id,
                                    const char *msg)
{
    return wsSendToFd(id,
                      (const uint8_t *)msg,
                      strlen(msg),
                      false);
}

/**
 * @brief Send binary frame using client socket ID.
 *
 * @param id Client socket ID.
 * @param data Binary payload.
 * @param len Payload length.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::wsSendBin(int id,
                                   const uint8_t *data,
                                   size_t len)
{
    return wsSendToFd(id,
                      data,
                      len,
                      true);
}

/**
 * @brief Send WebSocket frame asynchronously.
 *
 * @details
 * Uses ESP-IDF async WebSocket transmission.
 *
 * Features:
 * - Non-blocking send
 * - Binary/text support
 * - Lightweight packet construction
 *
 * @param fd Socket file descriptor.
 * @param data Payload buffer.
 * @param len Payload length.
 * @param isBinary True for binary frame.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::wsSendToFd(int fd,
                                    const uint8_t *data,
                                    size_t len,
                                    bool isBinary)
{
    if (!_server)
        return ESP_FAIL;

    httpd_ws_frame_t ws_pkt;

    /**
     * Clear packet memory.
     */
    memset(&ws_pkt,
           0,
           sizeof(httpd_ws_frame_t));

    ws_pkt.type =
        isBinary
            ? HTTPD_WS_TYPE_BINARY
            : HTTPD_WS_TYPE_TEXT;

    ws_pkt.payload =
        (uint8_t *)data;

    ws_pkt.len = len;

    return httpd_ws_send_frame_async(
        _server,
        fd,
        &ws_pkt);
}

/**
 * @brief Send WebSocket ping frame.
 *
 * @param fd Socket file descriptor.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::wsSendPing(int fd)
{
    if (!_server)
        return ESP_FAIL;

    httpd_ws_frame_t ping_pkt =
        {
            .type = HTTPD_WS_TYPE_PING};

    return httpd_ws_send_frame_async(
        _server,
        fd,
        &ping_pkt);
}

/**
 * @brief Broadcast WebSocket message to all clients.
 *
 * @details
 * Iterates through active WebSocket sessions
 * and sends packet asynchronously.
 *
 * Features:
 * - Broadcast text/binary support
 * - Automatic WS client filtering
 * - Runtime-safe allocation
 *
 * @param msg Payload message.
 * @param isBinary True for binary broadcast.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::wsBroadcast(const char *msg,
                                     bool isBinary)
{
    if (!_server)
        return ESP_FAIL;

    // =====================================================
    // Allocate Client FD Buffer
    // =====================================================

    size_t max =
        _config.max_open_sockets;

    int *fds =
        (int *)malloc(sizeof(int) * max);

    if (!fds)
        return ESP_ERR_NO_MEM;

    // =====================================================
    // Enumerate Active Clients
    // =====================================================

    size_t count = max;

    if (httpd_get_client_list(_server,
                              &count,
                              fds) == ESP_OK)
    {
        for (size_t i = 0; i < count; i++)
        {
            /**
             * Only broadcast to WebSocket clients.
             */
            if (httpd_ws_get_fd_info(
                    _server,
                    fds[i]) ==
                HTTPD_WS_CLIENT_WEBSOCKET)
            {
                wsSendToFd(
                    fds[i],
                    (const uint8_t *)msg,
                    strlen(msg),
                    isBinary);
            }
        }
    }

    free(fds);

    return ESP_OK;
}