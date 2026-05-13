/**
 * @file UDP.cpp
 * @author Ulywae
 * @brief Lightweight UDP runtime implementation for ESP32.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * This file implements:
 * - UDP socket initialization
 * - Async UDP packet listener
 * - UDP transmission
 * - Broadcast support
 * - Multicast support
 * - Runtime-safe socket shutdown
 * - FreeRTOS listener task
 *
 * Architecture goals:
 * - Deterministic UDP runtime
 * - Lightweight socket handling
 * - Embedded-safe networking
 * - Beginner-friendly asynchronous API
 * - Leak-free task lifecycle
 */

#include "UDP.h"
#include <string.h>

// =====================================================
// UDP Runtime Initialization
// =====================================================

/**
 * @brief Start UDP socket runtime.
 *
 * @details
 * Creates UDP socket and binds it
 * to the selected listening port.
 *
 * Features:
 * - Lightweight UDP listener
 * - FreeRTOS async packet task
 * - Embedded-safe startup flow
 *
 * Runtime flow:
 * begin()
 * ↓
 * Socket Create
 * ↓
 * Socket Bind
 * ↓
 * Async Listener Task
 *
 * @param port UDP listening port.
 *
 * @return true Initialization successful.
 * @return false Socket/bind failed.
 */
bool NeuUDP::begin(uint16_t port)
{
    // =====================================================
    // Store Listening Port
    // =====================================================

    _port = port;

    // =====================================================
    // Create UDP Socket
    // =====================================================

    _sock =
        socket(AF_INET,
               SOCK_DGRAM,
               IPPROTO_IP);

    if (_sock < 0)
        return false;

    // =====================================================
    // Configure Bind Address
    // =====================================================

    struct sockaddr_in addr;

    addr.sin_addr.s_addr =
        htonl(INADDR_ANY);

    addr.sin_family =
        AF_INET;

    addr.sin_port =
        htons(_port);

    // =====================================================
    // Bind Socket
    // =====================================================

    if (bind(_sock,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0)
    {
        stop();
        return false;
    }

    // =====================================================
    // Create Async Listener Task
    // =====================================================

    /**
     * Stack size:
     * 4096 bytes
     *
     * Suitable for:
     * - Socket operations
     * - Callback dispatch
     * - Packet buffering
     */
    xTaskCreate(
        [](void *p)
        {
            ((NeuUDP *)p)->_run();
        },
        "neu_udp_task",
        4096,
        this,
        5,
        NULL);

    return true;
}

// =====================================================
// Internal UDP Runtime Loop
// =====================================================

/**
 * @brief Internal UDP packet listener loop.
 *
 * @details
 * Continuously waits for incoming
 * UDP packets and dispatches them
 * to the registered callback.
 *
 * Runtime flow:
 * recvfrom()
 * ↓
 * Packet Parse
 * ↓
 * IP Conversion
 * ↓
 * User Callback
 *
 * Features:
 * - Asynchronous runtime
 * - Automatic sender detection
 * - Runtime-safe shutdown
 * - Self-cleaning task lifecycle
 */
void NeuUDP::_run()
{
    // =====================================================
    // Receive Buffer
    // =====================================================

    uint8_t buffer[1024];

    struct sockaddr_in source_addr;

    socklen_t socklen =
        sizeof(source_addr);

    // =====================================================
    // Main Runtime Loop
    // =====================================================

    while (true)
    {
        // =================================================
        // Socket Validity Check
        // =================================================

        if (_sock < 0)
            break;

        // =================================================
        // Wait for Incoming UDP Packet
        // =================================================

        int len =
            recvfrom(
                _sock,
                buffer,
                sizeof(buffer),
                0,
                (struct sockaddr *)&source_addr,
                &socklen);

        // =================================================
        // Incoming Packet
        // =================================================

        if (len > 0)
        {
            if (_onPacket)
            {
                // =========================================
                // Convert Source IP to String
                // =========================================

                char ip_str[16];

                inet_ntoa_r(
                    source_addr.sin_addr,
                    ip_str,
                    sizeof(ip_str));

                // =========================================
                // Dispatch Packet Callback
                // =========================================

                _onPacket(
                    buffer,
                    (size_t)len,
                    ip_str,
                    ntohs(source_addr.sin_port));
            }
        }

        // =================================================
        // Socket Failure / Shutdown
        // =================================================

        else if (len < 0)
        {
            /**
             * Socket closed or invalid.
             * Exit runtime loop safely.
             */
            break;
        }
    }

    // =====================================================
    // Self-Terminate Task
    // =====================================================

    /**
     * Automatically delete FreeRTOS task
     * after runtime exits.
     */
    vTaskDelete(NULL);
}

// =====================================================
// UDP Transmission
// =====================================================

/**
 * @brief Send UDP packet.
 *
 * @details
 * Sends binary payload to target:
 * - IP address
 * - UDP port
 *
 * Features:
 * - Lightweight socket send
 * - Binary-safe transmission
 * - Low-overhead runtime
 *
 * @param ip Target IP address.
 * @param port Target UDP port.
 * @param data Payload buffer.
 * @param len Payload length.
 *
 * @return int Bytes sent.
 * @return -1 Socket invalid.
 */
int NeuUDP::send(const char *ip,
                 uint16_t port,
                 const uint8_t *data,
                 size_t len)
{
    if (_sock < 0)
        return -1;

    // =====================================================
    // Configure Destination Address
    // =====================================================

    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr =
        inet_addr(ip);

    dest_addr.sin_family =
        AF_INET;

    dest_addr.sin_port =
        htons(port);

    // =====================================================
    // Send Packet
    // =====================================================

    return sendto(
        _sock,
        data,
        len,
        0,
        (struct sockaddr *)&dest_addr,
        sizeof(dest_addr));
}

// =====================================================
// Runtime Shutdown
// =====================================================

/**
 * @brief Stop UDP runtime safely.
 *
 * @details
 * Closes socket and triggers listener
 * task shutdown automatically.
 *
 * Runtime flow:
 * stop()
 * ↓
 * shutdown()
 * ↓
 * recvfrom() interrupted
 * ↓
 * _run() exits
 * ↓
 * Task self-delete
 */
void NeuUDP::stop()
{
    if (_sock != -1)
    {
        /**
         * Backup socket handle.
         */
        int temp_sock = _sock;

        /**
         * Invalidate runtime socket first
         * so _run() can detect shutdown.
         */
        _sock = -1;

        // =================================================
        // Force recvfrom() Exit
        // =================================================

        shutdown(temp_sock,
                 0);

        close(temp_sock);
    }
}

// =====================================================
// Broadcast Support
// =====================================================

/**
 * @brief Enable or disable UDP broadcast mode.
 *
 * @details
 * Allows sending packets to:
 * 255.255.255.255
 *
 * @param enable True to enable broadcast.
 */
void NeuUDP::setBroadcast(bool enable)
{
    if (_sock < 0)
        return;

    int opt =
        enable ? 1 : 0;

    setsockopt(
        _sock,
        SOL_SOCKET,
        SO_BROADCAST,
        &opt,
        sizeof(opt));
}

// =====================================================
// Multicast Support
// =====================================================

/**
 * @brief Join multicast group.
 *
 * @details
 * Enables reception of multicast packets.
 *
 * Supported examples:
 * - 224.x.x.x
 * - 239.x.x.x
 *
 * Features:
 * - Lightweight multicast join
 * - Embedded-safe configuration
 *
 * @param multicastIP Multicast group address.
 *
 * @return true Successfully joined group.
 * @return false Failed to join group.
 */
bool NeuUDP::joinMulticast(const char *multicastIP)
{
    if (_sock < 0)
        return false;

    // =====================================================
    // Configure Multicast Request
    // =====================================================

    struct ip_mreq mreq;

    mreq.imr_multiaddr.s_addr =
        inet_addr(multicastIP);

    mreq.imr_interface.s_addr =
        htonl(INADDR_ANY);

    // =====================================================
    // Join Multicast Group
    // =====================================================

    return (
        setsockopt(
            _sock,
            IPPROTO_IP,
            IP_ADD_MEMBERSHIP,
            &mreq,
            sizeof(mreq)) == 0);
}