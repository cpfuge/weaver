#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g0xx_hal.h"

/**
 * @brief Error handler function
 */
void Error_Handler(void);

/**
 * @brief NUCLEO-G071RB GPIO configuration
 */
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOF
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
