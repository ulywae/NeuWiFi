/**
 * @file NeuDNSServer.cpp
 * @brief Implementation of NeuDNSServer deterministic DNS runtime.
 *
 * This module implements:
 * - DNS runtime task lifecycle
 * - Captive probe interception
 * - Redirect / blackhole DNS logic
 * - UDP packet dispatch
 * - Embedded DNS orchestration
 *
 * Runtime Model:
 * - Standby while AP inactive
 * - Auto-start UDP listener when AP becomes ready
 * - Auto-stop UDP listener when AP shuts down
 * - Persistent lightweight monitoring task
 *
 * DNS Pipeline:
 * 1. Parse incoming query
 * 2. Optional debug callback
 * 3. Captive probe detection
 * 4. Redirect engine
 * 5. NXDOMAIN blackhole fallback
 *
 * Design Goals:
 * - Deterministic behavior
 * - Minimal heap churn
 * - Persistent runtime stability
 * - Safe task lifecycle
 * - Lightweight embedded footprint
 */

#include "dns/_NeuDNSServer.h"
#include "SystemState/SharedState.h"
#include "dns/DNSProbe.h"
#include "dns/DnsHelper.h"

#if defined(ESP_PLATFORM) || defined(ARDUINO)
#include <lwip/def.h>
#endif

#define DNS_HEADER_SIZE 12

NeuDNSServer::NeuDNSServer() : _onQuery(nullptr), _dnsEnabled(false)
{
    _dns_stack = 3072;
    _dns_port = 53;
}

NeuDNSServer::~NeuDNSServer()
{
    stop();
}

// =====================================================
// PUBLIC API
// =====================================================

/**
 * @brief Initialize DNS runtime system.
 *
 * Creates the persistent DNS runtime task responsible for:
 * - AP monitoring
 * - UDP lifecycle management
 * - DNS runtime orchestration
 *
 * The UDP listener itself is only started once
 * the Access Point becomes ready.
 *
 * @param port DNS UDP port.
 * @param taskStack Internal UDP runtime stack size.
 *
 * @return true Task created successfully.
 * @return false Failed to create task.
 */
bool NeuDNSServer::init(uint16_t port, uint32_t taskStack)
{
    _dns_port = port;
    _dns_stack = taskStack;
    _dnsEnabled = true;

    // 1. Configure DNS's internal NeuUDP
    _udp.setTaskStackSize(_dns_stack);
    _udp.onPacketRaw(_udpCallbackRaw, this);

    // 2. Directly call UDP's init().
    // Because NeuUDP is equipped with an "Event Bridge", it will automatically
    // sleep when AP is off, and wake up processing data ONLY when AP is on!
    if (!_udp.init(_dns_port))
    {
        _dnsEnabled = false;
        SharedState::getInstance().setDnsActive(false);
        return false;
    }

    SharedState::getInstance().setDnsActive(true);
    return true;
}

/**
 * @brief Stop DNS runtime system safely.
 *
 * Performs:
 * - runtime shutdown signal
 * - UDP transport stop
 * - shared state cleanup
 * - task synchronization
 * - emergency task deletion fallback
 */
void NeuDNSServer::stop()
{
    _dnsEnabled = false;
    _udp.stop(); // Cleanly kills internal UDP tasks
    _onQuery = nullptr;
    SharedState::getInstance().setDnsActive(false);
}

/**
 * @brief Check whether DNS UDP transport is active.
 *
 * @return true UDP listener active.
 * @return false UDP listener inactive.
 */
bool NeuDNSServer::isRunning() const
{
    // DNS liveness status now purely follows UDP hardware socket readiness
    return _udp.isRunning();
}

// =====================================================
// UDP PACKET DISPATCH
// =====================================================

/**
 * @brief Raw UDP packet dispatcher.
 *
 * DNS Processing Pipeline:
 *
 * STEP 1:
 * - Validate packet
 * - Parse domain
 * - Optional debug callback
 *
 * STEP 2:
 * - Captive probe detection
 * - Redirect enforcement
 *
 * STEP 3:
 * - NXDOMAIN blackhole fallback
 *
 * Redirect conditions:
 * - Known captive probe domains
 * - Client flagged for redirect enforcement
 *
 * Blackhole behavior:
 * - Non-approved DNS queries receive NXDOMAIN
 *
 * @param data Raw UDP payload.
 * @param len Payload length.
 * @param src Source client socket address.
 * @param user User context pointer.
 */
void NeuDNSServer::_udpCallbackRaw(const uint8_t *data, size_t len, const struct sockaddr_in *src, void *user)
{
    NeuDNSServer *self = (NeuDNSServer *)user;
    if (!self || !src || !data || !self->_dnsEnabled)
        return;

    uint32_t client_ip = src->sin_addr.s_addr;
    char domain[256] = {0};

    if (!parseDomainName(data + DNS_HEADER_SIZE, data + len, domain, sizeof(domain)))
        return;

    auto &state = SharedState::getInstance();
    const char *active_target_ip = state.getApIP();

    // =====================================================
    // STEP 1: ENGINE ANTI-PRIVATE DNS REGISTRATION
    // =====================================================
    if (self->_onQuery)
    {
        char srcIP[16];
        uint8_t *ipBytes = (uint8_t *)&(src->sin_addr.s_addr);
        snprintf(srcIP, sizeof(srcIP), "%d.%d.%d.%d", ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]);
        self->_onQuery(domain, srcIP, ntohs(src->sin_port));
    }

    // =====================================================
    // STEP 2: AGRESIF REDIRECT ENGINE (Captive Portal)
    // =====================================================
    if (DNSProbe::isCaptiveProbe(domain) || state.isClientRedirectRequired(client_ip))
    {
        uint8_t response[512];
        size_t respLen = buildDNSRedirect(response, sizeof(response), data, len, active_target_ip);
        if (respLen > 0)
        {
            self->_udp.reply(response, respLen);
        }
        return;
    }

    // =====================================================
    // STEP 3: BLACKHOLE (NXDOMAIN)
    // =====================================================
    uint8_t response[512];
    size_t respLen = buildDNS_NXDOMAIN(response, sizeof(response), data, len);
    if (respLen > 0)
    {
        self->_udp.reply(response, respLen);
    }
}
