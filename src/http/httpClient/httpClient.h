/**
 * @file httpClient.h
 * @author Ulywae
 * @brief Lightweight HTTP client helper for ESP32.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * NeuHttpClient is a lightweight wrapper around
 * ESP-IDF esp_http_client API.
 *
 * Features:
 * - Simple HTTP GET requests
 * - Simple HTTP POST requests
 * - Custom header support
 * - Optional response buffering
 * - Lightweight event-driven architecture
 * - Embedded-safe response handling
 *
 * Design goals:
 * - Minimal boilerplate
 * - Beginner-friendly API
 * - Deterministic networking behavior
 * - Lightweight memory usage
 * - ESP32-only optimization
 *
 * Intended for:
 * - REST APIs
 * - IoT dashboards
 * - Device telemetry
 * - Embedded web services
 * - Local HTTP communication
 */

#pragma once

// =====================================================
// Platform Validation
// =====================================================

#if !defined(ESP32)

/**
 * @brief Platform protection.
 *
 * NeuHttpClient is designed specifically
 * for ESP32 + ESP-IDF runtime.
 */
#error "Platform not supported! NeuHttpClient currently only supports ESP32."

#endif

#include "esp_http_client.h"
#include <string.h>

// =====================================================
// HTTP Header Structure
// =====================================================

/**
 * @struct HttpHeader
 * @brief Simple HTTP header container.
 *
 * @details
 * Used for:
 * - Authorization headers
 * - Content-Type
 * - API tokens
 * - Custom metadata
 */
struct HttpHeader
{
    /** @brief Header key. */
    const char *key;

    /** @brief Header value. */
    const char *value;
};

/**
 * @class NeuHttpClient
 * @brief Lightweight static HTTP client utility.
 *
 * @details
 * Provides simplified:
 * - GET requests
 * - POST requests
 * - Header management
 * - Response buffering
 *
 * Uses ESP-IDF internal event-based
 * HTTP client architecture.
 *
 * Architecture goals:
 * - Stateless helper design
 * - Minimal runtime allocation
 * - Easy integration
 * - Embedded-friendly networking
 */
class NeuHttpClient
{
public:
    // =====================================================
    // HTTP POST
    // =====================================================

    /**
     * @brief Execute HTTP POST request.
     *
     * @details
     * Sends binary or text payload to
     * target URL using HTTP POST.
     *
     * Features:
     * - Custom headers
     * - Optional response buffering
     * - Embedded-safe handling
     *
     * @param url Target URL.
     * @param data POST payload buffer.
     * @param data_len Payload size.
     * @param headers Optional HTTP headers.
     * @param header_count Number of headers.
     * @param response_buffer Optional response buffer.
     * @param buffer_len Response buffer size.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t post(
        const char *url,
        const uint8_t *data,
        size_t data_len,
        const HttpHeader *headers = nullptr,
        size_t header_count = 0,
        char *response_buffer = nullptr,
        size_t buffer_len = 0);

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
     * - Custom headers
     * - Optional response buffering
     * - Lightweight request flow
     *
     * @param url Target URL.
     * @param headers Optional HTTP headers.
     * @param header_count Number of headers.
     * @param response_buffer Optional response buffer.
     * @param buffer_len Response buffer size.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t get(
        const char *url,
        const HttpHeader *headers = nullptr,
        size_t header_count = 0,
        char *response_buffer = nullptr,
        size_t buffer_len = 0);

private:
    // =====================================================
    // Internal Response Context
    // =====================================================

    /**
     * @struct ResponseContext
     * @brief Internal response buffering context.
     *
     * @details
     * Stores:
     * - Target response buffer
     * - Buffer capacity
     * - Current received size
     *
     * Used internally by:
     * _http_event_handler()
     */
    struct ResponseContext
    {
        /** @brief User response buffer. */
        char *buffer;

        /** @brief Maximum response size. */
        size_t buffer_len;

        /** @brief Current bytes received. */
        size_t data_read;
    };

    // =====================================================
    // Internal HTTP Event Handler
    // =====================================================

    /**
     * @brief Internal ESP-IDF HTTP event handler.
     *
     * @details
     * Handles:
     * - Incoming response chunks
     * - Data buffering
     * - HTTP client events
     * - Connection lifecycle
     *
     * @param evt ESP-IDF HTTP event.
     *
     * @return esp_err_t ESP-IDF status.
     */
    static esp_err_t _http_event_handler(
        esp_http_client_event_t *evt);
};