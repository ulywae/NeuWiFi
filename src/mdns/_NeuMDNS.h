/**
 * @file _NeuMDNS.h
 * @brief mDNS (Multicast DNS) service publisher and discovery library
 * @details Provides functions to announce network services and discover
 *          other devices/services on the local network using mDNS protocol.
 *          Supports custom TXT records, callback-based service
 *          discovery, and multiple output formats for address information.
 */

#ifndef _NEU_MDNS_H
#define _NEU_MDNS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "NeuWiFiConfig.h"
#include "NeuWiFiTypes.h"
#include "mdns/NeuMDNSTXT.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

/**
 * @typedef neu_mdns_discovery_cb_t
 * @brief Callback function type invoked when a new service is discovered
 * @param hostname Host name of the discovered device
 * @param ip_address IP address of the device as string
 * @param txt Reference to NeuMDNSTXT object containing additional metadata
 */
typedef void (*neu_mdns_discovery_cb_t)(const char *hostname, const char *ip_address, const NeuMDNSTXT &txt);

#ifdef __cplusplus

/**
 * @def _NEU_MDNS_QUEUE_LEN
 * @brief Maximum number of discovered devices kept in internal queue
 */
#define _NEU_MDNS_QUEUE_LEN NEU_MDNS_QUEUE_LEN

/**
 * @def _NEU_MDNS_TEXT_MAX_LEN
 * @brief Maximum allowed length for TXT record text data
 */
#define _NEU_MDNS_TEXT_MAX_LEN NEU_MDNS_TEXT_MAX_LEN

/**
 * @def _NEU_MDNS_HOST_MAX_LEN
 * @brief Maximum length limit for host name strings
 */
#define _NEU_MDNS_HOST_MAX_LEN NEU_MDNS_HOST_MAX_LEN

/**
 * @def _NEU_MDNS_INSTANCE_MAX_LEN
 * @brief Maximum length limit for service instance name strings
 */
#define _NEU_MDNS_INSTANCE_MAX_LEN NEU_MDNS_INSTANCE_MAX_LEN

/**
 * @def _NEU_MDNS_SERVICE_MAX_LEN
 * @brief Maximum length limit for service type name strings
 */
#define _NEU_MDNS_SERVICE_MAX_LEN NEU_MDNS_SERVICE_MAX_LEN

/**
 * @class NeuMDNSClass
 * @brief mDNS management class for service publication and discovery
 * @details Encapsulates mDNS responder and query functions. Allows publishing
 *          services on the network, searching for services, and retrieving
 *          results via callback or polling methods in multiple formats.
 */
class NeuMDNSClass
{
public:
    /**
     * @brief Constructor: Initializes internal state and variables
     */
    NeuMDNSClass();

    /**
     * @brief Initialize mDNS responder with host and instance name
     * @param hostname Unique host name used for this device on the network
     * @param instance_name Human-readable name for this service instance (default: NEU_MDNS_DEFAULT_INSTANCE)
     */
    void init(const char *hostname, const char *instance_name = NEU_MDNS_DEFAULT_INSTANCE);

    /**
     * @brief Register and announce a new service via mDNS
     * @param service_name Service type identifier (e.g. "_http")
     * @param proto Transport protocol used ("_tcp" or "_udp")
     * @param port Port number where the service listens
     * @param txt Optional TXT record with additional key-value metadata
     * @return True if service was added successfully; false otherwise
     */
    bool addService(const char *service_name, const char *proto, uint16_t port, const NeuMDNSTXT &txt = NeuMDNSTXT());

    /**
     * @brief Stop mDNS responder and release all allocated resources
     * @return True on success; false if not initialized or error occurs
     */
    bool end();

    /**
     * @brief Alias for init()
     * @param hostname Unique host name used for this device
     * @param instance_name Human-readable name for this service instance
     * @see init()
     */
    inline void begin(const char *hostname, const char *instance_name = NEU_MDNS_DEFAULT_INSTANCE)
    {
        init(hostname, instance_name);
    }

    /**
     * @brief Start continuous discovery of specified service type
     * @param service_name Service type to search for (e.g. "_http")
     * @param proto Transport protocol to match ("_tcp" or "_udp")
     * @param timeout_ms Timeout for each query cycle in milliseconds (default: 2000)
     * @param interval_ms Interval between consecutive discovery cycles in milliseconds (default: 5000)
     * @return True if discovery started successfully; false otherwise
     */
    bool startDiscovery(const char *service_name, const char *proto,
                        uint32_t timeout_ms = 2000, uint32_t interval_ms = 5000);

    /**
     * @brief Stop ongoing service discovery process
     * @return True if stopped successfully; false if not running
     */
    bool stopDiscovery();

    /**
     * @brief Set callback function to receive discovered services
     * @param callback Function pointer matching neu_mdns_discovery_cb_t signature
     */
    void setCallback(neu_mdns_discovery_cb_t callback);

    /**
     * @brief Check if there are unread discovered devices in queue
     * @return True if new entries are available to read; false otherwise
     */
    bool available();

    /**
     * @brief Read IP address of the first entry in queue as 32-bit integer (little-endian)
     * @return IP address value, or 0 if queue is empty
     */
    uint32_t readIP();

    /**
     * @brief Read IP address and store into IPConfig structure
     * @param out_config Reference to IPConfig object to receive data
     * @return True if data read successfully; false if queue empty
     */
    bool readIP(IPConfig &out_config);

    /**
     * @brief Read IP address as human-readable dot-separated string
     * @param out_str Pointer to character buffer to store the string
     * @return True if data read successfully; false if queue empty
     */
    bool readIP(char *out_str);

    /**
     * @brief Read port number of the first entry in queue
     * @return Port number value, or 0 if queue is empty
     */
    uint16_t readPort();

    /**
     * @brief Read full device information including IPConfig structure
     * @param out_hostname Buffer to store discovered host name
     * @param host_max_len Maximum length of hostname buffer
     * @param out_config IPConfig structure to receive address data
     * @param out_txt NeuMDNSTXT object to receive metadata records
     * @return True if data read successfully; false if queue empty
     */
    bool readDevice(char *out_hostname, size_t host_max_len, IPConfig &out_config, NeuMDNSTXT &out_txt);

    /**
     * @brief Read full device information including port number and IPConfig
     * @param out_hostname Buffer to store discovered host name
     * @param host_max_len Maximum length of hostname buffer
     * @param out_config IPConfig structure to receive address data
     * @param out_port Variable to store service port number
     * @param out_txt NeuMDNSTXT object to receive metadata records
     * @return True if data read successfully; false if queue empty
     */
    bool readDevice(char *out_hostname, size_t host_max_len, IPConfig &out_config, uint16_t &out_port, NeuMDNSTXT &out_txt);

    /**
     * @brief Read device information with IP as string
     * @param out_hostname Buffer to store discovered host name
     * @param host_max_len Maximum length of hostname buffer
     * @param out_ip_str Buffer to store IP address string
     * @param out_txt NeuMDNSTXT object to receive metadata records
     * @return True if data read successfully; false if queue empty
     */
    bool readDevice(char *out_hostname, size_t host_max_len, char *out_ip_str, NeuMDNSTXT &out_txt);

    /**
     * @brief Read device information with IP string and port number
     * @param out_hostname Buffer to store discovered host name
     * @param host_max_len Maximum length of hostname buffer
     * @param out_ip_str Buffer to store IP address string
     * @param out_port Variable to store service port number
     * @param out_txt NeuMDNSTXT object to receive metadata records
     * @return True if data read successfully; false if queue empty
     */
    bool readDevice(char *out_hostname, size_t host_max_len, char *out_ip_str, uint16_t &out_port, NeuMDNSTXT &out_txt);

private:
    /**
     * @brief Initialization state flag
     * @details True if mDNS responder is running and configured
     */
    bool _is_initialized;
};

/**
 * @var NeuMDNS
 * @brief Global instance of NeuMDNSClass for application-wide use
 */
extern NeuMDNSClass NeuMDNS;

#endif // __cplusplus

#endif // _NEU_MDNS_H