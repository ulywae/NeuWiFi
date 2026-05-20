#ifndef IPCONFIG_H
#define IPCONFIG_H

#include <cstdint>
#include <cstdio>

/**
 * @class IPConfig
 * @brief A helper class to manage and manipulate IPv4 addresses and network configurations efficiently.
 *
 * This class provides a convenient and safe way to store, convert, and handle IPv4 related data
 * including IP address, Subnet Mask, Gateway, and DNS servers. It supports conversion between
 * string format, byte arrays, and raw 32-bit integer format. It also automatically handles
 * endianness correction required for compatibility with the LwIP library.
 *
 * All values are stored internally as 32-bit unsigned integers for maximum speed and memory efficiency.
 */
class IPConfig
{
private:
    uint32_t _ip;       ///< Internal storage for the IP address (raw 32-bit value)
    uint32_t _subnet;   ///< Internal storage for the Subnet Mask (raw 32-bit value)
    uint32_t _gateway;  ///< Internal storage for the Gateway address (raw 32-bit value)
    uint32_t _dns_prim; ///< Internal storage for the Primary DNS server (raw 32-bit value)
    uint32_t _dns_sec;  ///< Internal storage for the Secondary DNS server (raw 32-bit value)

    /**
     * @brief Parses a human-readable IPv4 string into a raw 32-bit integer.
     *
     * Validates the format and ensures each segment value is within the valid range (0-255).
     *
     * @param ip_str Null-terminated string containing the IP address (e.g., "192.168.4.1").
     * @return uint32_t The parsed IP address as a 32-bit integer, or 0 if the input is invalid.
     */
    uint32_t parseString(const char *ip_str)
    {
        if (!ip_str)
            return 0;
        uint32_t ip_terbaca = 0;
        int segmen = 0;
        uint32_t nilai_segmen = 0;

        while (*ip_str)
        {
            char c = *ip_str++;
            if (c >= '0' && c <= '9')
            {
                nilai_segmen = (nilai_segmen * 10) + (c - '0');
                if (nilai_segmen > 255)
                    return 0;
            }
            else if (c == '.')
            {
                if (segmen > 2)
                    return 0;
                ip_terbaca |= (nilai_segmen << (segmen * 8));
                nilai_segmen = 0;
                segmen++;
            }
            else
            {
                return 0;
            }
        }
        if (segmen != 3)
            return 0;
        ip_terbaca |= (nilai_segmen << (segmen * 8));
        return ip_terbaca;
    }

    /**
     * @brief Converts 4 separate octets into a single 32-bit integer.
     *
     * @param b1 First octet of the address.
     * @param b2 Second octet of the address.
     * @param b3 Third octet of the address.
     * @param b4 Fourth octet of the address.
     * @return uint32_t Combined 32-bit representation.
     */
    uint32_t parseBytes(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
    {
        return (uint32_t)b1 | ((uint32_t)b2 << 8) | ((uint32_t)b3 << 16) | ((uint32_t)b4 << 24);
    }

    /**
     * @brief Converts a 4-byte array into a single 32-bit integer.
     *
     * @param arr Pointer to a 4-byte array containing the address parts.
     * @return uint32_t Combined 32-bit representation, returns 0 if pointer is NULL.
     */
    uint32_t parseArray(const uint8_t *arr)
    {
        if (!arr)
            return 0;
        return (uint32_t)arr[0] | ((uint32_t)arr[1] << 8) | ((uint32_t)arr[2] << 16) | ((uint32_t)arr[3] << 24);
    }

    /**
     * @brief Exports a raw 32-bit IP value into a 4-byte array.
     *
     * @param raw_ip The raw 32-bit IP value to export.
     * @param out_arr Pointer to the output buffer (must be at least 4 bytes long).
     */
    void exportToBytes(uint32_t raw_ip, uint8_t *out_arr) const
    {
        if (!out_arr)
            return;
        out_arr[0] = raw_ip & 0xFF;
        out_arr[1] = (raw_ip >> 8) & 0xFF;
        out_arr[2] = (raw_ip >> 16) & 0xFF;
        out_arr[3] = (raw_ip >> 24) & 0xFF;
    }

    /**
     * @brief Converts a raw 32-bit IP value into a human-readable string format.
     *
     * @param raw_ip The raw 32-bit IP value to convert.
     * @param out_str Pointer to the output string buffer (minimum 16 bytes recommended).
     */
    void exportToString(uint32_t raw_ip, char *out_str) const
    {
        if (!out_str)
            return;
        std::sprintf(out_str, "%d.%d.%d.%d",
                     raw_ip & 0xFF,
                     (raw_ip >> 8) & 0xFF,
                     (raw_ip >> 16) & 0xFF,
                     (raw_ip >> 24) & 0xFF);
    }

public:
    /**
     * @brief Default constructor. Initializes all values to 0.0.0.0.
     */
    IPConfig() : _ip(0), _subnet(0), _gateway(0), _dns_prim(0), _dns_sec(0) {}

    /**
     * @brief Constructor that initializes the complete network configuration from strings.
     *
     * @param ip IP address string (e.g., "192.168.4.1").
     * @param subnet Subnet mask string (e.g., "255.255.255.0").
     * @param gateway Gateway address string.
     * @param dns_p Primary DNS server string. Defaults to "8.8.8.8".
     * @param dns_s Secondary DNS server string. Defaults to "8.8.4.4".
     */
    IPConfig(const char *ip, const char *subnet, const char *gateway, const char *dns_p = "8.8.8.8", const char *dns_s = "8.8.4.4")
    {
        _ip = parseString(ip);
        _subnet = parseString(subnet);
        _gateway = parseString(gateway);
        _dns_prim = parseString(dns_p);
        _dns_sec = parseString(dns_s);
    }

    // --- IP Address Configuration ---

    /**
     * @brief Sets the IP address from a string.
     * @param str IP address string (e.g., "192.168.1.10").
     */
    void setIP(const char *str) { _ip = parseString(str); }

    /**
     * @brief Sets the IP address from 4 separate octets.
     * @param b1 First octet.
     * @param b2 Second octet.
     * @param b3 Third octet.
     * @param b4 Fourth octet.
     */
    void setIP(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) { _ip = parseBytes(b1, b2, b3, b4); }

    /**
     * @brief Sets the IP address from a byte array.
     * @param arr Pointer to a 4-byte array representing the IP.
     */
    void setIP(const uint8_t *arr) { _ip = parseArray(arr); }

    // --- Subnet Mask Configuration ---

    /**
     * @brief Sets the Subnet Mask from a string.
     * @param str Subnet mask string (e.g., "255.255.255.0").
     */
    void setSubnet(const char *str) { _subnet = parseString(str); }

    /**
     * @brief Sets the Subnet Mask from 4 separate octets.
     * @param b1 First octet.
     * @param b2 Second octet.
     * @param b3 Third octet.
     * @param b4 Fourth octet.
     */
    void setSubnet(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) { _subnet = parseBytes(b1, b2, b3, b4); }

    /**
     * @brief Sets the Subnet Mask from a byte array.
     * @param arr Pointer to a 4-byte array representing the subnet mask.
     */
    void setSubnet(const uint8_t *arr) { _subnet = parseArray(arr); }

    // --- Gateway Configuration ---

    /**
     * @brief Sets the Gateway address from a string.
     * @param str Gateway address string.
     */
    void setGateway(const char *str) { _gateway = parseString(str); }

    /**
     * @brief Sets the Gateway address from 4 separate octets.
     * @param b1 First octet.
     * @param b2 Second octet.
     * @param b3 Third octet.
     * @param b4 Fourth octet.
     */
    void setGateway(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) { _gateway = parseBytes(b1, b2, b3, b4); }

    /**
     * @brief Sets the Gateway address from a byte array.
     * @param arr Pointer to a 4-byte array representing the gateway.
     */
    void setGateway(const uint8_t *arr) { _gateway = parseArray(arr); }

    // --- Primary DNS Configuration ---

    /**
     * @brief Sets the Primary DNS server address from a string.
     * @param str DNS address string.
     */
    void setDNSPrim(const char *str) { _dns_prim = parseString(str); }

    /**
     * @brief Sets the Primary DNS server address from 4 separate octets.
     * @param b1 First octet.
     * @param b2 Second octet.
     * @param b3 Third octet.
     * @param b4 Fourth octet.
     */
    void setDNSPrim(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) { _dns_prim = parseBytes(b1, b2, b3, b4); }

    /**
     * @brief Sets the Primary DNS server address from a byte array.
     * @param arr Pointer to a 4-byte array representing the DNS address.
     */
    void setDNSPrim(const uint8_t *arr) { _dns_prim = parseArray(arr); }

    // --- Secondary DNS Configuration ---

    /**
     * @brief Sets the Secondary DNS server address from a string.
     * @param str DNS address string.
     */
    void setDNSSec(const char *str) { _dns_sec = parseString(str); }

    /**
     * @brief Sets the Secondary DNS server address from 4 separate octets.
     * @param b1 First octet.
     * @param b2 Second octet.
     * @param b3 Third octet.
     * @param b4 Fourth octet.
     */
    void setDNSSec(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) { _dns_sec = parseBytes(b1, b2, b3, b4); }

    /**
     * @brief Sets the Secondary DNS server address from a byte array.
     * @param arr Pointer to a 4-byte array representing the DNS address.
     */
    void setDNSSec(const uint8_t *arr) { _dns_sec = parseArray(arr); }

    // --- Getters for Raw Values ---

    /** @return Raw 32-bit value of the stored IP address. */
    uint32_t getIP() const { return _ip; }

    /** @return Raw 32-bit value of the stored Subnet Mask. */

    uint32_t getSubnet() const { return _subnet; }

    /** @return Raw 32-bit value of the stored Gateway address. */
    uint32_t getGateway() const { return _gateway; }

    /** @return Raw 32-bit value of the stored Primary DNS address. */
    uint32_t getDNSPrim() const { return _dns_prim; }

    /** @return Raw 32-bit value of the stored Secondary DNS address. */
    uint32_t getDNSSec() const { return _dns_sec; }

    /**
     * @brief Converts internal IP storage format to LwIP compatible byte order.
     *
     * LwIP library expects IPs in Big-Endian format, while internal storage uses Little-Endian.
     * This function performs the necessary byte-swapping automatically.
     *
     * @param raw_ip The raw IP value to convert.
     * @return uint32_t IP value formatted correctly for LwIP API calls.
     */
    uint32_t getLwIP(uint32_t raw_ip) const
    {
        return ((raw_ip & 0xFF) << 24) |
               ((raw_ip >> 8) & 0xFF) << 16 |
               ((raw_ip >> 16) & 0xFF) << 8 |
               ((raw_ip >> 24) & 0xFF);
    }

    /** @return Primary DNS formatted for LwIP. Returns 1.1.1.1 if unset. */
    uint32_t getLwIP_DNSPrim() const
    {
        if (_dns_prim == 0)
            return (1U << 24) | (1U << 16) | (1U << 8) | 1U; // Default: 1.1.1.1
        return getLwIP(_dns_prim);
    }

    /** @return Secondary DNS formatted for LwIP. Returns 8.8.8.8 if unset. */
    uint32_t getLwIP_DNSSec() const
    {
        if (_dns_sec == 0)
            return (8U << 24) | (8U << 16) | (8U << 8) | 8U; // Default: 8.8.8.8
        return getLwIP(_dns_sec);
    }

    /** @return IP address formatted for LwIP library. */
    uint32_t getLwIP_IP() const { return getLwIP(_ip); }
    /** @return Subnet Mask formatted for LwIP library. */
    uint32_t getLwIP_Subnet() const { return getLwIP(_subnet); }
    /** @return Gateway address formatted for LwIP library. */
    uint32_t getLwIP_Gateway() const { return getLwIP(_gateway); }

    /**
     * @brief Validates if the Gateway is within the same subnet as the IP address.
     *
     * @return true If the configuration is logically valid and routable.
     * @return false If the subnet is zero or gateway is in a different network segment.
     */
    bool isValidSubnet() const
    {
        if (_subnet == 0)
            return false;
        return ((_ip & _subnet) == (_gateway & _subnet));
    }

    // --- Export to Byte Array ---

    /**
     * @brief Exports the stored IP address to a 4-byte array.
     * @param out Pointer to the output buffer (must be >= 4 bytes).
     */
    void asBytesIP(uint8_t *out) const { exportToBytes(_ip, out); }

    /**
     * @brief Exports the stored Subnet Mask to a 4-byte array.
     * @param out Pointer to the output buffer (must be >= 4 bytes).
     */
    void asBytesSubnet(uint8_t *out) const { exportToBytes(_subnet, out); }

    /**
     * @brief Exports the stored Gateway address to a 4-byte array.
     * @param out Pointer to the output buffer (must be >= 4 bytes).
     */
    void asBytesGateway(uint8_t *out) const { exportToBytes(_gateway, out); }

    /**
     * @brief Exports the stored Primary DNS address to a 4-byte array.
     * @param out Pointer to the output buffer (must be >= 4 bytes).
     */
    void asBytesDNSPrim(uint8_t *out) const { exportToBytes(_dns_prim, out); }

    /**
     * @brief Exports the stored Secondary DNS address to a 4-byte array.
     * @param out Pointer to the output buffer (must be >= 4 bytes).
     */
    void asBytesDNSSec(uint8_t *out) const { exportToBytes(_dns_sec, out); }

    // --- Export to String ---

    /**
     * @brief Exports the stored IP address to a string.
     * @param out Pointer to the output buffer (must be >= 16 bytes).
     */
    void asStringIP(char *out) const { exportToString(_ip, out); }

    /**
     * @brief Exports the stored Subnet Mask to a string.
     * @param out Pointer to the output buffer (must be >= 16 bytes).
     */
    void asStringSubnet(char *out) const { exportToString(_subnet, out); }

    /**
     * @brief Exports the stored Gateway address to a string.
     * @param out Pointer to the output buffer (must be >= 16 bytes).
     */
    void asStringGateway(char *out) const { exportToString(_gateway, out); }

    /**
     * @brief Exports the stored Primary DNS address to a string.
     * @param out Pointer to the output buffer (must be >= 16 bytes).
     */
    void asStringDNSPrim(char *out) const { exportToString(_dns_prim, out); }

    /**
     * @brief Exports the stored Secondary DNS address to a string.
     * @param out Pointer to the output buffer (must be >= 16 bytes).
     */
    void asStringDNSSec(char *out) const { exportToString(_dns_sec, out); }
};

#endif
