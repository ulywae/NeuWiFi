#include "mdns/_NeuMDNS.h"
#include "mdns.h"
#include "SystemState/SharedState.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdio.h>

NeuMDNSClass NeuMDNS;

static char s_hostname[_NEU_MDNS_HOST_MAX_LEN] = {0};
static char s_instance_name[_NEU_MDNS_INSTANCE_MAX_LEN] = {0};
static char s_service_name[_NEU_MDNS_SERVICE_MAX_LEN] = {0};
static char s_proto[_NEU_MDNS_TEXT_MAX_LEN] = {0};
static uint16_t s_port = 0;

static char s_saved_hostname[_NEU_MDNS_HOST_MAX_LEN] = {0};
static char s_saved_instance[_NEU_MDNS_INSTANCE_MAX_LEN] = {0};
static bool s_callback_registered = false;

static NeuMDNSTXT s_saved_txt_responder;
static bool s_new_device_available = false;

typedef struct
{
    uint32_t timeout_ms;
    uint32_t interval_ms;
} discovery_config_t;

static discovery_config_t s_discovery_config;

typedef struct
{
    uint32_t ip;
    char hostname[_NEU_MDNS_HOST_MAX_LEN];
    uint16_t port;
    NeuMDNSTXT txt;
} mdns_device_packet_t;

static QueueHandle_t s_mdns_queue = NULL;
static StaticQueue_t s_static_queue_struct;
static uint8_t s_queue_storage[_NEU_MDNS_QUEUE_LEN * sizeof(mdns_device_packet_t)];

static bool s_task_created = false;
static volatile bool s_stop_requested = false;
static neu_mdns_discovery_cb_t s_user_callback = NULL;

static void local_mdns_network_event_cb(SharedState::Event e);

NeuMDNSClass::NeuMDNSClass() : _is_initialized(false) {}

void NeuMDNSClass::init(const char *hostname, const char *instance_name)
{
    if (_is_initialized)
        return;

    if (hostname)
    {
        memset(s_hostname, 0, sizeof(s_hostname));
        strncpy(s_hostname, hostname, _NEU_MDNS_HOST_MAX_LEN - 1);
        strncpy(s_saved_hostname, hostname, _NEU_MDNS_HOST_MAX_LEN - 1);
    }
    if (instance_name)
    {
        memset(s_instance_name, 0, sizeof(s_instance_name));
        strncpy(s_instance_name, instance_name, _NEU_MDNS_INSTANCE_MAX_LEN - 1);
        strncpy(s_saved_instance, instance_name, _NEU_MDNS_INSTANCE_MAX_LEN - 1);
    }

    if (!s_callback_registered)
    {
        SharedState::getInstance().onEvent(local_mdns_network_event_cb);
        s_callback_registered = true;
    }

    if (s_mdns_queue == NULL)
    {
        s_mdns_queue = xQueueCreateStatic(_NEU_MDNS_QUEUE_LEN, sizeof(mdns_device_packet_t), s_queue_storage, &s_static_queue_struct);
    }

    _is_initialized = true;
}

bool NeuMDNSClass::addService(const char *service_name, const char *proto, uint16_t port, const NeuMDNSTXT &txt)
{
    if (service_name)
    {
        memset(s_service_name, 0, sizeof(s_service_name));
        if (service_name[0] != '_')
        {
            s_service_name[0] = '_';
            strncpy(&s_service_name[1], service_name, _NEU_MDNS_SERVICE_MAX_LEN - 2);
        }
        else
        {
            strncpy(s_service_name, service_name, _NEU_MDNS_SERVICE_MAX_LEN - 1);
        }
    }
    if (proto)
    {
        memset(s_proto, 0, sizeof(s_proto));
        if (proto[0] != '_')
        {
            s_proto[0] = '_';
            strncpy(&s_proto[1], proto, _NEU_MDNS_TEXT_MAX_LEN - 2);
        }
        else
        {
            strncpy(s_proto, proto, _NEU_MDNS_TEXT_MAX_LEN - 1);
        }
    }
    s_port = port;
    s_saved_txt_responder = txt;
    return true;
}

void NeuMDNSClass::setCallback(neu_mdns_discovery_cb_t callback) { s_user_callback = callback; }

static void mdns_discovery_worker_task(void *pvParameters)
{
    discovery_config_t *config = (discovery_config_t *)pvParameters;
    char static_ip_string[16];
    bool internal_mdns_running = false;

    while (1)
    {
        if (s_stop_requested)
            break;

        if (!SharedState::getInstance().isNetworkReady())
        {
            if (internal_mdns_running)
            {
                mdns_free();
                internal_mdns_running = false;
            }
            if (s_user_callback)
                s_user_callback(NULL, NULL, NeuMDNSTXT());
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        if (!internal_mdns_running)
        {
            esp_err_t err = mdns_init();
            if (err == ESP_OK || err == ESP_ERR_INVALID_STATE)
            {
                if (s_hostname[0] != '\0')
                    mdns_hostname_set(s_hostname);
                if (s_instance_name[0] != '\0')
                    mdns_instance_name_set(s_instance_name);

                if (s_service_name[0] != '\0' && s_proto[0] != '\0' && s_port != 0)
                {
                    mdns_txt_item_t esp_txt[NEU_MDNS_MAX_TXT_ITEMS];
                    for (size_t i = 0; i < s_saved_txt_responder.count && i < NEU_MDNS_MAX_TXT_ITEMS; i++)
                    {
                        esp_txt[i].key = s_saved_txt_responder.items[i].key;
                        esp_txt[i].value = s_saved_txt_responder.items[i].value;
                    }
                    mdns_service_add(NULL, s_service_name, s_proto, s_port, esp_txt, s_saved_txt_responder.count);
                }
                internal_mdns_running = true;
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
        }

        if (s_mdns_queue != NULL)
            xQueueReset(s_mdns_queue);

        mdns_result_t *results = NULL;
        esp_err_t err = mdns_query_ptr(s_service_name, s_proto, config->timeout_ms, 5, &results);

        bool found = false;
        if (err == ESP_OK && results != NULL)
        {
            mdns_result_t *r = results;
            while (r != NULL)
            {
                if (r->hostname != NULL && r->addr != NULL && r->addr->addr.type == ESP_IPADDR_TYPE_V4)
                {
                    mdns_device_packet_t packet;
                    packet.ip = r->addr->addr.u_addr.ip4.addr;
                    packet.port = r->port;

                    strncpy(packet.hostname, r->hostname, _NEU_MDNS_HOST_MAX_LEN - 1);
                    packet.hostname[_NEU_MDNS_HOST_MAX_LEN - 1] = '\0';

                    packet.txt = NeuMDNSTXT();
                    if (r->txt_count > 0 && r->txt != NULL)
                    {
                        for (size_t i = 0; i < r->txt_count && i < NEU_MDNS_MAX_TXT_ITEMS; i++)
                        {
                            if (r->txt[i].key && r->txt[i].value)
                            {
                                packet.txt.add(r->txt[i].key, r->txt[i].value);
                            }
                        }
                    }

                    if (s_mdns_queue != NULL)
                    {
                        xQueueSend(s_mdns_queue, &packet, 0);
                    }

                    s_new_device_available = true;

                    if (s_user_callback)
                    {
                        IPConfig temp_ip;
                        temp_ip.setIP((uint8_t)(packet.ip & 0xFF),
                                      (uint8_t)((packet.ip >> 8) & 0xFF),
                                      (uint8_t)((packet.ip >> 16) & 0xFF),
                                      (uint8_t)((packet.ip >> 24) & 0xFF));

                        temp_ip.asStringIP(static_ip_string);
                        s_user_callback(packet.hostname, static_ip_string, packet.txt);
                    }
                    found = true;
                }
                r = r->next;
            }
        }

        if (!found && s_user_callback)
        {
            s_user_callback(NULL, NULL, NeuMDNSTXT());
        }
        if (results != NULL)
            mdns_query_results_free(results);
        if (config->interval_ms == 0)
            break;
        if (s_stop_requested)
            break;

        vTaskDelay(pdMS_TO_TICKS(config->interval_ms));
    }

    s_task_created = false;
    s_stop_requested = false;
    vTaskDelete(NULL);
}

bool NeuMDNSClass::startDiscovery(const char *service_name, const char *proto,
                                  uint32_t timeout_ms, uint32_t interval_ms)
{
    if (timeout_ms == 0)
        timeout_ms = 2000;
    if (interval_ms < 500 && interval_ms != 0)
        interval_ms = 5000;

    if (service_name)
    {
        if (service_name[0] != '_')
        {
            s_service_name[0] = '_';
            strncpy(&s_service_name[1], service_name, _NEU_MDNS_SERVICE_MAX_LEN - 2);
        }
        else
        {
            strncpy(s_service_name, service_name, _NEU_MDNS_SERVICE_MAX_LEN - 1);
        }
        s_service_name[_NEU_MDNS_SERVICE_MAX_LEN - 1] = '\0';
    }
    if (proto)
    {
        if (proto[0] != '_')
        {
            s_proto[0] = '_';
            strncpy(&s_proto[1], proto, _NEU_MDNS_TEXT_MAX_LEN - 2);
        }
        else
        {
            strncpy(s_proto, proto, _NEU_MDNS_TEXT_MAX_LEN - 1);
        }
        s_proto[_NEU_MDNS_TEXT_MAX_LEN - 1] = '\0';
    }

    s_discovery_config.timeout_ms = timeout_ms;
    s_discovery_config.interval_ms = interval_ms;

    if (s_task_created)
        return true;

    BaseType_t result = xTaskCreate(mdns_discovery_worker_task, "neu_mdns_task", 4096, (void *)&s_discovery_config, 5, NULL);
    if (result == pdPASS)
    {
        s_task_created = true;
        return true;
    }
    return false;
}

bool NeuMDNSClass::stopDiscovery()
{
    if (!s_task_created)
        return false;

    s_stop_requested = true;
    return true;
}

bool NeuMDNSClass::available()
{
    if (s_mdns_queue == NULL)
        return false;

    return (uxQueueMessagesWaiting(s_mdns_queue) > 0);
}

uint32_t NeuMDNSClass::readIP()
{
    mdns_device_packet_t packet;
    if (s_mdns_queue != NULL && xQueueReceive(s_mdns_queue, &packet, 0) == pdTRUE)
    {
        return packet.ip;
    }
    return 0;
}

bool NeuMDNSClass::readIP(IPConfig &out_config)
{
    mdns_device_packet_t packet;
    if (s_mdns_queue != NULL && xQueueReceive(s_mdns_queue, &packet, 0) == pdTRUE)
    {
        out_config.setIP((uint8_t)(packet.ip & 0xFF),
                         (uint8_t)((packet.ip >> 8) & 0xFF),
                         (uint8_t)((packet.ip >> 16) & 0xFF),
                         (uint8_t)((packet.ip >> 24) & 0xFF));
        return true;
    }
    return false;
}

bool NeuMDNSClass::readIP(char *out_str)
{
    if (!out_str)
        return false;

    IPConfig temp_ip;
    if (readIP(temp_ip))
    {
        temp_ip.asStringIP(out_str);
        return true;
    }
    return false;
}

uint16_t NeuMDNSClass::readPort()
{
    mdns_device_packet_t packet;
    if (s_mdns_queue != NULL && xQueuePeek(s_mdns_queue, &packet, 0) == pdTRUE)
    {
        return packet.port;
    }
    return 0;
}

bool NeuMDNSClass::readDevice(char *out_hostname, size_t host_max_len, IPConfig &out_config, NeuMDNSTXT &out_txt)
{
    if (!out_hostname || s_mdns_queue == NULL)
        return false;

    mdns_device_packet_t packet;
    if (xQueueReceive(s_mdns_queue, &packet, 0) == pdTRUE)
    {
        out_config.setIP((uint8_t)(packet.ip & 0xFF),
                         (uint8_t)((packet.ip >> 8) & 0xFF),
                         (uint8_t)((packet.ip >> 16) & 0xFF),
                         (uint8_t)((packet.ip >> 24) & 0xFF));

        strncpy(out_hostname, packet.hostname, host_max_len - 1);
        out_hostname[host_max_len - 1] = '\0';

        out_txt = packet.txt;

        if (uxQueueMessagesWaiting(s_mdns_queue) == 0)
        {
            s_new_device_available = false;
        }
        return true;
    }
    return false;
}

bool NeuMDNSClass::readDevice(char *out_hostname, size_t host_max_len, IPConfig &out_config, uint16_t &out_port, NeuMDNSTXT &out_txt)
{
    if (!out_hostname || s_mdns_queue == NULL)
        return false;

    mdns_device_packet_t packet;
    if (xQueueReceive(s_mdns_queue, &packet, 0) == pdTRUE)
    {
        out_config.setIP((uint8_t)(packet.ip & 0xFF),
                         (uint8_t)((packet.ip >> 8) & 0xFF),
                         (uint8_t)((packet.ip >> 16) & 0xFF),
                         (uint8_t)((packet.ip >> 24) & 0xFF));

        strncpy(out_hostname, packet.hostname, host_max_len - 1);
        out_hostname[host_max_len - 1] = '\0';

        out_port = packet.port;
        out_txt = packet.txt;

        if (uxQueueMessagesWaiting(s_mdns_queue) == 0)
        {
            s_new_device_available = false;
        }
        return true;
    }
    return false;
}

bool NeuMDNSClass::readDevice(char *out_hostname, size_t host_max_len, char *out_ip_str, NeuMDNSTXT &out_txt)
{
    if (!out_hostname || !out_ip_str)
        return false;

    IPConfig temp_ip;
    if (readDevice(out_hostname, host_max_len, temp_ip, out_txt))
    {
        temp_ip.asStringIP(out_ip_str);
        return true;
    }
    return false;
}

bool NeuMDNSClass::readDevice(char *out_hostname, size_t host_max_len, char *out_ip_str, uint16_t &out_port, NeuMDNSTXT &out_txt)
{
    if (!out_hostname || !out_ip_str || !s_new_device_available)
        return false;

    IPConfig temp_ip;
    if (readDevice(out_hostname, host_max_len, temp_ip, out_port, out_txt))
    {
        temp_ip.asStringIP(out_ip_str);
        return true;
    }
    return false;
}

bool NeuMDNSClass::end()
{
    if (!_is_initialized)
        return false;

    if (s_task_created)
    {
        s_stop_requested = true;
        int timeout_counter = 0;
        while (s_task_created && timeout_counter < 100)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            timeout_counter++;
        }
        if (s_task_created)
        {
            s_task_created = false;
            s_stop_requested = false;
        }
    }

    if (s_mdns_queue != NULL)
        xQueueReset(s_mdns_queue);

    mdns_service_remove_all();
    mdns_free();

    s_new_device_available = false;
    _is_initialized = false;
    return true;
}

static void local_mdns_network_event_cb(SharedState::Event e)
{
    if (e == SharedState::Event::MODE_CHANGED ||
        e == SharedState::Event::STA_DOWN || e == SharedState::Event::AP_DOWN ||
        e == SharedState::Event::STA_READY || e == SharedState::Event::AP_READY)
    {
        NeuMDNS.end();

        // When the network recovers, the intact s_saved_txt_responder will be ready to regenerate its service.
        if (SharedState::getInstance().isNetworkReady())
        {
            NeuMDNS.init(s_saved_hostname, s_saved_instance);
        }
    }
}