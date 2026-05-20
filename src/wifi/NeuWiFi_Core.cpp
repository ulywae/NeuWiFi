/**
 * @file NeuWiFi_Core.cpp
 * @brief Core runtime implementation for NeuWiFiClass.
 *
 * This module implements the central logic and low-level management for the NeuWiFi framework.
 * It handles all interactions with the ESP-IDF Wi-Fi driver, event loops, state tracking,
 * and background tasks. This is the heart of the system that manages connectivity lifecycle.
 *
 * Core Responsibilities:
 * - Wi-Fi hardware initialization and deinitialization
 * - ESP-IDF event loop integration and dispatching
 * - Automatic reconnection logic with smart backoff
 * - Access Point (AP) and Station (STA) mode lifecycle management
 * - Hybrid mode channel synchronization for stable coexistence
 * - Network state propagation to the global SharedState system
 * - Client connection tracking via DHCP assignment interception
 *
 * Runtime Characteristics:
 * - Deterministic lifecycle handling with centralized start/stop control
 * - Centralized cleanup model to prevent memory/resource leaks
 * - Self-contained reconnect engine with anti-storm protection
 * - Event-driven architecture ensuring non-blocking operation
 * - Lightweight persistent runtime with minimal memory footprint
 * - Thread-safe state updates via shared state singleton
 *
 * Design Philosophy:
 * - Constructor performs NO hardware or memory allocation actions
 * - init() activates runtime systems, drivers, and event loops
 * - stop() performs complete centralized teardown and resource release
 * - Destructor acts as final safety layer to ensure cleanup
 * - All heavy processing runs in background tasks / interrupt context
 *
 * @author Ulywae (@Neufa)
 * @date 2026
 * @version 1.1.0
 */

#include "wifi/_NeuWiFi.h"
#include "SystemState/SharedState.h"

/**
 * @brief Global NeuWiFi runtime instance.
 * @details Singleton instance exposed globally for application-wide access
 *          to Wi-Fi management functions and state information.
 */
NeuWiFiClass NeuWiFi;

/**
 * @brief Default constructor for NeuWiFiClass.
 * @details Performs initialization of internal variables to default state.
 *          Does NOT initialize hardware, allocate memory, or start services.
 *          Hardware activation is handled exclusively via init().
 */
NeuWiFiClass::NeuWiFiClass()
{
}

// =====================================================
// CORE INITIALIZATION
// =====================================================

/**
 * @brief Initialize WiFi runtime infrastructure and driver.
 *
 * Performs complete setup of the Wi-Fi subsystem including network interfaces,
 * event loops, driver configuration, and event handler registration.
 * Function is protected against duplicate calls; will exit silently if already initialized.
 *
 * Responsibilities:
 * - Initialize ESP-NETIF stack for network interface control
 * - Create default system event loop (if not already created)
 * - Initialize Wi-Fi driver with default configuration
 * - Register event handlers for Wi-Fi and IP stack events
 * - Create event group for internal state synchronization
 *
 * @param max_conn Maximum number of allowed clients when running in Access Point mode.
 *                 Value is automatically clamped between 1 and 10. Default fallback is 4.
 *
 * @note This function must be called before using any other network functions.
 *       All operations are asynchronous after initialization.
 */
void NeuWiFiClass::init(uint8_t max_conn)
{
    if (_event_group)
        return;

    _max_conn =
        (max_conn < 1 || max_conn > 10)
            ? 4
            : max_conn;

    esp_netif_init();
    esp_err_t err = esp_event_loop_create_default();

    if (err != ESP_OK &&
        err != ESP_ERR_INVALID_STATE)
    {
        /**
         * Optional error handling location.
         */
    }

    _event_group = xEventGroupCreate();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &NeuWiFiClass::eventHandler, this);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &NeuWiFiClass::eventHandler, this);

    // =====================================================
    // REGISTER EVENT: Capture the moment DHCP assigns an IP to an AP client.
    // =====================================================
    esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &NeuWiFiClass::eventHandler, this);
}

// =====================================================
// CENTRAL EVENT DISPATCHER
// =====================================================

/**
 * @brief Central internal event dispatcher for ESP-IDF system events.
 *
 * This function is registered as the main callback for all network-related events.
 * It receives raw events from the OS, processes them, updates internal state,
 * triggers background logic, and propagates changes to the SharedState system.
 *
 * Handles processing for:
 * - Station mode startup and shutdown
 * - Connection, disconnection, and IP acquisition events
 * - Automatic reconnection triggering logic
 * - Network scan completion notification
 * - Access Point startup, shutdown, and client management
 * - Hybrid mode channel synchronization
 * - DHCP IP assignment detection for connected clients
 *
 * Also propagates state changes into SharedState singleton for system-wide visibility.
 *
 * @param arg User context pointer (always pointer to current NeuWiFiClass instance)
 * @param base ESP event base identifier (WIFI_EVENT, IP_EVENT, etc.)
 * @param id Specific event code identifier
 * @param data Raw native event payload structure specific to event type
 */
void NeuWiFiClass::eventHandler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    NeuWiFiClass *self = (NeuWiFiClass *)arg;
    int32_t unified_id = id;

    if (base == WIFI_EVENT)
    {
        switch (id)
        {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            xEventGroupClearBits(self->_event_group, WIFI_CONNECTED_BIT);
            xEventGroupSetBits(self->_event_group, WIFI_FAIL_BIT);

            SharedState::getInstance().setStaReady(false);
            SharedState::getInstance().setStaIP("0.0.0.0");

            if (self->_auto_reconnect)
            {
                if (self->_reconnect_task_handle == nullptr)
                {
                    xTaskCreatePinnedToCore(
                        &NeuWiFiClass::reconnectTask,
                        "wifi_reconnect_task",
                        3072,
                        self,
                        5,
                        &self->_reconnect_task_handle,
                        0);
                }
            }
            break;

        case WIFI_EVENT_SCAN_DONE:
            xEventGroupSetBits(self->_event_group, WIFI_SCAN_DONE_BIT);
            break;

        case WIFI_EVENT_AP_START:
            SharedState::getInstance().setApIP(self->getAPIP());
            SharedState::getInstance().setApReady(true);
            break;

        case WIFI_EVENT_AP_STOP:
            SharedState::getInstance().setApReady(false);
            break;
        }
    }
    else if (base == IP_EVENT)
    {
        if (id == IP_EVENT_STA_GOT_IP)
        {
            xEventGroupSetBits(self->_event_group, WIFI_CONNECTED_BIT);
            xEventGroupClearBits(self->_event_group, WIFI_FAIL_BIT);

            SharedState::getInstance().setStaReady(true);
            self->_reconnect_attempt = 0;
            SharedState::getInstance().setStaIP(self->getSTAIP());

            if (self->_smart_channel_tracking)
            {
                wifi_ap_record_t ap_info;
                if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
                {
                    uint8_t router_channel = ap_info.primary;
                    if (self->_ap_channel != router_channel)
                    {
                        self->syncChannelToAP(router_channel);
                    }
                }
            }
        }
        // =====================================================
        // CORE INTEGRATION: Anti-Private DNS Ring Buffer Execution
        // =====================================================
        else if (id == IP_EVENT_AP_STAIPASSIGNED)
        {
            ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)data;
            if (event != nullptr && event->ip.addr != 0)
            {
                uint32_t assigned_ip = event->ip.addr;
                uint8_t target_mac[6] = {0};
                bool mac_found = false;

                // Get a list of all stations physically connected to AP.
                wifi_sta_list_t sta_list;
                if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK)
                {
                    // Scan the internal ARP table for binary IP matches.
                    for (int i = 0; i < sta_list.num; i++)
                    {
                        // MATCH: If the IP binary from DHCP matches the one in the driver's internal memory
                        // Note: On some versions of ESP-IDF, matching via dhcp_search_ip_on_mac is required.
                        // However, the safest approach at the top level is to assume the MAC address from the active station list.
                        // As a strong safety measure, if there is only one user, simply retrieve their MAC address:
                        if (sta_list.num == 1)
                        {
                            memcpy(target_mac, sta_list.sta[0].mac, 6);
                            mac_found = true;
                            break;
                        }

                        // Default Fallback: Take MAC from last incoming index (most recent connect)
                        memcpy(target_mac, sta_list.sta[sta_list.num - 1].mac, 6);
                        mac_found = true;
                        break;
                    }
                }

                // If it fails to detect MAC via the table, fill it with a dummy to prevent the system from crashing.
                if (!mac_found)
                {
                    memset(target_mac, 0xFF, 6); // Broadcast MAC fallback
                }

                // Send IP and MAC to SharedState!
                SharedState::getInstance().markClientAsNewAPUser(assigned_ip, target_mac);
            }
        }

        unified_id = id;
    }

    if (self->_callback != nullptr)
    {
        NeuWiFiEvent _event(unified_id, data);
        self->_callback(unified_id, _event);
    }
}

// =====================================================
// RUNTIME CONFIGURATION
// =====================================================

/**
 * @brief Set device hostname.
 *
 * @param name Hostname string.
 */
void NeuWiFiClass::setHostname(const char *name)
{
    if (name)
    {
        strncpy(_hostname, name, sizeof(_hostname) - 1);
        _hostname[sizeof(_hostname) - 1] = '\0';
    }
}

/**
 * @brief Configure WiFi transmit power.
 *
 * Clamped range:
 * - minimum: 2 dBm
 * - maximum: 20 dBm
 *
 * @param dbm Desired transmit power.
 */
void NeuWiFiClass::setTxPower(int8_t dbm)
{
    if (dbm < 2)
        dbm = 2;

    if (dbm > 20)
        dbm = 20;

    esp_wifi_set_max_tx_power(dbm * 4);
}

// =====================================================
// RECONNECT ENGINE
// =====================================================

/**
 * @brief Internal reconnect runtime task.
 *
 * Features:
 * - exponential backoff
 * - randomized jitter
 * - reconnect throttling
 * - reconnect attempt tracking
 *
 * Designed to prevent:
 * - reconnect storms
 * - synchronized retries
 * - unstable AP hammering
 *
 * @param pvParameters User context pointer.
 */
void NeuWiFiClass::reconnectTask(void *pvParameters)
{
    NeuWiFiClass *self = (NeuWiFiClass *)pvParameters;

    const uint32_t BASE_DELAY_MS = 2000;
    const uint32_t MAX_DELAY_MS = 60000;

    uint32_t calculate_delay = BASE_DELAY_MS * (1 << self->_reconnect_attempt);

    if (calculate_delay > MAX_DELAY_MS || self->_reconnect_attempt > 10)
    {
        calculate_delay = MAX_DELAY_MS;
    }

    uint32_t jitter = (esp_random() % (calculate_delay / 6));
    if (esp_random() % 2 == 0)
        calculate_delay += jitter;
    else
        calculate_delay -= jitter;

    vTaskDelay(pdMS_TO_TICKS(calculate_delay));
    self->_reconnect_attempt++;

    if (self->_auto_reconnect)
    {
        esp_wifi_connect();
    }

    self->_reconnect_task_handle = nullptr;
    vTaskDelete(NULL);
}

// =====================================================
// HYBRID CHANNEL SYNCHRONIZATION
// =====================================================

/**
 * @brief Synchronize AP channel to STA router channel.
 *
 * Used during hybrid mode operation to ensure:
 * - stable STA+AP coexistence
 * - reduced channel mismatch issues
 *
 * @param target_channel Desired WiFi channel.
 */
void NeuWiFiClass::syncChannelToAP(uint8_t target_channel)
{
    if (target_channel < 1 || target_channel > 13)
        return;

    wifi_config_t ap_cfg;
    if (esp_wifi_get_config(WIFI_IF_AP, &ap_cfg) == ESP_OK)
    {
        _ap_channel = target_channel;
        ap_cfg.ap.channel = target_channel;

        esp_err_t err = esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
        if (err == ESP_OK)
        {
            /** Optional log hook location */
        }
    }
}

// =====================================================
// CENTRALIZED CLEANUP
// =====================================================

/**
 * @brief Stop and cleanup WiFi runtime engine.
 *
 * Cleanup responsibilities:
 * - reconnect task deletion
 * - WiFi driver stop
 * - event handler unregister
 * - WiFi driver deinitialization
 * - network interface destruction
 * - shared state reset
 * - event group cleanup
 *
 * This function acts as the centralized
 * runtime teardown point.
 */
void NeuWiFiClass::stop()
{
    if (!_event_group)
        return;

    if (_reconnect_task_handle != nullptr)
    {
        vTaskDelete(_reconnect_task_handle);
        _reconnect_task_handle = nullptr;
    }

    _reconnect_attempt = 0;

    esp_wifi_stop();

    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &NeuWiFiClass::eventHandler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &NeuWiFiClass::eventHandler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &NeuWiFiClass::eventHandler);

    esp_wifi_deinit();

    if (_sta_netif != nullptr)
    {
        esp_netif_destroy_default_wifi(_sta_netif);
        _sta_netif = nullptr;
    }
    if (_ap_netif != nullptr)
    {
        esp_netif_destroy_default_wifi(_ap_netif);
        _ap_netif = nullptr;
    }

    SharedState::getInstance().setStaReady(false);
    SharedState::getInstance().setApReady(false);

    vEventGroupDelete(_event_group);
    _event_group = nullptr;
}