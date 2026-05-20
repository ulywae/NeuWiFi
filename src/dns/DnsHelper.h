#ifndef DNS_HELPER_H
#define DNS_HELPER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    size_t buildDNSRedirect(uint8_t *response, size_t maxResponseLen, const uint8_t *query, size_t queryLen, const char *redirectIP);
    const uint8_t *parseDomainName(const uint8_t *data, const uint8_t *end, char *out, size_t outSize);
    size_t buildDNS_NXDOMAIN(uint8_t *buf, size_t maxLen, const uint8_t *query, size_t query_len);

#ifdef __cplusplus
}
#endif

#endif
