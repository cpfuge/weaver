#ifndef PC_UART_H
#define PC_UART_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32g0xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PC size related macros
 */
#define PC_RX_BUFFER_SIZE 512

/**
 * @brief PC device definition
 */
typedef struct
{
    UART_HandleTypeDef *uart_handle;      /**< UART handle connected to PC */
    char rx_buffer[PC_RX_BUFFER_SIZE];    /**< Received data from PC */
    uint16_t rx_index;                    /**< Current index of received data */
} pc_uart;

/**
 * @brief PC communication initialization
 * @param huart: Handle of the UART device connected to the PC
 * @return: true if PC communication was started successfully, false otherwise
 */
bool pc_uart_init(UART_HandleTypeDef *huart);

/**
 * @brief PC communication handler
 */
void pc_uart_handler(void);

/**
 * @brief PC UART rx callback
 * This function is called everytime a byte was received by the UART
 * @param huart: UART handle that received data
 */
void pc_uart_rx_callback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* PC_UART_H */
