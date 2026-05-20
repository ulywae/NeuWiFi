/**
 * @file NeuHttpServer_Helper.cpp
 * @brief Helper implementation for Neu HTTP Server module.
 *
 * Provides utility functions, error handling, response formatting, redirection,
 * URL decoding, and POST parameter parsing for the HTTP server subsystem.
 * Integrates with SharedState to apply smart redirection logic based on system state.
 */

#include "http/_NeuHttpServer.h"
#include "SystemState/SharedState.h"
#include <ctype.h>

/**
 * @brief Global pointer to user-defined custom error handler function.
 */
NeuErrorHandler NeuHttpServer::_user_error_handler = nullptr;

/**
 * @brief Register a custom callback function to handle HTTP errors.
 *
 * The provided handler will be invoked whenever an HTTP error occurs,
 * allowing user-defined error responses or logging.
 *
 * @param handler Function pointer matching the error handler signature;
 *                pass nullptr to use default error handling.
 */
void NeuHttpServer::onError(NeuErrorHandler handler)
{
    _user_error_handler = handler;
}

/**
 * @brief Convert HTTP error code to human-readable status string.
 *
 * @param error HTTP error code from underlying HTTP server library
 * @return const char* String representation of the error status
 */
const char *NeuHttpServer::errorToString(httpd_err_code_t error)
{
    switch (error)
    {
    case HTTPD_500_INTERNAL_SERVER_ERROR:
        return "500 Internal Server Error";

    case HTTPD_501_METHOD_NOT_IMPLEMENTED:
        return "501 Method Not Implemented";

    case HTTPD_505_VERSION_NOT_SUPPORTED:
        return "505 HTTP Version Not Supported";

    case HTTPD_400_BAD_REQUEST:
        return "400 Bad Request";

    case HTTPD_401_UNAUTHORIZED:
        return "401 Unauthorized";

    case HTTPD_403_FORBIDDEN:
        return "403 Forbidden";

    case HTTPD_404_NOT_FOUND:
        return "404 Not Found";

    case HTTPD_405_METHOD_NOT_ALLOWED:
        return "405 Method Not Allowed";

    case HTTPD_408_REQ_TIMEOUT:
        return "408 Request Timeout";

    case HTTPD_411_LENGTH_REQUIRED:
        return "411 Length Required";

    case HTTPD_414_URI_TOO_LONG:
        return "414 URI Too Long";

    case HTTPD_431_REQ_HDR_FIELDS_TOO_LARGE:
        return "431 Request Header Fields Too Large";

    default:
        return "Unknown HTTP Error";
    }
}

/**
 * @brief Internal error handler callback registered with the HTTP server.
 *
 * If a user error handler is registered, it will be executed.
 * Otherwise, default handling applies:
 * - For 404 Not Found: performs smart redirection based on active DNS state and Host header.
 *   Redirects to `http://neu.portal` if DNS is active and accessed from another address,
 *   otherwise redirects to root path (/).
 * - For other errors: sends standard error response with status text.
 *
 * @param req Pointer to HTTP request structure
 * @param err Error code that occurred
 * @return esp_err_t ESP_OK on successful handling
 */
esp_err_t NeuHttpServer::_internal_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (_user_error_handler)
    {
        _user_error_handler(req, err);
    }
    else
    {
        if (err == HTTPD_404_NOT_FOUND)
        {
            char target_redirect_url[20];
            bool dns_is_running = SharedState::getInstance().isDnsActive();

            char host_header[64] = {0};
            size_t host_len = httpd_req_get_hdr_value_len(req, "Host");

            if (host_len > 0 && host_len < sizeof(host_header))
            {
                httpd_req_get_hdr_value_str(req, "Host", host_header, sizeof(host_header));
            }

            if (dns_is_running)
            {
                if (strstr(host_header, "neu.portal") == nullptr)
                {
                    snprintf(target_redirect_url, sizeof(target_redirect_url), "http://neu.portal");
                }
                else
                {
                    snprintf(target_redirect_url, sizeof(target_redirect_url), "/");
                }
            }
            else
            {
                snprintf(target_redirect_url, sizeof(target_redirect_url), "/");
            }

            return NeuHttpServer::redirect(req, target_redirect_url);
        }
        else
        {
            const char *err_msg = NeuHttpServer::errorToString(err);
            httpd_resp_send_err(req, err, err_msg);
        }
    }
    return ESP_OK;
}

/**
 * @brief Send plain text response to client.
 *
 * Automatically sets content length based on string length.
 *
 * @param req Pointer to HTTP request structure
 * @param content Null-terminated string to send as response body
 * @return esp_err_t ESP_OK on success, ESP_FAIL if parameters are invalid
 */
esp_err_t NeuHttpServer::sendText(httpd_req_t *req, const char *content)
{
    if (!req || !content)
        return ESP_FAIL;

    return httpd_resp_send(req, content, HTTPD_RESP_USE_STRLEN);
}

/**
 * @brief Send JSON formatted response to client.
 *
 * Sets Content-Type header to `application/json` automatically.
 *
 * @param req Pointer to HTTP request structure
 * @param json Null-terminated JSON string to send
 * @return esp_err_t ESP_OK on success, ESP_FAIL if parameters are invalid
 */
esp_err_t NeuHttpServer::sendJSON(httpd_req_t *req, const char *json)
{
    if (!req || !json)
        return ESP_FAIL;

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

/**
 * @brief Send HTTP redirect response (307 Temporary Redirect).
 *
 * Sets Location header to target URL and applies cache control headers
 * to prevent browser caching of the redirection. Connection is closed after response.
 *
 * @param req Pointer to HTTP request structure
 * @param location Target URL or path for redirection
 * @return esp_err_t ESP_OK on success, ESP_FAIL if parameters are invalid
 */
esp_err_t NeuHttpServer::redirect(httpd_req_t *req, const char *location)
{
    if (!req || !location)
        return ESP_FAIL;

    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", location);

    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");

    httpd_resp_set_hdr(req, "Connection", "close");

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, NULL, 0);
}

/**
 * @brief Decode URL-encoded string into plain text.
 *
 * Converts percent-encoded characters (%XX) and plus signs (+) to spaces.
 * Result is null-terminated.
 *
 * @param dst Destination buffer to store decoded string
 * @param src Source URL-encoded string
 */
static void url_decode(char *dst, const char *src)
{
    char a, b;
    while (*src)
    {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit((unsigned char)a) && isxdigit((unsigned char)b)))
        {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= 'A' - 10;
            else
                a -= '0';

            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= 'A' - 10;
            else
                b -= '0';

            *dst++ = 16 * a + b;
            src += 3;
        }
        else if (*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

/**
 * @brief Extract and decode a single parameter from POST form data.
 *
 * Receives request body data, searches for the specified key,
 * decodes the value, and stores it in destination buffer.
 *
 * @param req Pointer to HTTP request structure
 * @param param Name of parameter to extract
 * @param dest Destination buffer to store decoded value
 * @param dest_len Maximum size of destination buffer
 * @return esp_err_t ESP_OK if parameter found and extracted successfully;
 *         ESP_FAIL on invalid input or receive error;
 *         ESP_ERR_NOT_FOUND if key not present
 */
esp_err_t NeuHttpServer::getPostParam(httpd_req_t *req, const char *param, char *dest, size_t dest_len)
{
    if (!req || !param || !dest || dest_len == 0)
        return ESP_FAIL;

    char buf[NEU_SERVER_POST_BUF_SIZE];
    memset(buf, 0, sizeof(buf));

    size_t total_expected = (req->content_len < sizeof(buf) - 1) ? req->content_len : sizeof(buf) - 1;
    size_t total_received = 0;

    while (total_received < total_expected)
    {
        int ret = httpd_req_recv(req, buf + total_received, total_expected - total_received);

        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            continue;

        if (ret <= 0)
            return ESP_FAIL;

        total_received += ret;
    }

    buf[total_received] = '\0';

    char temp_extracted[NEU_SERVER_POST_BUF_SIZE] = {0};
    esp_err_t err = httpd_query_key_value(buf, param, temp_extracted, sizeof(temp_extracted) - 1);

    if (err == ESP_OK)
    {
        url_decode(dest, temp_extracted);
        return ESP_OK;
    }

    return err;
}
