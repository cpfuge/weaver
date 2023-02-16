#ifndef SOFTWARE_TIMER_H
#define SOFTWARE_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32g0xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Macro for software timer definition
 * @param name: Timer name
 * @param size: Timer expire interval in ms
 */
#define SOFTWARE_TIMER_DEF(name, expire) \
    static software_timer name = {       \
        .start = 0,                      \
        .expire_interval = expire        \
    }

/**
 * @brief Software timer structure type
 */
typedef struct
{
    uint32_t start;                    /**< Timer start counter */
    const uint32_t expire_interval;    /**< Timer expiration interval in ms */
} software_timer;

/**
 * @brief Start software timer
 * @param timer: Pointer to software timer structure
 */
void timer_start(software_timer *timer);

/**
 * @brief Check if software timer is expired
 * @param timer: Pointer to software timer structure
 * @return: True if timer is expired, false otherwise
 */
bool timer_is_expired(software_timer *timer);

#ifdef __cplusplus
}
#endif

#endif /* SOFTWARE_TIMER_H */
