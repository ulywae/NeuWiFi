// file NeuHttpClient.cpp

#include "http/_NeuHttpClient.h"
#include "esp_crt_bundle.h"


esp_err_t NeuHttpClient::_http_event_handler(esp_http_client_event_t *evt)
{
    ResponseContext *ctx = (ResponseContext *)evt->user_data;

    if (evt->event_id == HTTP_EVENT_ON_DATA)
    {
        if (ctx && ctx->buffer && (ctx->data_read < ctx->buffer_len - 1))
        {
            size_t space = ctx->buffer_len - ctx->data_read - 1;
            size_t copy_len = (evt->data_len < space) ? evt->data_len : space;

            if (copy_len > 0)
            {
                memcpy(ctx->buffer + ctx->data_read, evt->data, copy_len);
                ctx->data_read += copy_len;
                ctx->buffer[ctx->data_read] = '\0';
            }
        }
    }
    return ESP_OK;
}


esp_err_t NeuHttpClient::post(
    const char *url, const uint8_t *data, size_t data_len,
    const HttpHeader *headers, size_t header_count,
    char *response_buffer, size_t buffer_len)
{
    if (response_buffer && buffer_len > 0)
    {
        response_buffer[0] = '\0';
    }

    ResponseContext ctx = {response_buffer, buffer_len, 0};

    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.event_handler = _http_event_handler;
    config.user_data = &ctx;
    config.timeout_ms = 5000;                         
    config.crt_bundle_attach = esp_crt_bundle_attach; 

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
        return ESP_FAIL;

    if (headers != nullptr)
    {
        for (size_t i = 0; i < header_count; i++)
        {
            if (headers[i].key && headers[i].value)
            {
                esp_http_client_set_header(client, headers[i].key, headers[i].value);
            }
        }
    }

    esp_http_client_set_post_field(client, (const char *)data, data_len);
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    return err;
}


esp_err_t NeuHttpClient::get(
    const char *url, const HttpHeader *headers, size_t header_count,
    char *response_buffer, size_t buffer_len)
{
    if (response_buffer && buffer_len > 0)
    {
        response_buffer[0] = '\0';
    }

    ResponseContext ctx = {response_buffer, buffer_len, 0};

    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_GET;
    config.event_handler = _http_event_handler;
    config.user_data = &ctx;
    config.timeout_ms = 5000;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
        return ESP_FAIL;

    if (headers != nullptr)
    {
        for (size_t i = 0; i < header_count; i++)
        {
            if (headers[i].key && headers[i].value)
            {
                esp_http_client_set_header(client, headers[i].key, headers[i].value);
            }
        }
    }

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    return err;
}
