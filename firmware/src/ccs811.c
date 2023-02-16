#include "ccs811.h"

/* CCS811 private functions */
static bool software_reset(ccs811_device *device);
static uint8_t read_status(ccs811_device *device);
static bool start_application(ccs811_device *device);
static bool configure_drive_mode(ccs811_device *device);

bool ccs811_init(ccs811_device *device)
{
    if (device == NULL)
        return false;

    device->i2c_address = (uint8_t)(device->i2c_address << 1);

    if (!software_reset(device))
        return false;

    HAL_Delay(75);

    uint8_t status = read_status(device);
    if (status & CCS811_STATUS_ERROR || !(status & CCS811_APP_VALID))
        return false;

    if (!start_application(device))
        return false;

    HAL_Delay(75);

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, CCS811_HW_ID,
            I2C_MEMADD_SIZE_8BIT, &device->hardware_id, sizeof(device->hardware_id),
            HAL_MAX_DELAY);

    if (result != HAL_OK)
        return false;

    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, CCS811_HW_VERSION,
            I2C_MEMADD_SIZE_8BIT, &device->hardware_version, sizeof(device->hardware_version),
            HAL_MAX_DELAY);

    if (result != HAL_OK)
        return false;

    if (device->hardware_id != CCS811_DEFAULT_HW_ID)
        return false;

    if (!configure_drive_mode(device))
        return false;

    return true;
}

bool ccs811_data_available(ccs811_device *device)
{
    if (device == NULL)
        return false;

    uint8_t status = read_status(device);
    return (status && CCS811_DATA_READY);
}

uint16_t ccs811_get_baseline(ccs811_device *device)
{
    if (device == NULL)
        return 0;

    uint8_t buffer[2];
    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, CCS811_BASELINE,
            I2C_MEMADD_SIZE_8BIT, buffer, 2, HAL_MAX_DELAY);

    if (result != HAL_OK)
        return 0;

    return ((uint16_t)buffer[0] << 8) | ((uint16_t)buffer[1]);
}

bool ccs811_set_baseline(ccs811_device *device, uint16_t baseline)
{
    if (device == NULL)
        return false;

    uint8_t buffer[] = {
        (uint8_t)((baseline >> 8) & 0xFF),
        (uint8_t)(baseline & 0xFF)
    };

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Write(device->i2c_handle, device->i2c_address, CCS811_BASELINE,
            I2C_MEMADD_SIZE_8BIT, buffer, 2, HAL_MAX_DELAY);

    return (result == HAL_OK);
}

bool ccs811_set_environmental_data(ccs811_device *device, float humidity, float temperature)
{
    if (device == NULL)
        return false;

    if ((device->env_data.humidity == humidity)
        && (device->env_data.temperature == temperature))
    {
        return false;
    }

    device->env_data.humidity = humidity;
    device->env_data.temperature = temperature;

    uint16_t hum = humidity * 512.0f + 0.5f;
    uint16_t temp = (temperature + 25.0f) * 512.0f + 0.5f;

    uint8_t buffer[] = {
        (uint8_t)((hum >> 8) & 0xFF),
        (uint8_t)(hum & 0xFF),
        (uint8_t)((temp >> 8) & 0xFF),
        (uint8_t)(temp & 0xFF)
    };

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Write(device->i2c_handle, device->i2c_address, CCS811_ENV_DATA,
            I2C_MEMADD_SIZE_8BIT, buffer, 4, HAL_MAX_DELAY);

    return (result == HAL_OK);
}

bool ccs811_read_measurements(ccs811_device *device, ccs811_measurements *measurements)
{
    if (device == NULL)
        return false;

     uint8_t buffer[8];
     HAL_StatusTypeDef result = HAL_OK;
     result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, CCS811_RESULT_DATA,
             I2C_MEMADD_SIZE_8BIT, buffer, 8, HAL_MAX_DELAY);

     if (result != HAL_OK)
         return false;

     measurements->eco2 = ((uint16_t)buffer[0] << 8) | ((uint16_t)buffer[1]);
     measurements->tvoc = ((uint16_t)buffer[2] << 8) | ((uint16_t)buffer[3]);
     measurements->current_selected = ((uint16_t)buffer[6] >> 2);
     measurements->raw_adc = ((uint16_t)(buffer[6] & 3) << 8) | ((uint16_t)buffer[7]);

     return true;
}

/**
 * @brief Reset the internal software
 * @param device: Pointer to BME280 device
 * @return: True if the device was restarted successfully, false otherwise
 */
static bool software_reset(ccs811_device *device)
{
    uint8_t buffer[4] = { 0x11, 0xE5, 0x72, 0x8A };

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Write(device->i2c_handle, device->i2c_address,
            CCS811_SOFTWARE_RESET, I2C_MEMADD_SIZE_8BIT, buffer, 4, HAL_MAX_DELAY);

    return (result == HAL_OK);
}

/**
 * @brief Read the status value from the register
 * @param device: Pointer to BME280 device
 * @return: The device status code
 */
static uint8_t read_status(ccs811_device *device)
{
    uint8_t status = 0;
    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, CCS811_STATUS,
            I2C_MEMADD_SIZE_8BIT, &status, sizeof(status), HAL_MAX_DELAY);

    if (result != HAL_OK)
        return 0;

    return status;
}

/**
 * @brief Start internal application
 * @param device: Pointer to BME280 device
 * @return: True if the application was started successfully, false otherwise
 */
static bool start_application(ccs811_device *device)
{
    uint8_t data = CCS811_APP_START;
    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Master_Transmit(device->i2c_handle, device->i2c_address, &data,
            sizeof(data), HAL_MAX_DELAY);

    return (result == HAL_OK);
}

/**
 * @brief Configure CCS811 drive mode
 * @param device: Pointer to BME280 device
 * @return: True if the device was configured successfully, false otherwise
 */
static bool configure_drive_mode(ccs811_device *device)
{
    uint8_t value = 0;
    uint8_t mode = (uint8_t)device->drive_mode;
    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_I2C_Mem_Read(device->i2c_handle, device->i2c_address, CCS811_MEASURE_MODE,
            I2C_MEMADD_SIZE_8BIT, &value, sizeof(value), HAL_MAX_DELAY);

    if (result != HAL_OK)
        return false;

    value &= ~(0b00000111 << 4);
    value |= (mode << 4);

    result = HAL_I2C_Mem_Write(device->i2c_handle, device->i2c_address, CCS811_MEASURE_MODE,
            I2C_MEMADD_SIZE_8BIT, &value, sizeof(value) + 1, HAL_MAX_DELAY);

    return (result == HAL_OK);
}
