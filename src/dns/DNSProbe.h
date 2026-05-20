#ifndef DNS_PROBE_H
#define DNS_PROBE_H

#include <stdint.h>
#include <string.h>

static const char *neu_strcasestr(const char *haystack, const char *needle)
{
    if (!haystack || !needle || !*needle)
        return nullptr;

    size_t needle_len = strlen(needle);

    for (const char *p = haystack; *p; p++)
    {
        if (strncasecmp(p, needle, needle_len) == 0)
            return p;
    }
    return nullptr;
}

namespace DNSProbe
{
    static constexpr const char *const ALL_PROBES[] = {
        "neu.portal", 
        "android",
        "apple",
        "connectivitycheck",
        "ctest",
        "detectportal",
        "facebook",
        "google",
        "googleapis",
        "gstatic",
        "microsoft",
        "msftconnecttest",
        "msftncsi",
        "samsung",
        "whatsapp"};

    static constexpr size_t PROBE_COUNT = sizeof(ALL_PROBES) / sizeof(ALL_PROBES[0]);

    bool isCaptiveProbe(const char *domain)
    {
        if (!domain || !domain[0])
            return false;

        for (size_t i = 0; i < PROBE_COUNT; i++)
        {
            if (neu_strcasestr(domain, ALL_PROBES[i]))
                return true;
        }
        return false;
    }
}

#endif // DNS_PROBE_H
