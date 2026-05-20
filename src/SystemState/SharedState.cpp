#include "SystemState/SharedState.h"
#include "SystemState/NeuTime.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <string.h>

// =====================================================
// SINGLETON INISED
// =====================================================
SharedState &SharedState::getInstance()
{
    static SharedState instance;
    return instance;
}

// =====================================================
// CONSTRUCTOR
// =====================================================
SharedState::SharedState()
{
    _mode.store(NetMode::AP, std::memory_order_relaxed);

    // Initialize static callback array list to empty (nullptr)
    _cb_count = 0;
    for (size_t i = 0; i < 4; ++i)
    {
        _cb_list[i] = nullptr;
    }

    strncpy(_active_ap_ip, "192.168.4.1", sizeof(_active_ap_ip) - 1);
    _active_ap_ip[15] = '\0';
    strncpy(_active_sta_ip, "0.0.0.0", sizeof(_active_sta_ip) - 1);
    _active_sta_ip[15] = '\0';

#ifdef _USE_CLIENT_TRACKER
    _client_buffer_index.store(0, std::memory_order_relaxed);
    _lock_buffer.store(false, std::memory_order_relaxed);
    for (size_t i = 0; i < MAX_TRACKED_CLIENTS; ++i)
    {
        _tracked_clients[i].ip = 0;
        memset(_tracked_clients[i].mac, 0, 6);
        _tracked_clients[i].is_allocated = false;
    }
#endif
}

// =====================================================
// EVENT TRANSMITTER (MULTI-CALLBACK STATIS LAYER)
// =====================================================
void SharedState::onEvent(EventCallback cb)
{
    if (cb == nullptr)
        return;

    // Prevent duplication if the same callback function is entered twice
    for (size_t i = 0; i < _cb_count; ++i)
    {
        if (_cb_list[i] == cb)
            return;
    }

    // Secure into a static queue slot (Max 4 registrants: mDNS, Wi-Fi, etc)
    if (_cb_count < 4)
    {
        _cb_list[_cb_count++] = cb;
    }
}

void SharedState::emit(Event e)
{
    // Distribute event signals in parallel to all listeners fairly
    for (size_t i = 0; i < _cb_count; ++i)
    {
        if (_cb_list[i] != nullptr)
        {
            _cb_list[i](e);
        }
    }
}

// =====================================================
// MANAGEMENT MODE
// =====================================================
SharedState::NetMode SharedState::getMode() const
{
    return _mode.load(std::memory_order_relaxed);
}

bool SharedState::isAPMode() const
{
    return getMode() == NetMode::AP;
}

bool SharedState::isSTAMode() const
{
    return getMode() == NetMode::STA;
}

// =====================================================
// AUTO MODE CONTROLLER (Automatic Switch)
// =====================================================
void SharedState::updateAutoMode()
{
    bool sta = _isStaReady.load(std::memory_order_relaxed);
    bool ap = _isApReady.load(std::memory_order_relaxed);

    uint32_t now = NeuTime::now();

    // Debouncing protection for mode transitions (300ms)
    if (now - _lastChangeMs.load(std::memory_order_relaxed) < 300)
        return;

    NetMode old = _mode.load(std::memory_order_relaxed);
    NetMode next = NetMode::AP;

    if (sta)
        next = NetMode::STA;
    else if (ap)
        next = NetMode::AP;

    if (old != next)
    {
        _mode.store(next, std::memory_order_relaxed);
        emit(Event::MODE_CHANGED);
    }

    _lastChangeMs.store(now, std::memory_order_relaxed);
}

// =====================================================
// STATION STATUS (STA)
// =====================================================
void SharedState::setStaReady(bool ready)
{
    bool old = _isStaReady.load(std::memory_order_acquire);
    _isStaReady.store(ready, std::memory_order_release);

    if (old != ready)
        emit(ready ? Event::STA_READY : Event::STA_DOWN);

    _lastChangeMs.store(NeuTime::now(), std::memory_order_relaxed);
    updateAutoMode();
}

bool SharedState::isStaReady() const
{
    return _isStaReady.load(std::memory_order_acquire);
}

// =====================================================
// ACCESS POINT STATUS (AP)
// =====================================================
void SharedState::setApReady(bool ready)
{
    bool old = _isApReady.load(std::memory_order_acquire);
    _isApReady.store(ready, std::memory_order_release);

    if (old != ready)
        emit(ready ? Event::AP_READY : Event::AP_DOWN);

    _lastChangeMs.store(NeuTime::now(), std::memory_order_relaxed);
    updateAutoMode();
}

bool SharedState::isApReady() const
{
    return _isApReady.load(std::memory_order_acquire);
}

bool SharedState::isNetworkReady() const
{
    return _isStaReady.load(std::memory_order_relaxed) || _isApReady.load(std::memory_order_relaxed);
}

// =====================================================
// DNS STATUS
// =====================================================
void SharedState::setDnsActive(bool active)
{
    bool old = _isDnsActive.load(std::memory_order_acquire);
    _isDnsActive.store(active, std::memory_order_release);

    if (old != active)
        emit(active ? Event::DNS_ON : Event::DNS_OFF);
}

bool SharedState::isDnsActive() const
{
    return _isDnsActive.load(std::memory_order_acquire);
}

// =====================================================
// IP ADDRESS MANAGEMENT
// =====================================================
void SharedState::setApIP(const char *ip)
{
    if (!ip)
        return;
    strncpy(_active_ap_ip, ip, sizeof(_active_ap_ip) - 1);
    _active_ap_ip[15] = '\0';
}

const char *SharedState::getApIP() const
{
    return _active_ap_ip;
}

void SharedState::setStaIP(const char *ip)
{
    if (!ip)
        return;
    strncpy(_active_sta_ip, ip, sizeof(_active_sta_ip) - 1);
    _active_sta_ip[15] = '\0';
}

const char *SharedState::getStaIP() const
{
    return _active_sta_ip;
}

const char *SharedState::getServerIP() const
{
    if (_mode.load(std::memory_order_relaxed) == NetMode::AP)
        return _active_ap_ip;
    else
        return _active_sta_ip;
}

// =====================================================
// IP-MAC BINDING TRACKER ENGINE (SAFE ATOMIC MUTEX)
// =====================================================
void SharedState::markClientAsNewAPUser(uint32_t client_ip, const uint8_t *client_mac)
{
#ifdef _USE_CLIENT_TRACKER
    if (client_ip == 0 || !client_mac)
        return;

    _lock();
    for (size_t i = 0; i < MAX_TRACKED_CLIENTS; ++i)
    {
        if (_tracked_clients[i].is_allocated && _tracked_clients[i].ip == client_ip)
        {
            memcpy(_tracked_clients[i].mac, client_mac, 6);
            _unlock();
            return;
        }
    }

    size_t idx = _client_buffer_index.load(std::memory_order_relaxed);
    _tracked_clients[idx].ip = client_ip;
    memcpy(_tracked_clients[idx].mac, client_mac, 6);
    _tracked_clients[idx].is_allocated = true;

    _client_buffer_index.store((idx + 1) % MAX_TRACKED_CLIENTS, std::memory_order_relaxed);
    _unlock();
#else
    (void)client_ip;
    (void)client_mac;
#endif
}

bool SharedState::isClientRedirectRequired(uint32_t client_ip) const
{
#ifdef _USE_CLIENT_TRACKER
    if (client_ip == 0)
        return false;
    for (size_t i = 0; i < MAX_TRACKED_CLIENTS; ++i)
    {
        if (_tracked_clients[i].is_allocated && _tracked_clients[i].ip == client_ip)
        {
            return true;
        }
    }
    return false;
#else
    (void)client_ip;
    return false;
#endif
}

void SharedState::clearClientFromTracker(uint32_t client_ip)
{
#ifdef _USE_CLIENT_TRACKER
    if (client_ip == 0)
        return;

    _lock();
    for (size_t i = 0; i < MAX_TRACKED_CLIENTS; ++i)
    {
        if (_tracked_clients[i].is_allocated && _tracked_clients[i].ip == client_ip)
        {
            _tracked_clients[i].ip = 0;
            memset(_tracked_clients[i].mac, 0, 6);
            _tracked_clients[i].is_allocated = false;
            _unlock();
            return;
        }
    }
    _unlock();
#else
    (void)client_ip;
#endif
}
