#include "udp/_NeuUDP.h"
#include "SystemState/SharedState.h"
#include <string.h>
#include <freertos/task.h>
#include <lwip/ip_addr.h>

NeuUDP::NeuUDP() : _conn(nullptr),
                   _port(0),
                   _lastError(0),
                   _running(false),
                   _socketActive(false),
                   _blocking(true),
                   _taskStack(4096),
                   _recvTimeoutMs(300), // Short timeout for atomic flag check interrupt
                   _onPacket(nullptr),
                   _user(nullptr),
                   _onPacketRaw(nullptr),
                   _userRaw(nullptr)
{
    memset(&_lastSender, 0, sizeof(_lastSender));
    _mutex = xSemaphoreCreateMutex();
}

NeuUDP::~NeuUDP()
{
    stop();
    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

// =====================================================
// INIT: IMMUNITY TO MESSY INITIALIZATION SEQUENCE
// =====================================================
bool NeuUDP::init(uint16_t port)
{
    if (_running && _port == port)
    {
        return true;
    }

    if (_mutex == nullptr || !xSemaphoreTake(_mutex, portMAX_DELAY))
        return false;

    _port = port;
    _running = true;
    _socketActive = false;
    xSemaphoreGive(_mutex);

    // Directly create a FreeRTOS task in the background
    if (xTaskCreate(_taskStub, "neu_udp", _taskStack, this, 5, nullptr) != pdPASS)
    {
        _running = false;
        return false;
    }

    return true;
}

void NeuUDP::stop()
{
    if (!_running)
        return;

    _running = false;

    // Force close the socket now to have netconn_recv unblock immediately.
    if (_mutex != nullptr && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
    {
        _closeSocket();
        xSemaphoreGive(_mutex);
    }

    // Wait for the FreeRTOS task scheduler to clear the _run() loop.
    int timeout = 100;
    while (_socketActive && timeout > 0)
    {
        vTaskDelay(pdMS_TO_TICKS(5));
        timeout--;
    }

    _socketActive = false;
}

// =====================================================
// LWIP SOCKET INTERNAL MANAGEMENT FUNCTION
// =====================================================
bool NeuUDP::_initSocket()
{
    _conn = netconn_new(NETCONN_UDP);
    if (!_conn)
    {
        _lastError = ERR_MEM;
        return false;
    }

    netconn_set_recvtimeout(_conn, _recvTimeoutMs);
    err_t err = netconn_bind(_conn, IP_ADDR_ANY, _port);

    if (err != ERR_OK)
    {
        _lastError = err;
        netconn_delete(_conn);
        _conn = nullptr;
        return false;
    }

    _socketActive = true;

    SharedState::getInstance().setUdpActive(true);
    SharedState::getInstance().setUdpPort(_port);

    return true;
}

void NeuUDP::_closeSocket()
{
    if (_conn != nullptr)
    {
        struct netconn *tmp = _conn;
        _conn = nullptr;
        netconn_close(tmp);
        netconn_delete(tmp);
    }
    _socketActive = false;

    SharedState::getInstance().setUdpActive(false);
    SharedState::getInstance().setUdpPort(0);
}

// =====================================================
// MUTEX LAYER SAFE PATH CONFIGURATION
// =====================================================
void NeuUDP::setBlocking(bool blocking)
{
    if (_mutex == nullptr || !xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)))
        return;
    _blocking = blocking;
    _recvTimeoutMs = blocking ? 300 : 1; // Keep the minimum timeout limit for unblock loop
    if (_conn)
    {
        netconn_set_recvtimeout(_conn, _recvTimeoutMs);
    }
    xSemaphoreGive(_mutex);
}

void NeuUDP::setTimeout(uint32_t ms)
{
    if (_mutex == nullptr || !xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)))
        return;
    _recvTimeoutMs = ms;
    if (_conn)
    {
        netconn_set_recvtimeout(_conn, _recvTimeoutMs);
    }
    xSemaphoreGive(_mutex);
}

// =====================================================
// DATA TRANSFER FUNCTION (THREAD-SAFE MAIN PATH)
// =====================================================
int NeuUDP::sendTo(const struct sockaddr_in *dest, const uint8_t *data, size_t len)
{
    if (_mutex == nullptr || !xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
        return -1;

    if (!_socketActive || !_conn)
    {
        xSemaphoreGive(_mutex);
        return -1;
    }

    struct netbuf *buf = netbuf_new();
    if (!buf)
    {
        xSemaphoreGive(_mutex);
        return -1;
    }

    netbuf_ref(buf, data, len);

    ip_addr_t dest_ip;
    inet_addr_to_ip4addr(ip_2_ip4(&dest_ip), &dest->sin_addr);
    uint16_t dest_port = ntohs(dest->sin_port);

    err_t err = netconn_sendto(_conn, buf, &dest_ip, dest_port);
    netbuf_delete(buf);

    xSemaphoreGive(_mutex);

    if (err != ERR_OK)
    {
        _lastError = err;
        return -1;
    }
    return len;
}

int NeuUDP::send(const char *ip, uint16_t port, const uint8_t *data, size_t len)
{
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = ipaddr_addr(ip);
    dest.sin_port = htons(port);
    return sendTo(&dest, data, len);
}

int NeuUDP::reply(const uint8_t *data, size_t len)
{
    struct sockaddr_in local_sender;

    if (_mutex == nullptr || !xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)))
        return -1;
    local_sender = _lastSender;
    xSemaphoreGive(_mutex);

    if (local_sender.sin_port == 0)
        return -1;

    return sendTo(&local_sender, data, len);
}

// =====================================================
// MULTICAST LAYER
// =====================================================
bool NeuUDP::joinMulticast(const char *multicastIP)
{
    if (_mutex == nullptr || !xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)))
        return false;

    if (!_socketActive || !_conn)
    {
        xSemaphoreGive(_mutex);
        return false;
    }

    ip_addr_t multi_addr;
    if (!ipaddr_aton(multicastIP, &multi_addr))
    {
        xSemaphoreGive(_mutex);
        return false;
    }

    err_t err = netconn_join_leave_group(_conn, &multi_addr, IP_ADDR_ANY, NETCONN_JOIN);
    if (err != ERR_OK)
        _lastError = err;

    xSemaphoreGive(_mutex);
    return err == ERR_OK;
}

bool NeuUDP::leaveMulticast(const char *multicastIP)
{
    if (_mutex == nullptr || !xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)))
        return false;

    if (!_socketActive || !_conn)
    {
        xSemaphoreGive(_mutex);
        return false;
    }

    ip_addr_t multi_addr;
    if (!ipaddr_aton(multicastIP, &multi_addr))
    {
        xSemaphoreGive(_mutex);
        return false;
    }

    err_t err = netconn_join_leave_group(_conn, &multi_addr, IP_ADDR_ANY, NETCONN_LEAVE);
    if (err != ERR_OK)
        _lastError = err;

    xSemaphoreGive(_mutex);
    return err == ERR_OK;
}

// =====================================================
// MAIN ENGINE: INDEPENDENT ATOMIC STATUS MONITOR
// =====================================================
void NeuUDP::_run()
{
    struct netbuf *buf = nullptr;

    bool lastStaState = false;
    bool lastApState = false;
    bool isFirstCheck = true;

    _recvTimeoutMs = 300; // Interrupts per 300ms

    while (_running)
    {
        // Direct atomic flag access without callbacks
        bool currentSta = SharedState::getInstance().isStaReady();
        bool currentAp = SharedState::getInstance().isApReady();
        bool netReady = currentSta || currentAp;

        // Route turbulence detection due to shifts/breaks in Hybrid Mode
        bool networkChanged = (currentSta != lastStaState) || (currentAp != lastApState);

        if (isFirstCheck)
        {
            networkChanged = false;
            isFirstCheck = false;
        }

        lastStaState = currentSta;
        lastApState = currentAp;

        // 1. SELF-HEALING: Turbulent network in Hybrid mode OR total shutdown
        if ((_socketActive && networkChanged) || (!netReady && _socketActive))
        {
            if (_mutex != nullptr && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)))
            {
                _closeSocket();
                xSemaphoreGive(_mutex);
            }
        }

        // 2. RECONNECT: The network is ready to serve and the socket status is down.
        if (netReady && !_socketActive)
        {
            if (_mutex != nullptr && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)))
            {
                _initSocket();
                xSemaphoreGive(_mutex);
            }
        }

        // 3. SLEEP MODE: If Wi-Fi is not on during initial boot, put the task to sleep for 500ms.
        if (!_socketActive)
        {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // --- GET SOCKET REFERENCE FOR DATA RECEIPT ---
        if (_mutex == nullptr || !xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)))
        {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        struct netconn *current_conn = _conn;
        xSemaphoreGive(_mutex);

        if (current_conn == nullptr)
            continue;

        // Periodic blocking interrupt (Max 300ms) to jump up to check SharedState
        err_t err = netconn_recv(current_conn, &buf);

        if (!_running)
        {
            if (buf != nullptr)
                netbuf_delete(buf);
            break;
        }

        if (err == ERR_OK && buf != nullptr)
        {
            void *data_ptr = nullptr;
            uint16_t data_len = 0;
            netbuf_data(buf, &data_ptr, &data_len);

            if (data_len > 0 && _running && _socketActive)
            {
                struct sockaddr_in src;
                memset(&src, 0, sizeof(src));
                src.sin_family = AF_INET;
                src.sin_port = htons(netbuf_fromport(buf));
                src.sin_addr.s_addr = ip_2_ip4(netbuf_fromaddr(buf))->addr;

                if (_mutex != nullptr && xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)))
                {
                    _lastSender = src;
                    xSemaphoreGive(_mutex);
                }

                if (_onPacketRaw)
                    _onPacketRaw((const uint8_t *)data_ptr, data_len, &src, _userRaw);
                if (_onPacket)
                {
                    char ip_str[16];
                    ipaddr_ntoa_r(netbuf_fromaddr(buf), ip_str, sizeof(ip_str));
                    _onPacket((const uint8_t *)data_ptr, data_len, ip_str, ntohs(src.sin_port), _user);
                }
            }
            netbuf_delete(buf);
            buf = nullptr;
        }
        else
        {
            if (buf != nullptr)
            {
                netbuf_delete(buf);
                buf = nullptr;
            }
            _lastError = err;

            // If it's a pure timeout interrupt, let the loop jump up to check the atomic flag
            if (err == ERR_TIMEOUT)
            {
                continue;
            }

            if (_mutex != nullptr && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)))
            {
                _closeSocket();
                xSemaphoreGive(_mutex);
            }
        }
    }

    if (_mutex != nullptr && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
    {
        _closeSocket();
        xSemaphoreGive(_mutex);
    }

    vTaskDelete(NULL);
}
