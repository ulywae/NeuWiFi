#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <atomic>
#include <stdint.h>
#include <stddef.h>

#include "NeuWiFiConfig.h"

#define MAX_TRACKED_CLIENTS NEU_MAX_TRACKED_CLIENTS

class SharedState
{
public:
    enum class NetMode : uint8_t
    {
        AP,
        STA
    };
    enum class Event : uint8_t
    {
        NONE,
        STA_READY,
        STA_DOWN,
        AP_READY,
        AP_DOWN,
        DNS_ON,
        DNS_OFF,
        MODE_CHANGED
    };

#ifdef _USE_CLIENT_TRACKER
    struct ClientBinding
    {
        uint32_t ip;
        uint8_t mac[6];
        bool is_allocated;
    };
#endif

    typedef void (*EventCallback)(Event e);
    static SharedState &getInstance();

    SharedState(const SharedState &) = delete;
    void operator=(const SharedState &) = delete;

    // MODE & STATE FUNCTIONS
    NetMode getMode() const;
    bool isAPMode() const;
    bool isSTAMode() const;
    void setStaReady(bool ready);
    void setApReady(bool ready);
    bool isStaReady() const;
    bool isApReady() const;
    bool isNetworkReady() const;

    // DNS & IP FUNCTIONS
    void setDnsActive(bool active);
    bool isDnsActive() const;
    bool isAggressiveDNS() const { return _mode.load(std::memory_order_relaxed) == NetMode::AP; }
    void setApIP(const char *ip);
    void setStaIP(const char *ip);
    const char *getApIP() const;
    const char *getStaIP() const;
    const char *getServerIP() const;

    void markClientAsNewAPUser(uint32_t client_ip, const uint8_t *client_mac);
    bool isClientRedirectRequired(uint32_t client_ip) const;
    void clearClientFromTracker(uint32_t client_ip);

    // MANAGEMENT STATUS UDP
    void setUdpActive(bool active) { _isUdpActive.store(active, std::memory_order_release); }
    bool isUdpActive() const { return _isUdpActive.load(std::memory_order_acquire); }

    void setUdpPort(uint16_t port) { _udpPort.store(port, std::memory_order_relaxed); }
    uint16_t getUdpPort() const { return _udpPort.load(std::memory_order_relaxed); }

    void onEvent(EventCallback cb);

private:
    SharedState();
    void emit(Event e);
    void updateAutoMode();

#ifdef _USE_CLIENT_TRACKER
    void _lock()
    {
        while (_lock_buffer.exchange(true, std::memory_order_acquire))
            ;
    }
    void _unlock() { _lock_buffer.store(false, std::memory_order_release); }
#endif

private:
    std::atomic<bool> _isStaReady{false};
    std::atomic<bool> _isApReady{false};
    std::atomic<bool> _isDnsActive{false};
    std::atomic<NetMode> _mode{NetMode::AP};
    std::atomic<uint32_t> _lastChangeMs{0};

    std::atomic<bool> _isUdpActive{false};
    std::atomic<uint16_t> _udpPort{0};

    char _active_ap_ip[16];
    char _active_sta_ip[16];

    EventCallback _cb_list[4];
    size_t _cb_count;

    // =====================================================
    // ARRAY MEMORY IS ONLY ALLOCATED IF THE MACRO IS ACTIVE
    // =====================================================
#ifdef _USE_CLIENT_TRACKER
    ClientBinding _tracked_clients[MAX_TRACKED_CLIENTS];
    std::atomic<size_t> _client_buffer_index{0};
    std::atomic<bool> _lock_buffer{false};
#endif
};

#endif
