#include "main.h"
#include "ccs811.h"
#include "bme280.h"
#include "wifi.h"
#include "pc_uart.h"
#include "status_led.h"
#include "software_timer.h"

/**
 * @brief Check measurement every 5 seconds
 */
SOFTWARE_TIMER_DEF(measurement_timer, 5000);

/**
 * @brief Blink status led
 */
SOFTWARE_TIMER_DEF(led_blink_timer, 500);

/**f
 * @brief I2C handles
 */
I2C_HandleTypeDef hi2c1;

/**
 * @brief UART handles
 */
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/**
 * @brief Sensors
 */
ccs811_device ccs811_dev;
bme280_device bme280_dev;

/**
 * Environmental and air quality data
 */
bme280_measurements environmental_data;
ccs811_measurements air_quality;

/**
 * @brief Private function prototypes
 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_CCS811_Init(void);
static void MX_BME280_Init(void);
static void MX_WiFi_Init(void);

/**
 * @brief The application entry point.
 */
int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    /* Initialize PC communication */
    pc_uart_init(&huart2);

    /* Initialize senzors */
    MX_CCS811_Init();
    MX_BME280_Init();

    /* Start WiFi */
    MX_WiFi_Init();

    /* System initialized */
    status_led_set_state(STATUS_LED_GREEN);
    timer_start(&led_blink_timer);

    timer_start(&measurement_timer);

    while (1)
    {
        if (timer_is_expired(&measurement_timer))
        {
            if (!bme280_is_measuring(&bme280_dev))
            {
                bme280_read_measurements(&bme280_dev, &environmental_data);
                ccs811_set_environmental_data(&ccs811_dev, environmental_data.humidity,
                        environmental_data.temperature);
            }

            if (ccs811_data_available(&ccs811_dev))
            {
                ccs811_read_measurements(&ccs811_dev, &air_quality);
            }

            timer_start(&measurement_timer);
        }
        
        wifi_handler();
        pc_uart_handler();

        if (timer_is_expired(&led_blink_timer))
        {
            if (status_led_get_state() == STATUS_LED_OFF)
            {
                status_led_set_state(STATUS_LED_GREEN);
            }
            else
            {
                status_led_set_state(STATUS_LED_OFF);
            }
            timer_start(&led_blink_timer);
        }
    }
}

/**
 * @brief System Clock Configuration
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

    /* Configure the main internal regulator output voltage */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Initializes the RCC Oscillators according to the specified parameters
    in the RCC_OscInitTypeDef structure. */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }

    /* Initializes the peripherals clocks */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_I2C1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief GPIO initialization
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Reset GPIOA port pins */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_9
        | GPIO_PIN_10, GPIO_PIN_RESET);

    /* Reset GPIOB port pins */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3 | GPIO_PIN_5, GPIO_PIN_RESET);

    /* Configure GPIO pins : PA5, PA6, PA8, PA9, PA10 */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure GPIO pins : PB3 PB5 */
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @brief I2C1 initialization
 */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00303D5B;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure Analogue filter */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure Digital filter */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief USART1 initialization
 */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief USART2 initialization
 */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief CCS811 sensor initialization
 */
static void MX_CCS811_Init(void)
{
    ccs811_dev.i2c_handle = &hi2c1;
    ccs811_dev.i2c_address = CCS811_DEFAULT_ADDRESS;
    ccs811_dev.hardware_id = 0;
    ccs811_dev.hardware_version = 0;
    ccs811_dev.drive_mode = CCS811_DRIVE_1SEC;
    ccs811_dev.env_data.humidity = 0.0f;
    ccs811_dev.env_data.temperature = 0.0f;

    if (!ccs811_init(&ccs811_dev))
    {
        Error_Handler();
    }
}

/**
 * @brief BME280 sensor initialization
 */
static void MX_BME280_Init(void)
{
    bme280_dev.i2c_handle = &hi2c1;
    bme280_dev.i2c_address = BME280_DEFAULT_ADDRESS;
    bme280_dev.mode = BMP280_MODE_NORMAL;
    bme280_dev.hardware_id = 0;
    bme280_dev.filter = BMP280_FILTER_4;
    bme280_dev.standby_duration = BME280_STANDBY_MS_250;
    bme280_dev.pressure_sampling = BME280_SAMPLING_X4;
    bme280_dev.temperature_sampling = BME280_SAMPLING_X4;
    bme280_dev.humidity_sampling = BME280_SAMPLING_X4;

    if (!bme280_init(&bme280_dev))
    {
        Error_Handler();
    }
}

/**
 * @brief WiFi initialization
 */
static void MX_WiFi_Init(void)
{
    if (!wifi_init(&huart1))
    {
        Error_Handler();
    }
}

/**
 * @brief This function is executed in case of error occurrence.
 */
void Error_Handler(void)
{
    status_led_set_state(STATUS_LED_RED);

    __disable_irq();
    while (1)
    {
        /* Wait here */
    }
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief Reports the name of the source file and the source line number
 *        where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    LOG("Wrong parameters value: file %s on line %d\r\n", file, line);
}
#endif /* USE_FULL_ASSERT */
