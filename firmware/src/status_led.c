#include "status_led.h"

/**
 * @brief Status led state
 */
static status_led_state led_state = STATUS_LED_OFF;

void status_led_set_state(status_led_state state)
{
    led_state = state;

    switch (led_state)
    {
    case STATUS_LED_GREEN:
        HAL_GPIO_WritePin(STATUS_LED_GREEN_PORT, STATUS_LED_GREEN_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(STATUS_LED_RED_PORT, STATUS_LED_RED_PIN, GPIO_PIN_RESET);
        break;

    case STATUS_LED_RED:
        HAL_GPIO_WritePin(STATUS_LED_GREEN_PORT, STATUS_LED_GREEN_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(STATUS_LED_RED_PORT, STATUS_LED_RED_PIN, GPIO_PIN_SET);
        break;

    default:
        HAL_GPIO_WritePin(STATUS_LED_GREEN_PORT, STATUS_LED_GREEN_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(STATUS_LED_RED_PORT, STATUS_LED_RED_PIN, GPIO_PIN_RESET);
        break;
    }
}

status_led_state status_led_get_state(void)
{
    return led_state;
}
