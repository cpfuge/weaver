#include "bme280.h"

#define CONCAT_BYTES(msb, lsb) (((uint16_t)msb << 8) | (uint16_t)lsb)

/* BME280 private functions */
static uint8_t read_hardware_id(bme280_device *device);
static bool soft_reset(bme280_device *device);
static uint8_t read_status(bme280_device *device);
static bool read_calibration_data(bme280_device *device);
static bool configure_sampling(bme280_device *device);
static float compensate_temperature(bme280_device *device, int32_t adc_temperature);
static float compensate_pressure(bme280_device *device, int32_t adc_pressure);
static float compensate_humidity(bme280_device *device, int32_t adc_humidity);

bool bme280_init(bme280_device *device)
{
    if (device == NULL)
        return false;

    device->i2c_address = (uint8_t)(device->i2c_address << 1);
    device->hardware_id = read_hardware_id(device);

    if (device->hardware_id != BME280_DEFAULT_HW_ID)
        return false;

    soft_reset(device);

    while (1)
    {
        uint8_t status = read_status(device);
        if ((status & (1 << 0)) == 0)
            break;
    }

    if (!read_calibration_data(device))
        return false;

    if (!configure_sampling(device))
        return false;

    return true;
}

uint8_t bme280_get_status(bme280_device *device)
{
    if (device == NULL)
        return false;

    uint8_t status = 0;

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, BME280_REG_STATUS,
            I2C_MEMADD_SIZE_8BIT, &status, sizeof(status), HAL_MAX_DELAY);

    if (result != HAL_OK)
        return 0;

    return status;
}

bool bme280_is_measuring(bme280_device *device)
{
    if (device == NULL)
        return false;

    uint8_t status = bme280_get_status(device);
    return (status & (1 << 3));
}

bool bme280_read_measurements(bme280_device *device, bme280_measurements *measurements)
{
    if (device == NULL || measurements == NULL)
        return false;

    int32_t adc_pressure = 0;
    int32_t adc_temperature = 0;
    int32_t adc_humidity = 0;
    uint8_t buffer[BME280_MEASURE_DATA_LENGTH] = { 0 };

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, BME280_REG_PRESSUREDATA,
            I2C_MEMADD_SIZE_8BIT, buffer, BME280_MEASURE_DATA_LENGTH, HAL_MAX_DELAY);

    if (result != HAL_OK)
        return false;

    adc_pressure = (buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4);
    adc_temperature = (buffer[3] << 12) | (buffer[4] << 4) | (buffer[5] >> 4);
    adc_humidity = (buffer[6] << 8) | buffer[7];

    measurements->temperature = compensate_temperature(device, adc_temperature);
    measurements->pressure = compensate_pressure(device, adc_pressure);
    measurements->humidity = compensate_humidity(device, adc_humidity);

    return true;
}

/**
 * @brief Read senzor hardware id
 * @param device: Pointer to BME280 device
 * @return: The device hardware id value, this should be 0x60
 */
static uint8_t read_hardware_id(bme280_device *device)
{
    uint8_t hardware_id = 0;

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, BME280_REG_CHIPID,
            I2C_MEMADD_SIZE_8BIT, &hardware_id, sizeof(hardware_id), HAL_MAX_DELAY);

    if (result != HAL_OK)
        return 0;

    return hardware_id;
}

/**
 * @brief Reset the device using soft-reset
 * @param device: Pointer to BME280 device
 * @return: True if the device was restarted successfully, false otherwise
 */
static bool soft_reset(bme280_device *device)
{
    uint8_t command = BME280_RESET_COMMAND;

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, BME280_REG_SOFTRESET,
            I2C_MEMADD_SIZE_8BIT, &command, sizeof(command), HAL_MAX_DELAY);

    return (result == HAL_OK);
}

/**
 * @brief Read device status
 * @param device: Pointer to BME280 device
 * @return: Device status code
 */
static uint8_t read_status(bme280_device *device)
{
    uint8_t status = 0;

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, BME280_REG_STATUS,
            I2C_MEMADD_SIZE_8BIT, &status, sizeof(status), HAL_MAX_DELAY);

    if (result != HAL_OK)
        return 0;

    return status;
}

/**
 * @brief Read calibration data
 * @param device: Pointer to BME280 device
 * @return: True if calibration data was read successfully, false otherwise
 */
static bool read_calibration_data(bme280_device *device)
{
    uint8_t calib0_data[BME280_CALIB0_DATA_LENGTH] = { 0 };
    uint8_t calib26_data[BME280_CALIB26_DATA_LENGTH] = { 0 };

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, BME280_REG_CALIB0,
            I2C_MEMADD_SIZE_8BIT, calib0_data, BME280_CALIB0_DATA_LENGTH, HAL_MAX_DELAY);

    if (result != HAL_OK)
        return false;

    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, BME280_REG_CALIB26,
            I2C_MEMADD_SIZE_8BIT, calib26_data, BME280_CALIB26_DATA_LENGTH, HAL_MAX_DELAY);

    if (result != HAL_OK)
        return false;

    /* Parse temperature compensation value */
    device->calibration_data.dig_t1 = CONCAT_BYTES(calib0_data[1], calib0_data[0]);
    device->calibration_data.dig_t2 = (int16_t)CONCAT_BYTES(calib0_data[3], calib0_data[2]);
    device->calibration_data.dig_t3 = (int16_t)CONCAT_BYTES(calib0_data[5], calib0_data[4]);

    /* Parse pressure compensation value */
    device->calibration_data.dig_p1 = CONCAT_BYTES(calib0_data[7], calib0_data[6]);
    device->calibration_data.dig_p2 = (int16_t)CONCAT_BYTES(calib0_data[9], calib0_data[8]);
    device->calibration_data.dig_p3 = (int16_t)CONCAT_BYTES(calib0_data[11], calib0_data[10]);
    device->calibration_data.dig_p4 = (int16_t)CONCAT_BYTES(calib0_data[13], calib0_data[12]);
    device->calibration_data.dig_p5 = (int16_t)CONCAT_BYTES(calib0_data[15], calib0_data[14]);
    device->calibration_data.dig_p6 = (int16_t)CONCAT_BYTES(calib0_data[17], calib0_data[16]);
    device->calibration_data.dig_p7 = (int16_t)CONCAT_BYTES(calib0_data[19], calib0_data[18]);
    device->calibration_data.dig_p8 = (int16_t)CONCAT_BYTES(calib0_data[21], calib0_data[20]);
    device->calibration_data.dig_p9 = (int16_t)CONCAT_BYTES(calib0_data[23], calib0_data[22]);

    /* Parse humidity compensation value */
    device->calibration_data.dig_h1 = calib0_data[25];
    device->calibration_data.dig_h2 = (int16_t)CONCAT_BYTES(calib26_data[1], calib26_data[0]);
    device->calibration_data.dig_h3 = calib26_data[2];
    device->calibration_data.dig_h4 = ((int16_t)(int8_t)calib26_data[3] * 16) | ((int16_t)(calib26_data[4] & 0x0F));
    device->calibration_data.dig_h5 = ((int16_t)(int8_t)calib26_data[5] * 16) | ((int16_t)(calib26_data[4] >> 4));
    device->calibration_data.dig_h6 = (int8_t)calib26_data[6];

    return true;
}

/**
 * @brief Configure device sampling
 * @param device: Pointer to BME280 device
 * @return: True if the device was configured, false otherwise
 */
static bool configure_sampling(bme280_device *device)
{
    uint8_t sleep = BMP280_MODE_SLEEP;
    uint8_t config = (device->standby_duration << 5) | (device->filter << 2);
    uint8_t ctrl = (device->temperature_sampling << 5) | (device->pressure_sampling << 2) | (device->mode);
    uint8_t ctrl_hum = device->humidity_sampling;

    /* Put device to sleep first */
    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Write(device->i2c_handle, device->i2c_address, BME280_REG_CTRLMEAS,
            I2C_MEMADD_SIZE_8BIT, &sleep, sizeof(sleep), HAL_MAX_DELAY);

    if (result != HAL_OK)
        return false;

    /* Write humidity register */
    result = HAL_I2C_Mem_Write(device->i2c_handle, device->i2c_address, BME280_REG_CTRLHUMID,
            I2C_MEMADD_SIZE_8BIT, &ctrl_hum, sizeof(ctrl_hum), HAL_MAX_DELAY);

    if (result != HAL_OK)
        return false;

    /* Write config register */
    result = HAL_I2C_Mem_Write(device->i2c_handle, device->i2c_address, BME280_REG_CONFIG,
            I2C_MEMADD_SIZE_8BIT, &config, sizeof(config), HAL_MAX_DELAY);

    if (result != HAL_OK)
        return false;

    /* Write ctrl register */
    result = HAL_I2C_Mem_Write(device->i2c_handle, device->i2c_address, BME280_REG_CTRLMEAS,
            I2C_MEMADD_SIZE_8BIT, &ctrl, sizeof(ctrl), HAL_MAX_DELAY);

    if (result != HAL_OK)
        return false;

    return true;
}

/**
 * @brief Temperature compensation algorithm from BME280 datasheet
 * @param device: Pointer to BME280 device
 * @param adc_temperature: ADC reading for temperature
 * @return: Temperature value in degrees Celsius, resolution is 0.01 degrees Celsius.
 */
static float compensate_temperature(bme280_device *device, int32_t adc_temperature)
{
    int32_t var1 = 0;
    int32_t var2 = 0;

    var1 = ((((adc_temperature >> 3) - ((int32_t)device->calibration_data.dig_t1 << 1)))
            * (int32_t)device->calibration_data.dig_t2) >> 11;
    var2 = (((((adc_temperature >> 4) - (int32_t)device->calibration_data.dig_t1)
            * ((adc_temperature >> 4) - (int32_t)device->calibration_data.dig_t1)) >> 12)
            * (int32_t)device->calibration_data.dig_t3) >> 14;

    /* Set fine temperature */
    device->calibration_data.t_fine = var1 + var2;

    float temperature = (device->calibration_data.t_fine * 5 + 128) >> 8;
    return (temperature / 100);
}

/**
 * @brief Pressure compensation algorithm from BME280 datasheet
 * @param device: Pointer to BME280 device
 * @param adc_pressure: ADC reading for pressure
 * @return: Pressure value in Pa (Pascal).
 */
static float compensate_pressure(bme280_device *device, int32_t adc_pressure)
{
    int64_t var1 = 0;
    int64_t var2 = 0;
    int64_t pressure = 0;

    var1 = ((int64_t)device->calibration_data.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)device->calibration_data.dig_p6;
    var2 = var2 + ((var1 * (int64_t)device->calibration_data.dig_p5) << 17);
    var2 = var2 + (((int64_t)device->calibration_data.dig_p4) << 35);
    var1 = ((var1 * var1 * (int64_t)device->calibration_data.dig_p3) >> 8)
            + ((var1 * (int64_t)device->calibration_data.dig_p2) << 12);

    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)device->calibration_data.dig_p1) >> 33;

    if (var1 == 0)
        return 0;

    pressure = 1048576 - adc_pressure;
    pressure = (((pressure << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)device->calibration_data.dig_p9) * (pressure >> 13) * (pressure >> 13)) >> 25;
    var2 = (((int64_t)device->calibration_data.dig_p8) * pressure) >> 19;

    pressure = ((pressure + var1 + var2) >> 8) + (((int64_t)device->calibration_data.dig_p7) << 4);

    return ((float)pressure / 256);
}

/**
 * @brief Humidity compensation algorithm from BME280 datasheet
 * @param device: Pointer to BME280 device
 * @param adc_humidity: ADC reading for humidity
 * @return: Humidity value in %RH (relative humidity)
 */
static float compensate_humidity(bme280_device *device, int32_t adc_humidity)
{
    int32_t v_x1_u32r = 0;
    float humidity = 0;

    v_x1_u32r = (device->calibration_data.t_fine - ((int32_t)76800));

    v_x1_u32r = (((((adc_humidity << 14) - (((int32_t)device->calibration_data.dig_h4) << 20)
            - (((int32_t)device->calibration_data.dig_h5) * v_x1_u32r)) + ((int32_t)16384)) >> 15)
            * (((((((v_x1_u32r * ((int32_t)device->calibration_data.dig_h6)) >> 10)
            * (((v_x1_u32r * ((int32_t)device->calibration_data.dig_h3)) >> 11)
            + ((int32_t)32768))) >> 10) + ((int32_t)2097152))
            * ((int32_t)device->calibration_data.dig_h2) + 8192) >> 14));

    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7)
            * ((int32_t)device->calibration_data.dig_h1)) >> 4));

    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;

    humidity = (v_x1_u32r >> 12);

    return (humidity / 1024.0);
}
