#ifndef CCS811_H
#define CCS811_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32g0xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers
 */
#define CCS811_STATUS          0x00
#define CCS811_MEASURE_MODE    0x01
#define CCS811_RESULT_DATA     0x02
#define CCS811_RAW_DATA        0x03
#define CCS811_ENV_DATA        0x05
#define CCS811_NTC             0x06
#define CCS811_THRESHOLDS      0x10
#define CCS811_BASELINE        0x11
#define CCS811_HW_ID           0x20
#define CCS811_HW_VERSION      0x21
#define CCS811_FW_BOOT_VERSION 0x23
#define CCS811_FW_APP_VERSION  0x24
#define CCS811_ERROR_ID        0xe0
#define CCS811_APP_START       0xf4
#define CCS811_SOFTWARE_RESET  0xff

/**
 * @brief Status bits
 */
#define CCS811_STATUS_ERROR (1 << 0)
#define CCS811_DATA_READY   (1 << 3)
#define CCS811_APP_VALID    (1 << 4)
#define CCS811_FW_MODE      (1 << 7)

/**
 * @brief Error bits
 */
#define CCS811_MSG_INVALID      (1 << 0)
#define CCS811_READ_REG_INVALID (1 << 1)
#define CCS811_MEASMODE_INVALID (1 << 2)
#define CCS811_MAX_RESISTANCE   (1 << 3)
#define CCS811_HEATER_FAULT     (1 << 4)
#define CCS811_HEATER_SUPPLY    (1 << 5)

/**
 * @brief Default hardware id
 */
#define CCS811_DEFAULT_HW_ID 0x81

/**
 * @brief Default I2C address
 *
 * This can me 0x5a or 0x5b
 * Note that for STM32 this address must be shifted left 1 bit
 */
#define CCS811_DEFAULT_ADDRESS 0x5a

/**
 * @brief CCS811 drive modes
 */
typedef enum
{
    CCS811_DRIVE_IDLE,     /**< Sensor is idle, no measurement */
    CCS811_DRIVE_1SEC,     /**< Constant power mode, take a measurement every second */
    CCS811_DRIVE_10SEC,    /**< Pulse heating mode, take a measurement every 10 seconds */
    CCS811_DRIVE_60SEC,    /**< Low power mode, take a measurement every 60 seconds */
    CCS811_DRIVE_RAW       /**< Constant power mode, take a measurement every 250 ms */
} ccs811_drive_mode;

/**
 * @brief CCS811 environmental data
 */
typedef struct
{
    float humidity;       /**< Humidity value used for the measurement algorithm */
    float temperature;    /**< temperature value used for the measurement algorithm */
} ccs811_environmental_data;

/**
 * @brief CCS811 device
 */
typedef struct
{
    I2C_HandleTypeDef *i2c_handle;         /**< I2C handle connected to the sensor */
    uint8_t i2c_address;                   /**< Sensor I2C address */
    ccs811_drive_mode drive_mode;          /**< Sensor drive mode */
    uint8_t hardware_id;                   /**< Sensor hardware ID */
    uint8_t hardware_version;              /**< Sensor hardware version */
    ccs811_environmental_data env_data;    /**< Environmental data used for the measurement algorithm */
} ccs811_device;

/**
 * @brief CCS811 measurements
 */
typedef struct
{
    uint16_t tvoc;                /**< Total volatile organic compound */
    uint16_t eco2;                /**< Equivalent carbon dioxide */
    uint16_t current_selected;    /**< Current through the sensor */
    uint16_t raw_adc;             /**< Raw ADC reading */
} ccs811_measurements;

/**
 * @brief Initialize sensor
 * @param device: Pointer to CCS811 device
 * @return: true if device was initialized successfully, false otherwise
 */
bool ccs811_init(ccs811_device *device);

/**
 * @brief Check if data is available to be read
 * @param device: Pointer to CCS811 device
 * @return: true if data is available to be read, false otherwise
 */
bool ccs811_data_available(ccs811_device *device);

/**
 * @brief Get the current baseline
 * @param device: Pointer to CCS811 device
 * @return: Current baseline
 */
uint16_t ccs811_get_baseline(ccs811_device *device);

/**
 * @brief Set current baseline
 * @param device: Pointer to CCS811 device
 * @param baseline: The baseline value
 * @return: true if baseline was configured successfully, false otherwise
 */
bool ccs811_set_baseline(ccs811_device *device, uint16_t baseline);

/**
 * @brief Set environmental data
 * @param device: Pointer to CCS811 device
 * @param humidity: The humidity value
 * @param temperature: The temperature value
 * @return: true if environmental data was configured successfully, false otherwise
 */
bool ccs811_set_environmental_data(ccs811_device *device, float humidity, float temperature);

/**
 * @brief Read measurements
 * @param device: Pointer to CCS811 device
 * @param measurements: Pointer to CCS811 measurements structure
 * @return: true if measurements were read successfully, false otherwise
 */
bool ccs811_read_measurements(ccs811_device *device, ccs811_measurements *measurements);

#ifdef __cplusplus
}
#endif

#endif /* CCS811_H */
