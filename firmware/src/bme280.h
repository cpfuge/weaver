#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32g0xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Default I2C address
 *
 * This can me 0x77 or 0x76
 * Note that for STM32 this address must be shifted left 1 bit
 */
#define BME280_DEFAULT_ADDRESS 0x76

/**
 * @brief Default hardware id
 */
#define BME280_DEFAULT_HW_ID 0x60

/**
 * @brief BME280 registers
 */
#define BME280_REG_CALIB0       0x88
#define BME280_REG_CHIPID       0xd0
#define BME280_REG_SOFTRESET    0xe0
#define BME280_REG_CALIB26      0xe1
#define BME280_REG_CTRLHUMID    0xf2
#define BME280_REG_STATUS       0xf3
#define BME280_REG_CTRLMEAS     0xf4
#define BME280_REG_CONFIG       0xf5
#define BME280_REG_PRESSUREDATA 0xf7
#define BME280_REG_TEMPDATA     0xfa
#define BME280_REG_HUMIDDATA    0xfd

/**
 * @brief BME280 calibration registers
 */
#define BME280_REG_DIG_T1 0x88
#define BME280_REG_DIG_T2 0x8a
#define BME280_REG_DIG_T3 0x8c
#define BME280_REG_DIG_P1 0x8e
#define BME280_REG_DIG_P2 0x90
#define BME280_REG_DIG_P3 0x92
#define BME280_REG_DIG_P4 0x94
#define BME280_REG_DIG_P5 0x96
#define BME280_REG_DIG_P6 0x98
#define BME280_REG_DIG_P7 0x9a
#define BME280_REG_DIG_P8 0x9c
#define BME280_REG_DIG_P9 0x9e
#define BME280_REG_DIG_H1 0xa1
#define BME280_REG_DIG_H2 0xe1
#define BME280_REG_DIG_H3 0xe3
#define BME280_REG_DIG_H4 0xe4
#define BME280_REG_DIG_H5 0xe5
#define BME280_REG_DIG_H6 0xe7

/**
 * @brief data length
 */
#define BME280_CALIB0_DATA_LENGTH  26
#define BME280_CALIB26_DATA_LENGTH 7
#define BME280_MEASURE_DATA_LENGTH 8

/**
 * @brief BME280 commands
 */
#define BME280_RESET_COMMAND 0xb6

/**
 * @brief BME280 working mode
 */
typedef enum
{
    BMP280_MODE_SLEEP  = 0,    /**< Sleep mode, no measurement */
    BMP280_MODE_FORCED = 1,    /**< Forced mode, measurement is initiated by user */
    BMP280_MODE_NORMAL = 3     /**< Normal mode, automated measurement */
} bme280_mode;

/**
 * @brief BME280 filter coefficient
 */
typedef enum
{
    BMP280_FILTER_OFF,    /**<  1 samples to reach >= 75% of step response */
    BMP280_FILTER_2,      /**<  2 samples to reach >= 75% of step response */
    BMP280_FILTER_4,      /**<  5 samples to reach >= 75% of step response */
    BMP280_FILTER_8,      /**< 11 samples to reach >= 75% of step response */
    BMP280_FILTER_16      /**< 22 samples to reach >= 75% of step response */
} bme280_filter;

/**
 * @brief BME280 sampling rates
 */
typedef enum
{
    BME280_SAMPLING_NONE,    /**< No measurement */
    BME280_SAMPLING_X1,      /**< Ultra low power, sampling x1 */
    BME280_SAMPLING_X2,      /**< Low power, sampling x2 */
    BME280_SAMPLING_X4,      /**< Standard, sampling x4 */
    BME280_SAMPLING_X8,      /**< High resolution, sampling x8 */
    BME280_SAMPLING_X16      /**< Ultra high resolution, sampling x16 */
} bme280_sampling;

/**
 * @brief BME280 standby duration between measurements
 */
typedef enum
{
    BME280_STANDBY_MS_05,      /**< 0.5ms  */
    BME280_STANDBY_MS_62,      /**< 62.5ms */
    BME280_STANDBY_MS_125,     /**< 125ms  */
    BME280_STANDBY_MS_250,     /**< 250ms  */
    BME280_STANDBY_MS_500,     /**< 500ms  */
    BME280_STANDBY_MS_1000,    /**< 1s     */
    BME280_STANDBY_MS_2000,    /**< 2s     */
    BME280_STANDBY_MS_4000,    /**< 4s     */
} bme280_standby;

/**
 * @brief BME280 Calibration data
 */
typedef struct
{
    /* Temperature compensation value */
    uint16_t dig_t1;
    int16_t  dig_t2;
    int16_t  dig_t3;

    /* Pressure compensation value */
    uint16_t dig_p1;
    int16_t  dig_p2;
    int16_t  dig_p3;
    int16_t  dig_p4;
    int16_t  dig_p5;
    int16_t  dig_p6;
    int16_t  dig_p7;
    int16_t  dig_p8;
    int16_t  dig_p9;

    /* Humidity compensation value */
    uint16_t dig_h1;
    int16_t  dig_h2;
    int16_t  dig_h3;
    uint16_t dig_h4;
    int16_t  dig_h5;
    int8_t   dig_h6;

    /* Temperature with high resolution */
    int32_t t_fine;
} bme280_calibration_data;

/**
 * @brief BME280 device
 */
typedef struct
{
    I2C_HandleTypeDef *i2c_handle;               /**< I2C handle connected to the sensor */
    uint8_t i2c_address;                         /**< Sensor I2C address */
    uint8_t hardware_id;                         /**< Chip hardware Id */
    bme280_mode mode;                            /**< Working mode */
    bme280_filter filter;                        /**< Filter configuration */
    bme280_standby standby_duration;             /**< Standby duration between measurements */
    bme280_sampling pressure_sampling;           /**< Pressure oversampling */
    bme280_sampling temperature_sampling;        /**< Temperature oversampling */
    bme280_sampling humidity_sampling;           /**< Humidity oversampling */
    bme280_calibration_data calibration_data;    /**< Calibration data */
} bme280_device;

/**
 * @brief BME280 measurements
 */
typedef struct
{
    float pressure;
    float temperature;
    float humidity;
} bme280_measurements;

/**
 * @brief BME180 initialization
 * @param device: Pointer to BME280 device
 * @return: true if device was initialized successfully, false otherwise
 */
bool bme280_init(bme280_device *device);

/**
 * @brief Get the device status
 * @param device: Pointer to BME280 device
 * @return: The status of the BME280 sensor
 */
uint8_t bme280_get_status(bme280_device *device);

/**
 * @brief Check if the device is taking measurements
 * @param device: Pointer to BME280 device
 * @return: true if the device is taking measurements, false otherwise
 */
bool bme280_is_measuring(bme280_device *device);

/**
 * @brief Read measurements
 * @param device: Pointer to BME280 device
 * @param measurements: Pointer to BME280 measurements structure
 * @return: true if measurements were read successfully, false otherwise
 */
bool bme280_read_measurements(bme280_device *device, bme280_measurements *measurements);

#ifdef __cplusplus
}
#endif

#endif /* BME280_H */
