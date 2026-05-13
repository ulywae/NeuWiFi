/**
 * @file UDP.h
 * @author Ulywae
 * @brief Lightweight UDP networking helper for ESP32.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * NeuUDP is a lightweight wrapper around
 * LWIP socket-based UDP communication.
 *
 * Features:
 * - UDP send/receive
 * - Broadcast support
 * - Multicast support
 * - Callback-based packet handling
 * - Lightweight socket abstraction
 * - Embedded-friendly runtime behavior
 *
 * Design goals:
 * - Deterministic networking flow
 * - Minimal overhead
 * - Beginner-friendly API
 * - Low-level socket flexibility
 * - Lightweight memory usage
 *
 * Intended for:
 * - Real-time device communication
 * - IoT telemetry
 * - LED synchronization
 * - Multiplayer/local discovery
 * - Broadcast protocols
 */

#pragma once

#include <functional>
#include "lwip/sockets.h"

// =====================================================
// UDP Packet Callback
// =====================================================

/**
 * @brief Modern UDP packet callback signature.
 *
 * @details
 * Triggered whenever a UDP packet
 * is received by NeuUDP runtime.
 *
 * Parameters:
 * - data  : Incoming packet buffer
 * - len   : Packet size
 * - ip    : Sender IP address
 * - port  : Sender port
 *
 * Example:
 * @code
 * udp.setCallback([](uint8_t* data,
 *                    size_t len,
 *                    const char* ip,
 *                    uint16_t port)
 * {
 *     printf("Packet from %s:%d\n", ip, port);
 * });
 * @endcode
 */
using UDPPacketCallback =
    std::function<void(
        uint8_t *data,
        size_t len,
        const char *ip,
        uint16_t port)>;

/**
 * @class NeuUDP
 * @brief Lightweight UDP socket helper.
 *
 * @details
 * Provides:
 * - UDP transmission
 * - Packet reception
 * - Broadcast support
 * - Multicast joining
 * - Callback-based event handling
 *
 * Internally uses:
 * LWIP BSD socket API.
 *
 * Runtime model:
 * User Loop
 * ↓
 * _run()
 * ↓
 * Packet Callback
 */
class NeuUDP
{
private:
    // =====================================================
    // Internal Runtime State
    // =====================================================

    /**
     * @brief Internal UDP socket handle.
     */
    int _sock = -1;

    /**
     * @brief Active listening port.
     */
    uint16_t _port;

    /**
     * @brief User packet callback.
     */
    UDPPacketCallback _onPacket = nullptr;

public:
    // =====================================================
    // Constructor
    // =====================================================

    /**
     * @brief Construct lightweight UDP runtime.
     */
    NeuUDP() {}

    // =====================================================
    // Lifecycle
    // =====================================================

    /**
     * @brief Start UDP listener.
     *
     * @details
     * Creates socket and binds it
     * to target UDP port.
     *
     * @param port UDP listening port.
     *
     * @return true Success.
     * @return false Failed to initialize socket.
     */
    bool begin(uint16_t port);

    /**
     * @brief Stop UDP runtime.
     *
     * @details
     * Closes socket safely and
     * releases runtime resources.
     */
    void stop();

    // =====================================================
    // Transmission
    // =====================================================

    /**
     * @brief Send UDP packet.
     *
     * @details
     * Sends binary payload to target:
     * - IP address
     * - UDP port
     *
     * @param ip Target IP address.
     * @param port Target UDP port.
     * @param data Payload buffer.
     * @param len Payload size.
     *
     * @return int Bytes sent.
     */
    int send(const char *ip,
             uint16_t port,
             const uint8_t *data,
             size_t len);

    // =====================================================
    // Broadcast Support
    // =====================================================

    /**
     * @brief Enable or disable UDP broadcast.
     *
     * @details
     * Allows sending packets to:
     * 255.255.255.255
     *
     * @param enable True to enable broadcast.
     */
    void setBroadcast(bool enable = false);

    // =====================================================
    // Multicast Support
    // =====================================================

    /**
     * @brief Join multicast group.
     *
     * @details
     * Enables reception of multicast packets
     * from target multicast address.
     *
     * Example multicast:
     * - 239.x.x.x
     * - 224.x.x.x
     *
     * @param multicastIP Multicast group IP.
     *
     * @return true Success.
     * @return false Failed to join group.
     */
    bool joinMulticast(const char *multicastIP);

    // =====================================================
    // Callback Registration
    // =====================================================

    /**
     * @brief Register UDP packet callback.
     *
     * @details
     * Called whenever incoming UDP packet
     * is received by runtime engine.
     *
     * @param cb Packet callback function.
     */
    void setCallback(UDPPacketCallback cb)
    {
        _onPacket = cb;
    }

    // =====================================================
    // Runtime Engine
    // =====================================================

    /**
     * @brief Internal UDP polling runtime.
     *
     * @details
     * Polls socket for incoming packets
     * and dispatches callback events.
     *
     * Intended usage:
     * - loop()
     * - FreeRTOS task
     * - Runtime scheduler
     *
     * Runtime flow:
     * Socket Receive
     * ↓
     * Packet Parse
     * ↓
     * Callback Dispatch
     */
    void _run();
};