/**
 * @file httpServer_Helper.cpp
 * @author Ulywae
 * @brief HTTP helper and error handling implementation for NeuHttpServer.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * This file contains:
 * - HTTP error handling system
 * - Error-to-string conversion
 * - Static response helpers
 * - Redirect utilities
 * - POST parameter parsing
 * - Internal error bridge logic
 *
 * Design goals:
 * - Beginner-friendly HTTP handling
 * - Lightweight runtime overhead
 * - Deterministic response flow
 * - Embedded-safe request parsing
 * - Safe fallback mechanisms
 */

#include "httpServer.h"

// =====================================================
// Static Runtime Variables
// =====================================================

/**
 * @brief Global user-defined HTTP error callback.
 *
 * @details
 * Shared across all NeuHttpServer instances.
 */
NeuErrorHandler NeuHttpServer::_user_error_handler = nullptr;

// =====================================================
// Error Handler Registration
// =====================================================

/**
 * @brief Register custom HTTP error handler.
 *
 * @details
 * Automatically attaches internal middleware
 * for multiple HTTP error codes.
 *
 * Supported errors:
 * - 400 Bad Request
 * - 401 Unauthorized
 * - 403 Forbidden
 * - 404 Not Found
 * - 405 Method Not Allowed
 * - 408 Request Timeout
 * - 500 Internal Server Error
 * - 501 Method Not Implemented
 *
 * @param handler User-defined error callback.
 */
void NeuHttpServer::onError(NeuErrorHandler handler)
{
    if (!_server)
        return;

    // =====================================================
    // Store User Callback
    // =====================================================

    _user_error_handler = handler;

    // =====================================================
    // List of Managed Error Codes
    // =====================================================

    httpd_err_code_t err_codes[] =
        {
            HTTPD_400_BAD_REQUEST,
            HTTPD_401_UNAUTHORIZED,
            HTTPD_403_FORBIDDEN,
            HTTPD_404_NOT_FOUND,
            HTTPD_405_METHOD_NOT_ALLOWED,
            HTTPD_408_REQ_TIMEOUT,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            HTTPD_501_METHOD_NOT_IMPLEMENTED};

    // =====================================================
    // Register Internal Error Wrapper
    // =====================================================

    for (int i = 0;
         i < sizeof(err_codes) / sizeof(err_codes[0]);
         i++)
    {
        httpd_register_err_handler(
            _server,
            err_codes[i],
            _internal_error_handler);
    }
}

// =====================================================
// Error String Conversion
// =====================================================

/**
 * @brief Convert HTTP error code into readable text.
 *
 * @param error HTTP error code.
 *
 * @return const char* Human-readable error string.
 */
const char *NeuHttpServer::errorToString(httpd_err_code_t error)
{
    switch (error)
    {
    case HTTPD_404_NOT_FOUND:
        return "404 Not Found";

    case HTTPD_405_METHOD_NOT_ALLOWED:
        return "405 Method Not Allowed";

    case HTTPD_400_BAD_REQUEST:
        return "400 Bad Request";

    case HTTPD_401_UNAUTHORIZED:
        return "401 Unauthorized";

    case HTTPD_403_FORBIDDEN:
        return "403 Forbidden";

    case HTTPD_500_INTERNAL_SERVER_ERROR:
        return "500 Internal Server Error";

    case HTTPD_408_REQ_TIMEOUT:
        return "408 Request Timeout";

    default:
        return "Unknown Error";
    }
}

// =====================================================
// Internal Error Dispatcher
// =====================================================

/**
 * @brief Internal HTTP error middleware.
 *
 * @details
 * Handles:
 * - User-defined error callbacks
 * - Automatic fallback responses
 * - Embedded-safe error replies
 *
 * Fallback behavior:
 * If no custom error handler exists,
 * NeuHttpServer automatically sends
 * default HTTP error responses.
 *
 * @param req HTTP request object.
 * @param err HTTP error code.
 *
 * @return esp_err_t Always returns ESP_OK.
 */
esp_err_t NeuHttpServer::_internal_error_handler(httpd_req_t *req,
                                                 httpd_err_code_t err)
{
    // =====================================================
    // User Custom Error Handler
    // =====================================================

    if (_user_error_handler)
    {
        _user_error_handler(req,
                            err);
    }
    else
    {
        // =================================================
        // Default Fallback Error Response
        // =================================================

        const char *err_msg =
            NeuHttpServer::errorToString(err);

        httpd_resp_send_err(req,
                            err,
                            err_msg);
    }

    /**
     * Return ESP_OK to prevent
     * forced socket termination
     * unless protocol requires it.
     */
    return ESP_OK;
}

// =====================================================
// Static HTTP Helpers
// =====================================================

/**
 * @brief Send plain text response.
 *
 * @details
 * Sends string response using automatic
 * string length calculation.
 *
 * @param req HTTP request object.
 * @param content Text response.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::sendText(httpd_req_t *req,
                                  const char *content)
{
    return httpd_resp_send(
        req,
        content,
        HTTPD_RESP_USE_STRLEN);
}

/**
 * @brief Send JSON response.
 *
 * @details
 * Automatically sets:
 * application/json
 * MIME type.
 *
 * @param req HTTP request object.
 * @param json JSON payload.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::sendJSON(httpd_req_t *req,
                                  const char *json)
{
    httpd_resp_set_type(
        req,
        "application/json");

    return httpd_resp_send(
        req,
        json,
        HTTPD_RESP_USE_STRLEN);
}

/**
 * @brief Send HTTP redirect response.
 *
 * @details
 * Sends:
 * - HTTP 302 Found
 * - Location header
 *
 * @param req HTTP request object.
 * @param location Redirect destination.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::redirect(httpd_req_t *req,
                                  const char *location)
{
    httpd_resp_set_status(
        req,
        "302 Found");

    httpd_resp_set_hdr(
        req,
        "Location",
        location);

    return httpd_resp_send(
        req,
        NULL,
        0);
}

/**
 * @brief Retrieve POST parameter value.
 *
 * @details
 * Reads POST request body and extracts:
 * key=value
 * parameter pairs.
 *
 * Features:
 * - Safe buffer limits
 * - Automatic null termination
 * - Embedded-friendly parsing
 *
 * @param req HTTP request object.
 * @param param Parameter name.
 * @param dest Destination buffer.
 * @param dest_len Destination buffer size.
 *
 * @return esp_err_t ESP-IDF status.
 */
esp_err_t NeuHttpServer::getPostParam(httpd_req_t *req,
                                      const char *param,
                                      char *dest,
                                      size_t dest_len)
{
    // =====================================================
    // Temporary POST Buffer
    // =====================================================

    char buf[NEU_SERVER_POST_BUF_SIZE];

    // =====================================================
    // Limit Receive Size
    // =====================================================

    size_t recv_size =
        (req->content_len < sizeof(buf))
            ? req->content_len
            : sizeof(buf) - 1;

    // =====================================================
    // Receive Request Body
    // =====================================================

    int ret =
        httpd_req_recv(
            req,
            buf,
            recv_size);

    if (ret <= 0)
        return ESP_FAIL;

    // =====================================================
    // Ensure Safe String Termination
    // =====================================================

    buf[ret] = '\0';

    // =====================================================
    // Extract Target Parameter
    // =====================================================

    return httpd_query_key_value(
        buf,
        param,
        dest,
        dest_len);
}