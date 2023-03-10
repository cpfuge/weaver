#include "main.h"

/**
 * Initializes the Global MSP.
 */
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral */
    HAL_SYSCFG_StrobeDBattpinsConfig(SYSCFG_CFGR1_UCPD1_STROBE | SYSCFG_CFGR1_UCPD2_STROBE);
}

/**
 * @brief I2C MSP Initialization
 *
 * This function configures the hardware resources used
 *
 * I2C1 GPIO Configuration
 *   PB8 - I2C1_SCL
 *   PB9 - I2C1_SDA
 *
 * @param hi2c: I2C handle pointer
 */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    if (hi2c->Instance == I2C1)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF6_I2C1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_I2C1_CLK_ENABLE();

        /* I2C1 interrupt Init */
        HAL_NVIC_SetPriority(I2C1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(I2C1_IRQn);
    }
}

/**
 * @brief I2C MSP De-Initialization
 *
 * This function freeze the hardware resources used
 *
 * I2C1 GPIO Configuration
 *   PB8 - I2C1_SCL
 *   PB9 - I2C1_SDA
 *
 * @param hi2c: I2C handle pointer
 */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        /* Peripheral clock disable */
        __HAL_RCC_I2C1_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);

        /* I2C1 interrupt DeInit */
        HAL_NVIC_DisableIRQ(I2C1_IRQn);
    }
}

/**
 * @brief UART MSP Initialization
 *
 * This function configures the hardware resources used
 *
 * USART1 GPIO Configuration
 *   PC4 - USART1_TX
 *   PC5 - USART1_RX
 *
 * USART2 GPIO Configuration
 *   PA2 - USART2_TX
 *   PA3 - USART2_RX
 *
 * @param huart: UART handle pointer
 */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    if (huart->Instance == USART1)
    {
        /* Peripheral clock enable */
        __HAL_RCC_USART1_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* USART1 interrupt Init */
        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
    else if (huart->Instance == USART2)
    {
        /* Peripheral clock enable */
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USART2 interrupt Init */
        HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
}

/**
 * @brief UART MSP De-Initialization
 *
 * This function freeze the hardware resources used
 *
 * USART1 GPIO Configuration
 *   PC4 - USART1_TX
 *   PC5 - USART1_RX
 *
 * USART2 GPIO Configuration
 *   PA2 - USART2_TX
 *   PA3 - USART2_RX
 *
 * @param huart: UART handle pointer
*/
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART1)
    {
        /* Peripheral clock disable */
        __HAL_RCC_USART1_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_4 | GPIO_PIN_5);

        /* USART1 interrupt DeInit */
        HAL_NVIC_DisableIRQ(USART1_IRQn);
    }
    else if(huart->Instance == USART2)
    {
        /* Peripheral clock disable */
        __HAL_RCC_USART2_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);

        /* USART2 interrupt DeInit */
        HAL_NVIC_DisableIRQ(USART2_IRQn);
    }
}
