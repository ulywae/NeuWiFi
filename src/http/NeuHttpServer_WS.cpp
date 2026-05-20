/**
 * @file NeuHttpServer_WS.cpp
 * @brief WebSocket functionality implementation for Neu HTTP Server.
 *
 * Provides WebSocket route registration, connection handling, frame processing,
 * event management (connect, disconnect, text, binary, ping/pong), and data transmission.
 * Includes broadcast capability and deep integration with system state for client tracking.
 */

#include "http/_NeuHttpServer.h"
#include "SystemState/SharedState.h"

/**
 * @brief Register a new WebSocket endpoint.
 *
 * Stores route information internally. Actual registration to server
 * happens automatically during server startup/reload in core task.
 *
 * @param uri URI path for the WebSocket endpoint (e.g., "/ws")
 * @param handler Callback function to handle WebSocket events and data
 */
void NeuHttpServer::onWS(const char *uri, NeuWSHandler handler)
{
    if (!handler || !uri)
        return;

    if (_ws_route_count < NEU_MAX_WS_ROUTES)
    {
        _saved_ws_routes[_ws_route_count] = {
            .uri = uri,
            .handler = handler};
        _ws_route_count++;
    }
    else
    {
        // WebSocket route storage full: maximum routes reached
    }
}

/**
 * @brief Internal wrapper handler for all WebSocket events.
 *
 * Manages the full WebSocket lifecycle:
 * - Performs handshake on initial GET request
 * - Integrates with SharedState to clear client from DNS tracking upon connection
 * - Receives and parses incoming frames (text, binary, ping, pong, close)
 * - Sends automatic responses according to protocol (PONG for PING, CLOSE for CLOSE)
 * - Triggers user-defined callback with appropriate event type
 *
 * @param req Pointer to HTTP request structure
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error or invalid state
 */
esp_err_t NeuHttpServer::_ws_internal_wrapper(httpd_req_t *req)
{
    int fd = httpd_req_to_sockfd(req);
    NeuWSHandler user_func = (NeuWSHandler)req->user_ctx;

    // =====================================================
    // STEP 1: HANDSHAKE JABAT TANGAN (WS EVENT CONNECT)
    // =====================================================
    if (req->method == HTTP_GET)
    {
        // SELF-HEALING INTEGRATION: Get client IP and remove from aggressive DNS tracking
        uint32_t client_ip = NeuHttpServer::getClientIP(req);
        if (client_ip != 0)
        {
            SharedState::getInstance().clearClientFromTracker(client_ip);
        }

        // Notify user of successful connection
        if (user_func)
        {
            user_func(fd, WS_EVENT_CONNECT, NULL, 0);
        }
        return ESP_OK;
    }

    // SOCKET PROTECTION: Abort processing if socket is no longer a valid WebSocket
    if (httpd_ws_get_fd_info(req->handle, fd) != HTTPD_WS_CLIENT_WEBSOCKET)
    {
        return ESP_FAIL;
    }

    // STEP 1: Find out the original size of the payload data sent by the client without reading it first
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

    // Calling a function with a NULL pointer forces ESP-IDF to only populate the ws_pkt.len variable
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        if (user_func)
            user_func(fd, WS_EVENT_DISCONNECT, NULL, 0);
        return ret;
    }

    // STEP 2: Prepare the main static buffer in the stack
    uint8_t buf[NEU_SERVER_WS_BUF_SIZE];
    memset(buf, 0, sizeof(buf));

    // MAXIMUM PROTECTION: If client data is larger than our buffer, truncate forcefully!
    size_t max_read_len = ws_pkt.len;
    if (max_read_len >= NEU_SERVER_WS_BUF_SIZE)
    {
        max_read_len = NEU_SERVER_WS_BUF_SIZE - 1;
    }

    // STEP 2: Allocate memory for actual data reception safely according to max_read_len
    ws_pkt.payload = buf;
    ret = httpd_ws_recv_frame(req, &ws_pkt, max_read_len);
    if (ret != ESP_OK)
    {
        // If the read fails midway, discard the remaining packets to prevent memory leaks.
        if (user_func)
        {
            user_func(fd, WS_EVENT_DISCONNECT, NULL, 0);
        }
        return ret;
    }

    // =====================================================
    // STEP 2: PROCESS PAYLOAD FRAME DATA VIA SWITCH CASE
    // =====================================================
    if (user_func)
    {
        // Make sure the ws_pkt.len variable does not truncate the string reading.
        size_t final_len = (ws_pkt.len < max_read_len) ? ws_pkt.len : max_read_len;

        switch (ws_pkt.type)
        {
        case HTTPD_WS_TYPE_TEXT:
            buf[final_len] = '\0'; // Null-terminate the string
            user_func(fd, WS_EVENT_TEXT, (const char *)buf, final_len);
            break;

        case HTTPD_WS_TYPE_BINARY:
            user_func(fd, WS_EVENT_BINARY, (const char *)buf, final_len);
            break;

        case HTTPD_WS_TYPE_PING:
        {
            httpd_ws_frame_t pong_pkt = {.type = HTTPD_WS_TYPE_PONG};
            httpd_ws_send_frame(req, &pong_pkt);
            user_func(fd, WS_EVENT_PING, NULL, 0);
            break;
        }

        case HTTPD_WS_TYPE_PONG:
            user_func(fd, WS_EVENT_PONG, (const char *)buf, final_len);
            break;

        case HTTPD_WS_TYPE_CLOSE:
        {
            httpd_ws_frame_t close_pkt = {.type = HTTPD_WS_TYPE_CLOSE};
            httpd_ws_send_frame(req, &close_pkt);
            user_func(fd, WS_EVENT_DISCONNECT, NULL, 0);
            break;
        }

        default:
            break;
        }
    }
    return ESP_OK;
}

/**
 * @brief Send text message through active WebSocket request.
 *
 * Used primarily inside the event handler during request processing.
 *
 * @param req Pointer to current HTTP/WebSocket request
 * @param msg Null-terminated string message to send
 * @return esp_err_t ESP_OK on success, ESP_FAIL if parameters invalid
 */
esp_err_t NeuHttpServer::wsSendText(httpd_req_t *req, const char *msg)
{
    if (!req || !msg)
        return ESP_FAIL;

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)msg;
    ws_pkt.len = strlen(msg);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    return httpd_ws_send_frame(req, &ws_pkt);
}

/**
 * @brief Send binary data through active WebSocket request.
 *
 * Used primarily inside the event handler during request processing.
 *
 * @param req Pointer to current HTTP/WebSocket request
 * @param data Pointer to binary data buffer
 * @param len Length of data in bytes
 * @return esp_err_t ESP_OK on success, ESP_FAIL if parameters invalid
 */
esp_err_t NeuHttpServer::wsSendBin(httpd_req_t *req, uint8_t *data, size_t len)
{
    if (!req || !data || len == 0)
        return ESP_FAIL;

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = data;
    ws_pkt.len = len;
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    return httpd_ws_send_frame(req, &ws_pkt);
}

/**
 * @brief Send text message to specific client socket ID (asynchronous).
 *
 * @param fd Client socket file descriptor
 * @param msg Null-terminated string message
 * @return esp_err_t ESP_OK on success, ESP_FAIL if invalid parameters or server not running
 */
esp_err_t NeuHttpServer::wsSendText(int id, const char *msg)
{
    if (!msg)
        return ESP_FAIL;

    return wsSendToFd(id, (const uint8_t *)msg, strlen(msg), false);
}

/**
 * @brief Send binary data to specific client socket ID (asynchronous).
 *
 * @param fd Client socket file descriptor
 * @param data Pointer to binary data buffer
 * @param len Length of data in bytes
 * @return esp_err_t ESP_OK on success, ESP_FAIL if invalid parameters or server not running
 */
esp_err_t NeuHttpServer::wsSendBin(int id, const uint8_t *data, size_t len)
{
    return wsSendToFd(id, data, len, true);
}

/**
 * @brief Low-level function to send data to specific client socket.
 *
 * Uses asynchronous send to avoid blocking current task.
 *
 * @param fd Client socket file descriptor
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 * @param isBinary True = binary frame, False = text frame
 * @return esp_err_t ESP_OK on success, ESP_FAIL if server not running
 */
esp_err_t NeuHttpServer::wsSendToFd(int fd, const uint8_t *data, size_t len, bool isBinary)
{
    if (!_server)
        return ESP_FAIL;

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = isBinary ? HTTPD_WS_TYPE_BINARY : HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = (uint8_t *)data;
    ws_pkt.len = len;
    return httpd_ws_send_frame_async(_server, fd, &ws_pkt);
}

/**
 * @brief Send PING keep-alive frame to specific client.
 *
 * @param fd Client socket file descriptor
 * @return esp_err_t ESP_OK on success, ESP_FAIL if server not running
 */
esp_err_t NeuHttpServer::wsSendPing(int fd)
{
    if (!_server)
        return ESP_FAIL;

    httpd_ws_frame_t ping_pkt = {.type = HTTPD_WS_TYPE_PING};
    return httpd_ws_send_frame_async(_server, fd, &ping_pkt);
}

/**
 * @brief Worker task for broadcast operation.
 *
 * Runs in server context via queue_work. Iterates all active clients,
 * filters WebSocket connections only, and sends message to each.
 * Frees allocated memory automatically after completion.
 *
 * @param arg Pointer to ws_broadcast_ctx_t structure containing message and metadata
 */
void NeuHttpServer::_ws_broadcast_worker(void *arg)
{
    ws_broadcast_ctx_t *ctx = (ws_broadcast_ctx_t *)arg;
    if (!ctx || ctx->server_ptr == nullptr)
        return;

    NeuHttpServer *self = ctx->server_ptr;
    httpd_handle_t hd = self->_server;

    size_t max_sessions = NEU_MAX_HTTP_ROUTES + NEU_MAX_WS_ROUTES;
    int *fds = (int *)malloc(sizeof(int) * max_sessions);
    if (!fds)
    {
        free(ctx->data);
        free(ctx);
        return;
    }

    size_t count = max_sessions;
    if (httpd_get_client_list(hd, &count, fds) == ESP_OK)
    {
        for (size_t i = 0; i < count; i++)
        {
            if (httpd_ws_get_fd_info(hd, fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET)
                self->wsSendToFd(fds[i], ctx->data, ctx->len, ctx->is_binary);
        }
    }

    free(fds);
    free(ctx->data);
    free(ctx);
}

/**
 * @brief Broadcast message/data to all connected WebSocket clients.
 *
 * Allocates memory, copies payload, and schedules broadcast operation
 * as background work to avoid blocking caller.
 *
 * @param data Pointer to data buffer to broadcast
 * @param len Length of data in bytes
 * @param isBinary True = send as binary frame, False = send as text frame
 * @return esp_err_t ESP_OK if scheduled successfully, error code otherwise
 */
esp_err_t NeuHttpServer::wsBroadcast(const uint8_t *data, size_t len, bool isBinary)
{
    if (!_server || !data || len == 0)
        return ESP_FAIL;

    ws_broadcast_ctx_t *ctx = (ws_broadcast_ctx_t *)malloc(sizeof(ws_broadcast_ctx_t));
    if (!ctx)
        return ESP_ERR_NO_MEM;

    ctx->server_ptr = this;
    ctx->data = (uint8_t *)malloc(len);
    if (!ctx->data)
    {
        free(ctx);
        return ESP_ERR_NO_MEM;
    }

    memcpy(ctx->data, data, len);
    ctx->len = len;
    ctx->is_binary = isBinary;

    // Schedule broadcast in server's work queue
    esp_err_t err = httpd_queue_work(_server, NeuHttpServer::_ws_broadcast_worker, ctx);
    if (err != ESP_OK)
    {
        free(ctx->data);
        free(ctx);
        return err;
    }

    return ESP_OK;
}
