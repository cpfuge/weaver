#include "main.h"
#include "wifi.h"
#include "pc_uart.h"
#include "stm32g0xx_it.h"

/**
 * I2C handles
 */
extern I2C_HandleTypeDef hi2c1;

/**
 * UART Handles
 */
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void)
{
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void)
{
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/**
 * @brief STM32G0xx Peripheral Interrupt Handlers
 *
 * Add here the Interrupt Handlers for the used peripherals.
 * For the available peripheral interrupt handler names,
 * please refer to the startup file (startup_stm32g0xx.s).
 */

/**
 * @brief This function handles I2C1 event global interrupt
 *        I2C1 wake-up interrupt through EXTI line 23.
 */
void I2C1_IRQHandler(void)
{
    if (hi2c1.Instance->ISR & (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR))
    {
        HAL_I2C_ER_IRQHandler(&hi2c1);
    }
    else
    {
        HAL_I2C_EV_IRQHandler(&hi2c1);
    }
}

/**
 * @brief This function handles USART1 global interrupt
 *        USART1 wake-up interrupt through EXTI line 25.
 */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

/**
 * @brief This function handles USART2 global interrupt
 *        USART2 wake-up interrupt through EXTI line 26.
 */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

/**
 * @brief Rx Transfer completed callback
 * @param huart: UART handle that received data
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    wifi_rx_callback(huart);
    pc_uart_rx_callback(huart);
}
