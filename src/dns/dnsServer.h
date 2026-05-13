/**
 * @file dnsServer.h
 * @author Ulywae
 * @brief Lightweight DNS server for ESP32 captive portal systems.
 * @version 1.0.0
 * @date 2026
 *
 * @details
 * NeuDnsServer is a lightweight UDP-based
 * DNS responder designed primarily for:
 * - Captive portals
 * - Local AP environments
 * - Offline configuration systems
 * - Embedded network onboarding
 *
 * Features:
 * - Lightweight DNS responder
 * - Captive portal redirection
 * - UDP socket-based runtime
 * - FreeRTOS task architecture
 * - Embedded-safe packet parsing
 *
 * Design goals:
 * - Minimal memory overhead
 * - Deterministic runtime behavior
 * - Simple captive portal integration
 * - Lightweight packet handling
 * - ESP32-only optimization
 *
 * Typical usage:
 * All DNS requests
 * ↓
 * Redirect to local AP IP
 * ↓
 * Captive portal opens automatically
 */

#pragma once

#include <stdint.h>
#include "../NeuWiFiConfig.h"

// =====================================================
// Platform Validation
// =====================================================

#if !defined(ESP32)

/**
 * @brief Platform protection.
 *
 * NeuDnsServer only supports:
 * - ESP32
 * - ESP-IDF runtime
 */
#error "Platform not supported! NeuDnsServer currently only supports ESP32."

#endif

/**
 * @class NeuDnsServer
 * @brief Lightweight DNS responder for captive portals.
 *
 * @details
 * Provides:
 * - UDP DNS listening
 * - DNS packet parsing
 * - DNS response generation
 * - Captive portal support
 *
 * Runtime architecture:
 * UDP Socket
 * ↓
 * FreeRTOS DNS Task
 * ↓
 * DNS Packet Parse
 * ↓
 * DNS Response
 *
 * Intended usage:
 * - WiFi AP onboarding
 * - Device configuration portal
 * - Local network redirection
 */
class NeuDnsServer
{
public:
    // =====================================================
    // Constructor / Destructor
    // =====================================================

    /**
     * @brief Construct DNS server runtime.
     */
    NeuDnsServer();

    /**
     * @brief Destroy DNS server runtime.
     *
     * @details
     * Ensures socket cleanup and
     * graceful runtime shutdown.
     */
    ~NeuDnsServer();

    // =====================================================
    // Lifecycle
    // =====================================================

    /**
     * @brief Start DNS server task.
     *
     * @details
     * Creates:
     * - UDP DNS socket
     * - Background FreeRTOS task
     *
     * Default DNS port:
     * 53
     *
     * Runtime flow:
     * start()
     * ↓
     * UDP Bind
     * ↓
     * DNS Task
     * ↓
     * Request Handling
     *
     * @param port DNS server port.
     *
     * @return true Successfully started.
     * @return false Failed to initialize.
     */
    bool start(int port = 53);

    /**
     * @brief Stop DNS runtime safely.
     *
     * @details
     * Closes socket and terminates
     * DNS listener runtime.
     */
    void stop();

private:
    // =====================================================
    // Internal Runtime State
    // =====================================================

    /**
     * @brief Internal UDP socket handle.
     */
    int _socket_fd = -1;

    // =====================================================
    // Internal DNS Runtime Task
    // =====================================================

    /**
     * @brief Internal DNS listener task.
     *
     * @details
     * Handles:
     * - Incoming DNS packets
     * - DNS query parsing
     * - DNS response generation
     * - Captive portal redirects
     *
     * @param pvParameters Pointer to runtime instance.
     */
    static void dnsTask(void *pvParameters);

    // =====================================================
    // DNS Packet Header
    // =====================================================

    /**
     * @struct DNSHeader
     * @brief Lightweight DNS packet header.
     *
     * @details
     * Represents standard DNS protocol
     * packet header structure.
     *
     * Uses packed alignment to ensure:
     * - Exact protocol layout
     * - Embedded-safe parsing
     * - Network byte compatibility
     */
    struct DNSHeader
    {
        /** @brief Transaction ID. */
        uint16_t id;

        /** @brief DNS flags field. */
        uint16_t flags;

        /** @brief Question count. */
        uint16_t qd_count;

        /** @brief Answer count. */
        uint16_t an_count;

        /** @brief Authority record count. */
        uint16_t ns_count;

        /** @brief Additional record count. */
        uint16_t ar_count;

    } __attribute__((packed));
};