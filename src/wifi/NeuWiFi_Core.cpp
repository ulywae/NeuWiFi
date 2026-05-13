/**
 * @file NeuWiFi_Core.cpp
 * @author Ulywae
 * @brief Core runtime implementation for NeuWiFi.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * This file contains the main runtime engine for NeuWiFi,
 * including:
 *
 * - WiFi driver initialization
 * - TCP/IP stack setup
 * - Event system integration
 * - Runtime synchronization
 * - Internal event handling
 * - Hostname management
 * - TX power configuration
 * - Safe WiFi shutdown
 *
 * Design goals:
 * - Deterministic runtime behavior
 * - Defensive initialization
 * - Low-overhead event dispatching
 * - Stable reconnect handling
 * - Embedded-safe operation
 */

#include "_NeuWiFi.h"

/**
 * @brief Construct a new NeuWiFi object.
 *
 * @details
 * Initializes internal runtime defaults and validates
 * AP maximum client count.
 *
 * Allowed AP client range:
 * - Minimum: 1
 * - Maximum: 10
 *
 * Invalid values automatically fallback to 4.
 *
 * @param max_conn Maximum AP clients.
 */
NeuWiFi::NeuWiFi(uint8_t max_conn)
{
    /**
     * Runtime safety guard for AP client count.
     */
    _max_conn =
        (max_conn < 1 || max_conn > 10)
            ? 4
            : max_conn;
}

/**
 * @brief Initialize WiFi subsystem.
 *
 * @details
 * Performs complete initialization of:
 *
 * - TCP/IP stack
 * - ESP event loop
 * - WiFi driver
 * - Internal synchronization objects
 * - Event dispatch system
 *
 * Features:
 * - Double initialization protection
 * - ESP-IDF default event loop integration
 * - Unified event handler architecture
 * - Runtime-safe initialization flow
 */
void NeuWiFi::init()
{
    /**
     * Prevent duplicate initialization.
     */
    if (_event_group)
        return;

    // =====================================================
    // Initialize TCP/IP Stack
    // =====================================================

    esp_netif_init();

    // =====================================================
    // Create Default Event Loop
    // =====================================================

    /**
     * Create default ESP-IDF event loop.
     *
     * ESP_ERR_INVALID_STATE is acceptable because
     * it means the loop already exists.
     */
    esp_err_t err = esp_event_loop_create_default();

    if (err != ESP_OK &&
        err != ESP_ERR_INVALID_STATE)
    {
        /**
         * Optional error handling location.
         *
         * Can be extended later for:
         * - logging
         * - assertions
         * - fault recovery
         */
    }

    // =====================================================
    // Create Internal Event Group
    // =====================================================

    /**
     * Event group used for:
     * - connection state tracking
     * - scan synchronization
     * - failure detection
     */
    _event_group = xEventGroupCreate();

    // =====================================================
    // Initialize WiFi Driver
    // =====================================================

    /**
     * Load default ESP-IDF WiFi configuration.
     */
    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&cfg);

    // =====================================================
    // Register Unified Event Handlers
    // =====================================================

    /**
     * Register handler for all WiFi events.
     */
    esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &NeuWiFi::eventHandler,
        this,
        NULL);

    /**
     * Register handler for STA IP acquisition.
     */
    esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &NeuWiFi::eventHandler,
        this,
        NULL);
}

/**
 * @brief Unified internal event dispatcher.
 *
 * @details
 * Centralized event handler for:
 *
 * - WiFi events
 * - IP events
 * - Connection management
 * - Internal synchronization
 * - User callback forwarding
 *
 * Internal responsibilities:
 * - Auto reconnect
 * - State synchronization
 * - Scan completion tracking
 * - Connection flag management
 *
 * @param arg Pointer to NeuWiFi instance.
 * @param base Event base type.
 * @param id Event identifier.
 * @param data Event payload pointer.
 */
void NeuWiFi::eventHandler(void *arg,
                           esp_event_base_t base,
                           int32_t id,
                           void *data)
{
    /**
     * Recover class instance pointer.
     */
    NeuWiFi *self = (NeuWiFi *)arg;

    // =====================================================
    // Internal WiFi Event Logic
    // =====================================================

    if (base == WIFI_EVENT)
    {
        switch (id)
        {
        /**
         * STA mode started.
         * Automatically begin connection attempt.
         */
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;

        /**
         * STA disconnected.
         */
        case WIFI_EVENT_STA_DISCONNECTED:

            /**
             * Clear connected flag.
             */
            xEventGroupClearBits(
                self->_event_group,
                WIFI_CONNECTED_BIT);

            /**
             * Mark failure state.
             */
            xEventGroupSetBits(
                self->_event_group,
                WIFI_FAIL_BIT);

            /**
             * Auto reconnect logic.
             */
            if (self->_auto_reconnect)
            {
                esp_wifi_connect();
            }

            break;

        /**
         * WiFi scan completed.
         */
        case WIFI_EVENT_SCAN_DONE:

            xEventGroupSetBits(
                self->_event_group,
                WIFI_SCAN_DONE_BIT);

            break;

        /**
         * AP client connected.
         */
        case WIFI_EVENT_AP_STACONNECTED:

            /**
             * Internal slave connection logic
             * may be extended here.
             */
            break;

        /**
         * AP client disconnected.
         */
        case WIFI_EVENT_AP_STADISCONNECTED:

            /**
             * Internal slave disconnect logic
             * may be extended here.
             */
            break;
        }
    }

    // =====================================================
    // IP Event Logic
    // =====================================================

    else if (base == IP_EVENT &&
             id == IP_EVENT_STA_GOT_IP)
    {
        /**
         * Mark successful connection.
         */
        xEventGroupSetBits(
            self->_event_group,
            WIFI_CONNECTED_BIT);

        /**
         * Clear failure state.
         */
        xEventGroupClearBits(
            self->_event_group,
            WIFI_FAIL_BIT);
    }

    // =====================================================
    // User Callback Bridge
    // =====================================================

    /**
     * Forward raw event to user callback.
     */
    if (self->_user_callback)
    {
        self->_user_callback(id,
                             data);
    }
}

/**
 * @brief Set device hostname.
 *
 * @details
 * Updates internal hostname buffer safely
 * using bounded string copy protection.
 *
 * Features:
 * - Null pointer protection
 * - Guaranteed null termination
 * - Buffer overflow prevention
 *
 * @param name Hostname string.
 */
void NeuWiFi::setHostname(const char *name)
{
    if (name)
    {
        strncpy(_hostname,
                name,
                sizeof(_hostname) - 1);

        /**
         * Force null terminator for safety.
         */
        _hostname[sizeof(_hostname) - 1] = '\0';
    }
}

/**
 * @brief Set WiFi TX power.
 *
 * @details
 * Configures WiFi transmit power.
 *
 * Accepted range:
 * - Minimum: 2 dBm
 * - Maximum: 20 dBm
 *
 * ESP-IDF internally uses:
 * - 0.25 dBm units
 *
 * Example:
 * - 20 dBm = 80 internal units
 *
 * @param dbm TX power in dBm.
 */
void NeuWiFi::setTxPower(int8_t dbm)
{
    /**
     * Clamp minimum power.
     */
    if (dbm < 2)
        dbm = 2;

    /**
     * Clamp maximum power.
     */
    if (dbm > 20)
        dbm = 20;

    /**
     * Convert dBm to ESP-IDF internal unit.
     */
    esp_wifi_set_max_tx_power(dbm * 4);
}

/**
 * @brief Safely stop WiFi subsystem.
 *
 * @details
 * Stops the WiFi radio and underlying
 * driver operation.
 *
 * Intended for:
 * - power saving
 * - runtime shutdown
 * - safe reconfiguration
 */
void NeuWiFi::stop()
{
    esp_wifi_stop();
}