/**
 * @file _NeuUDP.h
 * @brief Lightweight UDP communication wrapper built on LwIP and FreeRTOS
 * @details Provides a simple, thread-safe interface for UDP communication,
 *          including socket management, data transmission, multicast support,
 *          and asynchronous packet reception via callback functions. Designed
 *          for embedded systems using LwIP stack and FreeRTOS.
 */

#ifndef _NEU_UDP_H
#define _NEU_UDP_H

#include <stdint.h>
#include <stddef.h>
#include <lwip/api.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @class NeuUDP
 * @brief UDP communication handler with background processing and thread safety
 * @details Encapsulates LwIP socket operations and runs a dedicated FreeRTOS task
 *          to handle incoming data. Supports blocking/non-blocking modes, timeouts,
 *          multicast groups, and two types of reception callbacks: human-readable
 *          and raw address format.
 */
class NeuUDP
{
public:
    /**
     * @brief Constructor: Initializes internal states and resources
     */
    NeuUDP();

    /**
     * @brief Destructor: Ensures proper cleanup of socket and task resources
     */
    ~NeuUDP();

    /**
     * @brief Initialize UDP socket and start background processing task
     * @param port Local UDP port to bind and listen on
     * @return True if initialization and task creation succeed; false otherwise
     * @note Must be called before using send/receive functions
     */
    bool init(uint16_t port);

    /**
     * @brief Stop background task, close active socket, and release resources
     * @details Thread-safe; ensures no operations are performed after stopping
     */
    void stop();

    /**
     * @brief Alias for init()
     * @param port Local UDP port to bind and listen on
     * @return True if initialization succeeds; false otherwise
     * @see init()
     */
    inline bool begin(uint16_t port) { return init(port); }

    /**
     * @brief Check if the underlying LwIP socket is currently active
     * @return True if socket is created and bound; false otherwise
     */
    bool isRunning() const { return _socketActive; }

    /**
     * @brief Set stack size for the internal FreeRTOS task
     * @param bytes Stack size in bytes (default: 4096)
     * @note Must be set before calling init() to take effect
     */
    void setTaskStackSize(uint32_t bytes) { _taskStack = bytes; }

    /**
     * @brief Send data to the last known sender
     * @param data Pointer to data buffer to send
     * @param len Length of data in bytes
     * @return Number of bytes actually sent, or negative error code on failure
     * @note Thread-safe; uses internal mutex for protection
     */
    int reply(const uint8_t *data, size_t len);

    /**
     * @brief Send data to a specified IP address and port
     * @param ip Destination IPv4 address in dot-separated string format
     * @param port Destination UDP port number
     * @param data Pointer to data buffer to send
     * @param len Length of data in bytes
     * @return Number of bytes actually sent, or negative error code on failure
     * @note Thread-safe
     */
    int send(const char *ip, uint16_t port, const uint8_t *data, size_t len);

    /**
     * @brief Send data to a destination defined by raw socket address structure
     * @param dest Pointer to sockaddr_in structure containing destination info
     * @param data Pointer to data buffer to send
     * @param len Length of data in bytes
     * @return Number of bytes actually sent, or negative error code on failure
     * @note Thread-safe; for advanced usage
     */
    int sendTo(const struct sockaddr_in *dest, const uint8_t *data, size_t len);

    /**
     * @brief Set socket blocking mode
     * @param blocking True for blocking operations, false for non-blocking
     * @note Applies to receive operations only
     */
    void setBlocking(bool blocking);

    /**
     * @brief Set receive operation timeout
     * @param ms Timeout value in milliseconds
     * @note Effective only in blocking mode
     */
    void setTimeout(uint32_t ms);

    /**
     * @brief Join a UDP multicast group
     * @param multicastIP Multicast group IPv4 address as string
     * @return True if group joined successfully; false otherwise
     */
    bool joinMulticast(const char *multicastIP);

    /**
     * @brief Leave a previously joined UDP multicast group
     * @param multicastIP Multicast group IPv4 address as string
     * @return True if group left successfully; false otherwise
     */
    bool leaveMulticast(const char *multicastIP);

    /**
     * @brief Get currently bound local UDP port
     * @return Local port number
     */
    uint16_t getLocalPort() const { return _port; }

    /**
     * @brief Retrieve last recorded error code
     * @return Negative error code or 0 if no error occurred
     * @note Codes correspond to standard LwIP error numbers
     */
    int getLastError() const { return _lastError; }

    /**
     * @brief Register callback function for received packets (formatted source info)
     * @param callback Function to call when data arrives; parameters:
     *                 - Received data buffer
     *                 - Length of received data
     *                 - Source IP as string
     *                 - Source port number
     *                 - User-defined context pointer
     * @param user Optional pointer passed unchanged to the callback
     */
    void onPacket(void (*callback)(const uint8_t *data, size_t len, const char *ip, uint16_t port, void *user), void *user = nullptr)
    {
        _onPacket = callback;
        _user = user;
    }

    /**
     * @brief Register callback function for received packets (raw source address)
     * @param callback Function to call when data arrives; parameters:
     *                 - Received data buffer
     *                 - Length of received data
     *                 - Raw source address structure
     *                 - User-defined context pointer
     * @param user Optional pointer passed unchanged to the callback
     * @note Preferred when you need full access to address structure details
     */
    void onPacketRaw(void (*callback)(const uint8_t *data, size_t len, const struct sockaddr_in *src, void *user), void *user = nullptr)
    {
        _onPacketRaw = callback;
        _userRaw = user;
    }

private:
    struct netconn *_conn;          /**< LwIP network connection handle */
    uint16_t _port;                 /**< Local bound UDP port */
    struct sockaddr_in _lastSender; /**< Address of the most recent sender */
    int _lastError;                 /**< Last recorded error code */

    volatile bool _running;      /**< Control flag for background task loop */
    volatile bool _socketActive; /**< Status flag: true if socket is valid and bound */

    bool _blocking;           /**< Current blocking mode setting */
    uint32_t _taskStack;      /**< Stack size allocated for FreeRTOS task */
    uint32_t _recvTimeoutMs;  /**< Receive timeout in milliseconds */
    SemaphoreHandle_t _mutex; /**< Mutex for thread-safe operations */

    void (*_onPacket)(const uint8_t *, size_t, const char *, uint16_t, void *);        /**< Standard callback */
    void *_user;                                                                       /**< Context pointer for standard callback */
    void (*_onPacketRaw)(const uint8_t *, size_t, const struct sockaddr_in *, void *); /**< Raw address callback */
    void *_userRaw;                                                                    /**< Context pointer for raw callback */

    /**
     * @brief Internal task main function: handles receiving and dispatching packets
     */
    void _run();

    /**
     * @brief Static entry point for FreeRTOS task
     * @param p Pointer to NeuUDP instance
     */
    static void _taskStub(void *p) { ((NeuUDP *)p)->_run(); }

    /**
     * @brief Create, configure and bind the UDP socket
     * @return True on success; false on error
     */
    bool _initSocket();

    /**
     * @brief Close socket and release associated resources
     */
    void _closeSocket();
};

#endif // _NEU_UDP_H