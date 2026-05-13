/**
 * @file httpClient.cpp
 * @author Ulywae
 * @brief Lightweight HTTP client implementation for ESP32.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * This file implements the runtime engine for
 * NeuHttpClient using ESP-IDF HTTP Client API.
 *
 * Features:
 * - HTTP GET support
 * - HTTP POST support
 * - HTTPS support via CRT bundle
 * - Automatic response buffering
 * - Lightweight event-driven architecture
 * - Embedded-safe memory handling
 *
 * Design goals:
 * - Minimal runtime overhead
 * - Deterministic request handling
 * - Beginner-friendly networking API
 * - Safe response buffering
 * - Leak-free request lifecycle
 */

#include "httpClient.h"
#include "esp_crt_bundle.h"

// =====================================================
// Internal HTTP Event Handler
// =====================================================

/**
 * @brief Internal HTTP client event dispatcher.
 *
 * @details
 * Handles incoming HTTP response chunks
 * and safely appends them into the user
 * response buffer.
 *
 * Features:
 * - Safe buffer limiting
 * - Automatic null termination
 * - Chunked response support
 * - Embedded-safe memory operations
 *
 * Supported events:
 * - HTTP_EVENT_ON_DATA
 *
 * @param evt ESP-IDF HTTP client event.
 *
 * @return esp_err_t Always returns ESP_OK.
 */
esp_err_t NeuHttpClient::_http_event_handler(
    esp_http_client_event_t *evt)
{
    /**
     * Recover response buffering context.
     */
    ResponseContext *ctx =
        (ResponseContext *)evt->user_data;

    // =====================================================
    // Incoming Response Data
    // =====================================================

    /**
     * We only care about incoming payload data.
     */
    if (evt->event_id == HTTP_EVENT_ON_DATA)
    {
        // =================================================
        // Buffer Safety Validation
        // =================================================

        if (ctx &&
            ctx->buffer &&
            (ctx->data_read < ctx->buffer_len - 1))
        {
            /**
             * Calculate remaining buffer space.
             */
            size_t space =
                ctx->buffer_len -
                ctx->data_read - 1;

            /**
             * Prevent overflow during copy.
             */
            size_t copy_len =
                (evt->data_len < space)
                    ? evt->data_len
                    : space;

            // =============================================
            // Safe Buffer Copy
            // =============================================

            if (copy_len > 0)
            {
                memcpy(
                    ctx->buffer + ctx->data_read,
                    evt->data,
                    copy_len);

                ctx->data_read += copy_len;

                /**
                 * Always null terminate string.
                 */
                ctx->buffer[ctx->data_read] = '\0';
            }
        }
    }

    return ESP_OK;
}

// =====================================================
// HTTP POST
// =====================================================

/**
 * @brief Execute HTTP POST request.
 *
 * @details
 * Sends POST payload to target URL
 * using ESP-IDF HTTP client runtime.
 *
 * Features:
 * - HTTPS support
 * - Optional custom headers
 * - Automatic response buffering
 * - Safe request cleanup
 *
 * @param url Target URL.
 * @param data POST payload.
 * @param data_len Payload length.
 * @param headers Optional HTTP headers.
 * @param header_count Header count.
 * @param response_buffer Optional response buffer.
 * @param buffer_len Response buffer size.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpClient::post(
    const char *url,
    const uint8_t *data,
    size_t data_len,
    const HttpHeader *headers,
    size_t header_count,
    char *response_buffer,
    size_t buffer_len)
{
    // =====================================================
    // Initialize Response Buffer
    // =====================================================

    if (response_buffer && buffer_len > 0)
        response_buffer[0] = '\0';

    /**
     * Create response buffering context.
     */
    ResponseContext ctx =
        {
            response_buffer,
            buffer_len,
            0};

    // =====================================================
    // Configure HTTP Client
    // =====================================================

    esp_http_client_config_t config = {};

    config.url = url;

    config.method =
        HTTP_METHOD_POST;

    config.event_handler =
        _http_event_handler;

    config.user_data = &ctx;

    /**
     * Request timeout.
     */
    config.timeout_ms = 5000;

    /**
     * Enable built-in HTTPS certificate bundle.
     */
    config.crt_bundle_attach =
        esp_crt_bundle_attach;

    // =====================================================
    // Create HTTP Client
    // =====================================================

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    if (!client)
        return ESP_FAIL;

    // =====================================================
    // Apply Custom Headers
    // =====================================================

    for (size_t i = 0;
         i < header_count;
         i++)
    {
        esp_http_client_set_header(
            client,
            headers[i].key,
            headers[i].value);
    }

    // =====================================================
    // Attach POST Payload
    // =====================================================

    esp_http_client_set_post_field(
        client,
        (const char *)data,
        data_len);

    // =====================================================
    // Execute Request
    // =====================================================

    esp_err_t err =
        esp_http_client_perform(client);

    // =====================================================
    // Cleanup Runtime Resources
    // =====================================================

    /**
     * Prevent memory leaks.
     */
    esp_http_client_cleanup(client);

    return err;
}

// =====================================================
// HTTP GET
// =====================================================

/**
 * @brief Execute HTTP GET request.
 *
 * @details
 * Sends HTTP GET request to target URL.
 *
 * Features:
 * - HTTPS support
 * - Optional custom headers
 * - Automatic response buffering
 * - Lightweight request flow
 *
 * @param url Target URL.
 * @param headers Optional HTTP headers.
 * @param header_count Header count.
 * @param response_buffer Optional response buffer.
 * @param buffer_len Response buffer size.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpClient::get(
    const char *url,
    const HttpHeader *headers,
    size_t header_count,
    char *response_buffer,
    size_t buffer_len)
{
    // =====================================================
    // Initialize Response Buffer
    // =====================================================

    if (response_buffer && buffer_len > 0)
        response_buffer[0] = '\0';

    /**
     * Create response buffering context.
     */
    ResponseContext ctx =
        {
            response_buffer,
            buffer_len,
            0};

    // =====================================================
    // Configure HTTP Client
    // =====================================================

    esp_http_client_config_t config = {};

    config.url = url;

    config.method =
        HTTP_METHOD_GET;

    config.event_handler =
        _http_event_handler;

    config.user_data = &ctx;

    /**
     * Request timeout.
     */
    config.timeout_ms = 5000;

    /**
     * Enable HTTPS certificate bundle.
     */
    config.crt_bundle_attach =
        esp_crt_bundle_attach;

    // =====================================================
    // Create HTTP Client
    // =====================================================

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    if (!client)
        return ESP_FAIL;

    // =====================================================
    // Apply Custom Headers
    // =====================================================

    for (size_t i = 0;
         i < header_count;
         i++)
    {
        esp_http_client_set_header(
            client,
            headers[i].key,
            headers[i].value);
    }

    // =====================================================
    // Execute HTTP Request
    // =====================================================

    esp_err_t err =
        esp_http_client_perform(client);

    // =====================================================
    // Cleanup Runtime Resources
    // =====================================================

    /**
     * Prevent memory leaks.
     */
    esp_http_client_cleanup(client);

    return err;
}