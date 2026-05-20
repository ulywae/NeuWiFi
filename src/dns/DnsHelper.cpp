#include "dns/DnsHelper.h"

#if defined(ESP_PLATFORM) || defined(ARDUINO)
#include <lwip/sockets.h>
#else
#include <arpa/inet.h>
#endif

// Header DNS (12 byte)
typedef struct __attribute__((packed))
{
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} DNSHeader;


const uint8_t *parseDomainName(const uint8_t *data, const uint8_t *end, char *out, size_t outSize)
{
    if (!data || data >= end)
        return nullptr;

    const uint8_t *p = data;
    char *outPtr = out;
    size_t remain = outSize;

    if (out && outSize > 0)
        *out = '\0';

    while (p < end)
    {
        uint8_t len = *p;

        // Detecting DNS compression pointer (0xC0)
        if (len & 0xC0)
        {
            if (p + 2 > end)
                return nullptr;
            return p + 2;
        }

        p++; 
        if (len == 0)
            break;
        if (len > (end - p))
            return nullptr;

        if (outPtr && remain > 0)
        {
            if (len + 1 >= remain)
            { 
                return nullptr;
            }

            memcpy(outPtr, p, len);
            outPtr += len;
            *outPtr++ = '.';
            remain -= (len + 1);
        }
        p += len;
    }

    if (outPtr > out)
    {
        outPtr[-1] = '\0'; 
    }
    return p;
}


size_t buildDNSRedirect(uint8_t *response, size_t maxResponseLen, const uint8_t *query, size_t queryLen, const char *redirectIP)
{
    if (!response || !query || queryLen < sizeof(DNSHeader) || maxResponseLen < queryLen + 16)
        return 0;

    const DNSHeader *qh = (const DNSHeader *)query;
    if (ntohs(qh->qdcount) != 1)
        return 0;

    DNSHeader *rh = (DNSHeader *)response;
    memcpy(response, query, sizeof(DNSHeader));
    rh->flags = htons(0x8180); // Standard Response: QR=1, AA=1, RD=1, RA=1
    rh->ancount = htons(1);    // Giving 1 answer (Answer Count)
    rh->nscount = 0;
    rh->arcount = 0;

    const uint8_t *qPtr = query + sizeof(DNSHeader);
    const uint8_t *qEnd = query + queryLen;
    uint8_t *rPtr = response + sizeof(DNSHeader);
    const uint8_t *rEnd = response + maxResponseLen; // Output buffer end limit

    while (qPtr < qEnd)
    {
        if (rPtr >= rEnd)
            return 0;

        uint8_t len = *qPtr++;
        *rPtr++ = len;

        if (len == 0)
            break;

        // DNS Packet Compression
        if (len & 0xC0)
        {
            if (qPtr >= qEnd || rPtr >= rEnd)
                return 0;
            *rPtr++ = *qPtr++;
            break;
        }

        if (qPtr + len > qEnd || rPtr + len > rEnd)
            return 0;

        memcpy(rPtr, qPtr, len);
        rPtr += len;
        qPtr += len;
    }

    if (qPtr + 4 > qEnd || rPtr + 4 > rEnd)
        return 0;

    uint16_t qtype = (qPtr[0] << 8) | qPtr[1];
    uint16_t qclass = (qPtr[2] << 8) | qPtr[3];

    memcpy(rPtr, qPtr, 4);
    rPtr += 4;
    qPtr += 4;

    // Only responds if the type is IPv4 lookup (A Record = 1, Class IN = 1)
    if (qtype != 1 || qclass != 1)
        return 0;

    if (rPtr + 16 > rEnd)
        return 0;

    // 3. DNS Answer Section Construction
    uint16_t namePtr = htons(0xC00C);
    memcpy(rPtr, &namePtr, 2);
    rPtr += 2;

    uint16_t ansType = htons(1); // Type A (IPv4)
    memcpy(rPtr, &ansType, 2);
    rPtr += 2;

    uint16_t ansClass = htons(1); // Class IN
    memcpy(rPtr, &ansClass, 2);
    rPtr += 2;

    uint32_t ttl = htonl(60); // DNS Cache Duration (TTL) = 60 Seconds
    memcpy(rPtr, &ttl, 4);
    rPtr += 4;

    uint16_t rdlength = htons(4); // IP data length (4 Bytes for IPv4)
    memcpy(rPtr, &rdlength, 2);
    rPtr += 2;

    uint8_t ip_bytes[4] = {0};
    int parts = sscanf(redirectIP, "%hhu.%hhu.%hhu.%hhu", &ip_bytes[0], &ip_bytes[1], &ip_bytes[2], &ip_bytes[3]);
    if (parts != 4)
        return 0;

    memcpy(rPtr, ip_bytes, 4);
    rPtr += 4;

    return (rPtr - response); // Returns the total length of the entire DNS packet.
}


size_t buildDNS_NXDOMAIN(uint8_t *buf, size_t maxLen,
                         const uint8_t *query, size_t query_len)
{
    if (query_len < 12 || maxLen < query_len)
        return 0;

    memcpy(buf, query, query_len);

    // QR=1 (response), AA=1, RCODE=3 (NXDOMAIN)
    buf[2] = 0x81;
    buf[3] = 0x83;

    // ANCOUNT, NSCOUNT, ARCOUNT = 0
    buf[6] = buf[7] = 0;
    buf[8] = buf[9] = 0;
    buf[10] = buf[11] = 0;

    return query_len;
}