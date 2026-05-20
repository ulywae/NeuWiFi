#ifndef NEU_MDNS_TXT_H
#define NEU_MDNS_TXT_H

#include <string.h>
#include <stddef.h>

/**
 * @def NEU_MDNS_MAX_TXT_ITEMS
 * @brief Maximum number of key-value pairs allowed in one TXT record.
 */
#define NEU_MDNS_MAX_TXT_ITEMS 4

/**
 * @def NEU_MDNS_TXT_KEY_LEN
 * @brief Maximum length (in bytes) for a TXT record key name (including null terminator).
 */
#define NEU_MDNS_TXT_KEY_LEN 16

/**
 * @def NEU_MDNS_TXT_VAL_LEN
 * @brief Maximum length (in bytes) for a TXT record value string (including null terminator).
 */
#define NEU_MDNS_TXT_VAL_LEN 32

/**
 * @struct NeuTXTItem
 * @brief Structure representing a single key-value entry within an mDNS TXT record.
 *
 * Stores a descriptive attribute for a published service, following the standard
 * format `key=value` used in mDNS/DNS-SD advertisements.
 */
struct NeuTXTItem
{
    char key[NEU_MDNS_TXT_KEY_LEN];   ///< Attribute name (e.g., "version", "type")
    char value[NEU_MDNS_TXT_VAL_LEN]; ///< Attribute value associated with the key
};

/**
 * @class NeuMDNSTXT
 * @brief Container class for managing and building TXT record data for mDNS services.
 *
 * This class provides a simple, fixed-size storage mechanism for service metadata.
 * It ensures data fits within protocol limits and handles safe string copying.
 * TXT records are used to provide additional information about a service
 * (like version numbers, capabilities, or status) during discovery.
 *
 * @note This implementation uses static memory allocation only, making it suitable
 * for embedded systems with strict memory requirements.
 */
class NeuMDNSTXT
{
public:
    size_t count;                             ///< Number of valid items currently stored in the list
    NeuTXTItem items[NEU_MDNS_MAX_TXT_ITEMS]; ///< Array holding all TXT key-value pairs

    /**
     * @brief Constructor: Initializes an empty TXT record list.
     *
     * Resets the item counter and ensures all internal strings are null-terminated
     * and ready for use.
     */
    NeuMDNSTXT() : count(0)
    {
        for (int i = 0; i < NEU_MDNS_MAX_TXT_ITEMS; i++)
        {
            items[i].key[0] = '\0';
            items[i].value[0] = '\0';
        }
    }

    /**
     * @brief Adds a new key-value pair to the TXT record list.
     *
     * Copies the provided strings safely into internal buffers, ensuring
     * null-termination and respecting maximum length limits.
     *
     * @param key Null-terminated string representing the attribute name.
     *            Must be shorter than NEU_MDNS_TXT_KEY_LEN.
     * @param value Null-terminated string representing the attribute value.
     *              Must be shorter than NEU_MDNS_TXT_VAL_LEN.
     * @return true Item was added successfully.
     * @return false Failed to add: list is full, or one of the input pointers is NULL.
     */
    bool add(const char *key, const char *value)
    {
        if (count >= NEU_MDNS_MAX_TXT_ITEMS || !key || !value)
        {
            return false;
        }

        strncpy(items[count].key, key, NEU_MDNS_TXT_KEY_LEN - 1);
        items[count].key[NEU_MDNS_TXT_KEY_LEN - 1] = '\0';

        strncpy(items[count].value, value, NEU_MDNS_TXT_VAL_LEN - 1);
        items[count].value[NEU_MDNS_TXT_VAL_LEN - 1] = '\0';

        count++;
        return true;
    }
};

#endif // NEU_MDNS_TXT_H