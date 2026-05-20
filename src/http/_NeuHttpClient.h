// file httpClient.h

#pragma once

#include "esp_http_client.h"
#include <string.h>


struct HttpHeader
{
    const char *key;
    const char *value;
};


class NeuHttpClient
{
public:
    static esp_err_t post(
        const char *url,
        const uint8_t *data,
        size_t data_len,
        const HttpHeader *headers = nullptr,
        size_t header_count = 0,
        char *response_buffer = nullptr,
        size_t buffer_len = 0);

    static esp_err_t get(
        const char *url,
        const HttpHeader *headers = nullptr,
        size_t header_count = 0,
        char *response_buffer = nullptr,
        size_t buffer_len = 0);

private:
    struct ResponseContext
    {
        char *buffer;
        size_t buffer_len;
        size_t data_read;
    };

    static esp_err_t _http_event_handler(
        esp_http_client_event_t *evt);
};