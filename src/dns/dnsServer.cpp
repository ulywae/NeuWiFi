/**
 * @file dnsServer.cpp
 * @author Ulywae
 * @brief Lightweight captive portal DNS server implementation for ESP32.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * Implements:
 * - Lightweight UDP DNS responder
 * - Captive portal redirection
 * - Embedded-safe DNS packet parsing
 * - Async FreeRTOS DNS runtime
 * - Dynamic AP IP resolution
 *
 * Runtime architecture:
 * UDP Socket
 * ↓
 * DNS Listener Task
 * ↓
 * DNS Query Parse
 * ↓
 * Captive Portal Redirect
 *
 * Designed for:
 * - ESP32 captive portals
 * - Offline configuration pages
 * - Local AP onboarding systems
 * - Embedded network redirection
 */

#include "dnsServer.h"

#include <string.h>
#include "lwip/sockets.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// =====================================================
// Constructor
// =====================================================

/**
 * @brief Construct lightweight DNS runtime.
 */
NeuDnsServer::NeuDnsServer()
{
}

// =====================================================
// Destructor
// =====================================================

/**
 * @brief Destroy DNS runtime safely.
 *
 * @details
 * Automatically stops:
 * - DNS socket
 * - Runtime listener
 * - Internal networking resources
 */
NeuDnsServer::~NeuDnsServer()
{
    stop();
}

// =====================================================
// DNS Server Startup
// =====================================================

/**
 * @brief Start captive portal DNS server.
 *
 * @details
 * Creates asynchronous FreeRTOS task
 * for DNS packet handling.
 *
 * Features:
 * - Lightweight runtime
 * - Embedded-safe task lifecycle
 * - Async UDP DNS processing
 *
 * Default DNS port:
 * 53
 *
 * Runtime flow:
 * start()
 * ↓
 * DNS Task
 * ↓
 * UDP Socket Bind
 * ↓
 * DNS Listener Loop
 *
 * @param port DNS listening port.
 *
 * @return true Task created successfully.
 * @return false Failed to create task.
 */
bool NeuDnsServer::start(int port)
{
    /**
     * Prevent duplicate startup.
     */
    if (_socket_fd != -1)
        return true;

    // =====================================================
    // Create DNS Runtime Task
    // =====================================================

    /**
     * Priority:
     * 5
     *
     * Stack:
     * 3072 bytes
     *
     * Optimized for:
     * - Fast DNS responses
     * - Lightweight parsing
     * - Captive portal workloads
     */
    BaseType_t ret =
        xTaskCreate(
            dnsTask,
            "neu_dns_task",
            3072,
            this,
            5,
            NULL);

    return (ret == pdPASS);
}

// =====================================================
// DNS Runtime Task
// =====================================================

/**
 * @brief Internal DNS listener runtime.
 *
 * @details
 * Handles:
 * - UDP DNS packets
 * - DNS query parsing
 * - Captive portal redirects
 * - DNS response generation
 *
 * Runtime flow:
 * recvfrom()
 * ↓
 * Parse DNS Header
 * ↓
 * Validate Query
 * ↓
 * Generate Redirect Response
 * ↓
 * sendto()
 *
 * Captive portal logic:
 * Any DNS request
 * ↓
 * Returns AP IP address
 *
 * @param pvParameters Runtime instance pointer.
 */
void NeuDnsServer::dnsTask(void *pvParameters)
{
    NeuDnsServer *self =
        (NeuDnsServer *)pvParameters;

    // =====================================================
    // Runtime Buffers
    // =====================================================

    uint8_t buffer[NEU_DNS_BUFFER_SIZE];

    struct sockaddr_in server_addr,
        client_addr;

    socklen_t client_addr_len =
        sizeof(client_addr);

    // =====================================================
    // Create UDP Socket
    // =====================================================

    self->_socket_fd =
        socket(AF_INET,
               SOCK_DGRAM,
               IPPROTO_IP);

    if (self->_socket_fd < 0)
    {
        vTaskDelete(NULL);
        return;
    }

    // =====================================================
    // Configure DNS Bind Address
    // =====================================================

    server_addr.sin_addr.s_addr =
        htonl(INADDR_ANY);

    server_addr.sin_family =
        AF_INET;

    /**
     * Standard DNS Port.
     */
    server_addr.sin_port =
        htons(53);

    // =====================================================
    // Bind DNS Socket
    // =====================================================

    if (bind(self->_socket_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        self->stop();
        vTaskDelete(NULL);
        return;
    }

    // =====================================================
    // Resolve AP IP Address Dynamically
    // =====================================================

    /**
     * Captive portal target IP.
     *
     * Retrieves:
     * ESP32 SoftAP IP address.
     */

    esp_netif_ip_info_t ip_info;

    esp_netif_t *netif =
        esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

    uint32_t cached_ip =
        (netif &&
         esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
            ? ip_info.ip.addr
            : inet_addr("192.168.4.1");

    // =====================================================
    // Main DNS Runtime Loop
    // =====================================================

    while (self->_socket_fd != -1)
    {
        // =================================================
        // Wait for DNS Packet
        // =================================================

        int len =
            recvfrom(
                self->_socket_fd,
                buffer,
                sizeof(buffer),
                0,
                (struct sockaddr *)&client_addr,
                &client_addr_len);

        // =================================================
        // Packet Validation
        // =================================================

        /**
         * Ignore invalid DNS packets.
         */
        if (len < (int)sizeof(DNSHeader))
            continue;

        DNSHeader *header =
            (DNSHeader *)buffer;

        uint16_t flags =
            ntohs(header->flags);

        // =================================================
        // Standard DNS Query Detection
        // =================================================

        /**
         * Ignore packets that are already:
         * - DNS responses
         */
        if (!(flags & 0x8000))
        {
            // =============================================
            // Configure DNS Response Header
            // =============================================

            /**
             * 0x8180:
             * - Standard response
             * - Recursion available
             * - No error
             */
            header->flags =
                htons(0x8180);

            /**
             * Match answer count
             * with question count.
             */
            header->an_count =
                header->qd_count;

            // =============================================
            // DNS Answer Section
            // =============================================

            /**
             * DNS Answer Layout:
             *
             * Pointer to query
             * Type A
             * Class IN
             * TTL
             * IPv4 Length
             * AP IP Address
             */

            uint8_t answer[] =
                {
                    0xc0, 0x0c,
                    0x00, 0x01,
                    0x00, 0x01,
                    0x00, 0x00, 0x00, 0x3c,
                    0x00, 0x04,

                    (uint8_t)(cached_ip & 0xFF),
                    (uint8_t)((cached_ip >> 8) & 0xFF),
                    (uint8_t)((cached_ip >> 16) & 0xFF),
                    (uint8_t)((cached_ip >> 24) & 0xFF)};

            // =============================================
            // Prevent Buffer Overflow
            // =============================================

            if (len + (int)sizeof(answer) <= NEU_DNS_BUFFER_SIZE)
            {
                memcpy(
                    buffer + len,
                    answer,
                    sizeof(answer));

                // =========================================
                // Send DNS Response
                // =========================================

                sendto(
                    self->_socket_fd,
                    buffer,
                    len + sizeof(answer),
                    0,
                    (struct sockaddr *)&client_addr,
                    client_addr_len);
            }
        }
    }

    // =====================================================
    // Self-Terminate Runtime Task
    // =====================================================

    vTaskDelete(NULL);
}

// =====================================================
// DNS Runtime Shutdown
// =====================================================

/**
 * @brief Stop DNS runtime safely.
 *
 * @details
 * Closes UDP socket and interrupts:
 * - recvfrom()
 * - DNS listener loop
 * - runtime task
 *
 * Runtime flow:
 * stop()
 * ↓
 * shutdown()
 * ↓
 * close()
 * ↓
 * dnsTask exits
 */
void NeuDnsServer::stop()
{
    if (_socket_fd != -1)
    {
        /**
         * Backup socket descriptor.
         */
        int temp_fd = _socket_fd;

        /**
         * Invalidate runtime state first.
         */
        _socket_fd = -1;

        // =================================================
        // Force recvfrom() Exit
        // =================================================

        shutdown(temp_fd,
                 SHUT_RDWR);

        close(temp_fd);
    }
}