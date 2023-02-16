#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "stm32g0xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status led connection configuration
 *
 * Status led green pin is connected to PB3 and
 * red pin is connected to PA10
 */
#define STATUS_LED_GREEN_PIN  GPIO_PIN_3
#define STATUS_LED_GREEN_PORT GPIOB
#define STATUS_LED_RED_PIN    GPIO_PIN_10
#define STATUS_LED_RED_PORT   GPIOA

/**
 * @brief The states the status led can be in
 */
typedef enum
{
    STATUS_LED_OFF,      /**< Led is turned off */
    STATUS_LED_GREEN,    /**< Led is turned on and the color is green */
    STATUS_LED_RED       /**< Led is turned on and the color is red */
} status_led_state;

/**
 * @brief Set the state of the status led
 * @param state The state of the status led
 */
void status_led_set_state(status_led_state state);

/**
 * @brief Get the status led state
 * @return The status led state
 */
status_led_state status_led_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* STATUS_LED_H */
