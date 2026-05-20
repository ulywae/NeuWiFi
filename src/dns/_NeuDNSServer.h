/**
 * @file _NeuDNSServer.h
 * @brief Lightweight deterministic DNS server for embedded WiFi environments.
 *
 * NeuDNSServer is designed for captive portal systems, local DNS routing,
 * probe interception, and embedded network orchestration on ESP-class devices.
 *
 * Features:
 * - UDP-based DNS listener
 * - Lightweight runtime footprint
 * - Deterministic callback dispatch
 * - Captive portal compatible
 * - User-visible DNS query debug callback
 * - ESP32 / Arduino compatible
 *
 * Typical use cases:
 * - Captive portal DNS redirect
 * - Local AP DNS resolver
 * - Probe request monitoring
 * - DNS blackhole / sinkhole logic
 * - Embedded network testing
 *
 * Architecture Notes:
 * - Uses NeuUDP as transport backend
 * - Static dispatcher isolates unsafe void* casts
 * - Runtime designed for low-overhead embedded loops
 *
 * @author Ulywae
 * @version 1.1.0
 */

#ifndef _NEU_DNS_SERVER_H
#define _NEU_DNS_SERVER_H

#include "udp/_NeuUDP.h"
#include <stdint.h>
#include <stddef.h>

#if defined(ESP_PLATFORM) || defined(ARDUINO)
#include <lwip/sockets.h>
#else
#include <arpa/inet.h>
#endif

/**
 * @class NeuDNSServer
 * @brief Deterministic lightweight DNS server engine.
 *
 * This class provides a small-footprint DNS server implementation
 * intended for embedded systems and WiFi access point environments.
 *
 * The server listens on UDP port 53 and processes incoming DNS queries.
 * It can be used for:
 * - captive portals
 * - DNS redirect systems
 * - local DNS override
 * - probe interception
 * - custom embedded networking experiments
 *
 * Threading:
 * - Internally uses a dedicated task for DNS initialization/runtime.
 *
 * Safety:
 * - Unsafe pointer casting is isolated inside the static dispatcher.
 * - Public API remains strongly typed.
 */
class NeuDNSServer
{
public:
    NeuDNSServer();
    ~NeuDNSServer();

    /**
     * @brief Initialize and start the DNS server.
     *
     * Creates the internal UDP listener and starts the DNS runtime task.
     *
     * @param port UDP port to bind the DNS server to.
     *             Default is 53.
     *
     * @param taskStack Stack size for internal runtime task.
     *                  Default is 4096 bytes.
     *
     * @return true  DNS server started successfully.
     * @return false Initialization failed.
     */
    bool init(uint16_t port = 53, uint32_t taskStack = 4096);

    /**
     * @brief Stop the DNS server.
     *
     * Closes UDP transport and destroys runtime task resources.
     */
    void stop();

    /**
     * @brief Check whether the DNS server is currently running.
     *
     * @return true  DNS server active.
     * @return false DNS server stopped.
     */
    bool isRunning() const;

    /**
     * @brief Alias for init().
     *
     * Arduino-style compatibility wrapper.
     *
     * @param port UDP port.
     * @param taskStack Internal task stack size.
     * @return true Success.
     * @return false Failed.
     */
    inline bool begin(uint16_t port = 53, uint32_t taskStack = 4096)
    {
        return init(port, taskStack);
    }

    // =====================================================
    // PUBLIC DEBUG API (USER VISIBLE)
    // =====================================================

    /**
     * @brief Register DNS query debug callback.
     *
     * Called whenever a DNS query is received.
     * Useful for:
     * - monitoring probe requests
     * - logging DNS activity
     * - captive portal debugging
     * - telemetry systems
     *
     * @param callback Function called on incoming DNS queries.
     *
     * Callback parameters:
     * - domain  : requested domain name
     * - srcIP   : source client IP address
     * - srcPort : source client UDP port
     */
    void onQuery(void (*callback)(const char *domain, const char *srcIP, uint16_t srcPort))
    {
        _onQuery = callback;
    }

private:
    // =====================================================
    // STATIC DISPATCHER (ONLY PLACE WITH void* CAST)
    // =====================================================

    /**
     * @brief Internal UDP packet dispatcher.
     *
     * Static bridge callback used by NeuUDP transport layer.
     * This is intentionally the only location where raw void*
     * user-pointer casting is performed.
     *
     * @param data Raw UDP payload.
     * @param len Payload length.
     * @param src Source socket address.
     * @param user User context pointer.
     */
    static void _udpCallbackRaw(
        const uint8_t *data,
        size_t len,
        const struct sockaddr_in *src,
        void *user);

    NeuUDP _udp;

    void (*_onQuery)(const char *, const char *, uint16_t);

    bool _dnsEnabled;
    uint16_t _dns_port;
    uint32_t _dns_stack;
};

#endif // _NEU_DNS_SERVER_H
