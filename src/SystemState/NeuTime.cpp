#include "SystemState/NeuTime.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#if defined(ESP_PLATFORM) && !defined(ARDUINO)
#include "esp_timer.h"
#endif

uint32_t NeuTime::now()
{
#if defined(ARDUINO)
    return millis();

#elif defined(ESP_PLATFORM)
    return (uint32_t)(esp_timer_get_time() / 1000);

#else
    return 0;
#endif
}