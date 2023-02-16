#include <stdio.h>
#include <string.h>
#include "wifi.h"
#include "bme280.h"
#include "ccs811.h"
#include "circular_buffer.h"
#include "software_timer.h"

/**
 * @brief WiFi configuration flash storage
 */
__attribute__((__section__(".wifi_user_data")))
    const uint8_t wifi_user_data[512];

/**
 * @brief WiFi device
 */
static wifi_device wifi_dev;

/**
 * @brief WiFi circular buffer for UART communication
 */
CIRCULAR_BUFFER_DEF(wifi_cbuff, 1024);

/**
 * @brief WiFi timeout timers
 */
SOFTWARE_TIMER_DEF(wifi_init_timer, 3000);
SOFTWARE_TIMER_DEF(wifi_network_timer, 10000);
SOFTWARE_TIMER_DEF(wifi_mqtt_timer, 10000);

/**
 * @brief Byte received from the UART
 */
static uint8_t received_byte = 0;

/**
 * @brief Measurement values from the sensors
 */
extern bme280_measurements environmental_data;
extern ccs811_measurements air_quality;

/**
 * @brief Local variables to keep track if measured data has changed
 */
static bme280_measurements old_environmental_data;
static ccs811_measurements old_air_quality;

/**
 * @brief WiFi private functions
 */
static bool send_command(const char *command);
static void clear_rx_buffer(void);
static bool check_response(const char *str);
static uint16_t create_payload(char *payload);
static bool measured_data_changed(void);
static bool save_configuration();
static void read_configuration();

bool wifi_init(UART_HandleTypeDef *huart)
{
    wifi_dev.uart_handle = huart;
    wifi_dev.rx_index = 0;
    wifi_dev.lf_received = false;
    wifi_dev.init_retry_count = 0;
    wifi_dev.network_retry_count = 0;
    wifi_dev.mqtt_retry_count = 0;
    wifi_dev.publish_retry_count = 0;
    wifi_dev.state = WIFI_INITIALIZE;

    clear_rx_buffer();

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_UART_Receive_IT(wifi_dev.uart_handle, &received_byte,
            sizeof(received_byte));

    if (result != HAL_OK)
        return false;

    read_configuration();

    return true;
}

bool wifi_set_configuration(wifi_config *config)
{
    if (config == NULL)
        return false;

    strcpy(wifi_dev.configuration.network_ssid, config->network_ssid);
    strcpy(wifi_dev.configuration.network_password, config->network_password);
    strcpy(wifi_dev.configuration.broker_address, config->broker_address);
    wifi_dev.configuration.broker_port = config->broker_port;
    strcpy(wifi_dev.configuration.broker_token, config->broker_token);

    if (!save_configuration())
    {
        return false;
    }

    wifi_dev.state = WIFI_INITIALIZE;
    return true;
}

wifi_state wifi_get_state(void)
{
    return wifi_dev.state;
}

void wifi_get_configuration(wifi_config *config)
{
    if (config == NULL)
        return;

    strcpy(config->network_ssid, wifi_dev.configuration.network_ssid);
    strcpy(config->network_password, wifi_dev.configuration.network_password);
    strcpy(config->broker_address, wifi_dev.configuration.broker_address);
    config->broker_port = wifi_dev.configuration.broker_port;
    strcpy(config->broker_token, wifi_dev.configuration.broker_token);
}

void wifi_handler()
{
    char command_buffer[128];
    char payload[512];
    uint16_t payload_size = 0;

    if (circular_buffer_has_data(&wifi_cbuff))
    {
        uint8_t data = 0;
        circular_buffer_pop(&wifi_cbuff, &data);

        wifi_dev.rx_buffer[wifi_dev.rx_index] = data;
        wifi_dev.rx_index++;

        if (data == 0x0A)
        {
            wifi_dev.lf_received = true;
        }
    }

    switch (wifi_dev.state)
    {
    case WIFI_INITIALIZE:
        if (!send_command("AT+RST\r\n"))
        {
            wifi_dev.state = WIFI_ERROR_INITIALIZE;
        }
        else
        {
            wifi_dev.state = WIFI_RESTARTING;
            timer_start(&wifi_init_timer);
        }
        break;

    case WIFI_RESTARTING:
        if (wifi_dev.lf_received)
        {
            wifi_dev.lf_received = false;
            if (check_response("ready"))
            {
                wifi_dev.state = WIFI_MODE_CONFIGURE;
            }
            else if (check_response("ERROR"))
            {
                wifi_dev.state = WIFI_ERROR_INITIALIZE;
            }
            clear_rx_buffer();
        }

        if (timer_is_expired(&wifi_init_timer))
        {
            wifi_dev.state = WIFI_ERROR_INITIALIZE;
        }
        break;

    case WIFI_MODE_CONFIGURE:
        if (!send_command("AT+CWMODE=1\r\n"))
        {
            wifi_dev.state = WIFI_ERROR_INITIALIZE;
        }
        else
        {
            wifi_dev.state = WIFI_MODE_CONFIGURING;
            timer_start(&wifi_init_timer);
        }
        break;

    case WIFI_MODE_CONFIGURING:
        if (wifi_dev.lf_received)
        {
            wifi_dev.lf_received = false;
            if (check_response("OK"))
            {
                wifi_dev.state = WIFI_NETWORK_CONNECT;
            }
            else if (check_response("ERROR"))
            {
                wifi_dev.state = WIFI_ERROR_INITIALIZE;
            }
            clear_rx_buffer();
        }

        if (timer_is_expired(&wifi_init_timer))
        {
            wifi_dev.state = WIFI_ERROR_INITIALIZE;
        }
        break;

    case WIFI_NETWORK_CONNECT:
        sprintf(command_buffer, "AT+CWJAP=\"%s\",\"%s\"\r\n",
                wifi_dev.configuration.network_ssid,
                wifi_dev.configuration.network_password);

        if (!send_command(command_buffer))
        {
            wifi_dev.state = WIFI_ERROR_NETWORK;
        }
        else
        {
            wifi_dev.state = WIFI_NETWORK_CONNECTING;
            timer_start(&wifi_network_timer);
        }
        break;

    case WIFI_NETWORK_CONNECTING:
        if (wifi_dev.lf_received)
        {
            wifi_dev.lf_received = false;
            if (check_response("OK"))
            {
                wifi_dev.state = WIFI_NETWORK_CONNECTED;
            }
            else if (check_response("FAIL"))
            {
                wifi_dev.state = WIFI_ERROR_NETWORK;
            }
            clear_rx_buffer();
        }

        if (timer_is_expired(&wifi_network_timer))
        {
            wifi_dev.state = WIFI_ERROR_NETWORK;
        }
        break;

    case WIFI_NETWORK_CONNECTED:
        wifi_dev.network_retry_count = 0;
        wifi_dev.state = WIFI_MQTT_CONNECT;
        break;

    case WIFI_MQTT_CONNECT:
        sprintf(command_buffer, "AT+CIPSTART=\"TCP\",\"%s\",%lu\r\n",
                wifi_dev.configuration.broker_address,
                wifi_dev.configuration.broker_port);

        if (!send_command(command_buffer))
        {
            wifi_dev.state = WIFI_ERROR_MQTT_BROKER;
        }
        else
        {
            wifi_dev.state = WIFI_MQTT_CONNECTING;
            timer_start(&wifi_mqtt_timer);
        }
        break;

    case WIFI_MQTT_CONNECTING:
        if (wifi_dev.lf_received)
        {
            wifi_dev.lf_received = false;
            if (check_response("OK") || check_response("ALREADY CONNECTED"))
            {
                wifi_dev.state = WIFI_MQTT_CONNECTED;
            }
            else if (check_response("ERROR"))
            {
                wifi_dev.state = WIFI_ERROR_MQTT_BROKER;
            }
            clear_rx_buffer();
        }

        if (timer_is_expired(&wifi_mqtt_timer))
        {
            wifi_dev.state = WIFI_ERROR_MQTT_BROKER;
        }
        break;

    case WIFI_MQTT_CONNECTED:
        if (wifi_dev.lf_received)
        {
            wifi_dev.lf_received = false;
            if (check_response("WIFI DISCONNECTED"))
            {
                wifi_dev.state = WIFI_ERROR_NETWORK;
            }
            else if (check_response("CLOSED"))
            {
                wifi_dev.state = WIFI_ERROR_MQTT_BROKER;
            }
            clear_rx_buffer();
        }

        if (measured_data_changed())
        {
            old_environmental_data = environmental_data;
            old_air_quality = air_quality;
            wifi_dev.state = WIFI_MQTT_PUBLISH_START;
        }
        break;

    case WIFI_MQTT_PUBLISH_START:
        payload_size = create_payload(payload);
        sprintf(command_buffer, "AT+CIPSEND=%d\r\n", payload_size);

        if (!send_command(command_buffer))
        {
            wifi_dev.state = WIFI_ERROR_MQTT_PUBLISH;
            timer_start(&wifi_mqtt_timer);
        }
        else
        {
            wifi_dev.state = WIFI_MQTT_PUBLISH;
        }
        break;

    case WIFI_MQTT_PUBLISH:
        if (wifi_dev.lf_received)
        {
            wifi_dev.lf_received = false;
            if (check_response("OK"))
            {
                HAL_Delay(25);
                if (!send_command(payload))
                {
                    wifi_dev.state = WIFI_ERROR_MQTT_PUBLISH;
                }
                else
                {
                    wifi_dev.state = WIFI_MQTT_PUBLISH_WAIT_REPLY;
                    timer_start(&wifi_mqtt_timer);
                }
            }
            clear_rx_buffer();
        }

        if (timer_is_expired(&wifi_mqtt_timer))
        {
            wifi_dev.state = WIFI_ERROR_MQTT_PUBLISH;
        }
        break;

    case WIFI_MQTT_PUBLISH_WAIT_REPLY:
        if (wifi_dev.lf_received)
        {
            wifi_dev.lf_received = false;
            if (check_response("SEND OK"))
            {
                wifi_dev.publish_retry_count = 0;
                wifi_dev.state = WIFI_MQTT_CONNECTED;
            }
            else if (check_response("SEND FAIL"))
            {
                wifi_dev.state = WIFI_ERROR_MQTT_PUBLISH;
            }
            clear_rx_buffer();
        }

        if (timer_is_expired(&wifi_mqtt_timer))
        {
            wifi_dev.state = WIFI_ERROR_MQTT_PUBLISH;
        }
        break;

    case WIFI_ERROR_INITIALIZE:
        if (wifi_dev.init_retry_count < WIFI_INIT_MAX_RETRY)
        {
            wifi_dev.init_retry_count++;
            wifi_dev.state = WIFI_INITIALIZE;
        }
        break;

    case WIFI_ERROR_NETWORK:
        if (wifi_dev.network_retry_count < WIFI_NETWORK_MAX_RETRY)
        {
            wifi_dev.network_retry_count++;
            wifi_dev.state = WIFI_NETWORK_CONNECT;
        }
        break;

    case WIFI_ERROR_MQTT_BROKER:
        if (wifi_dev.mqtt_retry_count < WIFI_MQTT_MAX_RETRY)
        {
            wifi_dev.mqtt_retry_count++;
            wifi_dev.state = WIFI_MQTT_CONNECT;
        }
        break;

    case WIFI_ERROR_MQTT_PUBLISH:
        if (wifi_dev.publish_retry_count < WIFI_MQTT_MAX_RETRY)
        {
            wifi_dev.publish_retry_count++;
            wifi_dev.state = WIFI_MQTT_PUBLISH_START;
        }
        break;

    default:
        break;
    }
}

void wifi_rx_callback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != wifi_dev.uart_handle->Instance)
        return;

    circular_buffer_push(&wifi_cbuff, received_byte);
    HAL_UART_Receive_IT(wifi_dev.uart_handle, &received_byte, sizeof(received_byte));
}

/**
 * @brief Send AT command to the WiFi chip
 * @param command: The AT command
 * @return: True if the command was sent successfully, false otherwise
 */
static bool send_command(const char *command)
{
    if (command == NULL)
        return false;

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_UART_Transmit(wifi_dev.uart_handle, (uint8_t *)command,
            strlen(command), HAL_MAX_DELAY);

    return (result == HAL_OK);
}

static void clear_rx_buffer()
{
    memset(wifi_dev.rx_buffer, 0, WIFI_RX_BUFFER_SIZE);
    wifi_dev.rx_index = 0;
}

/**
 * @brief Check if response contains string
 * @param str: The string value
 * @return: True if the rx buffer contains the string, false otherwise
 */
static bool check_response(const char *str)
{
    if (str == NULL)
        return false;

    return (strstr((char *)wifi_dev.rx_buffer, str) != NULL);
}

/**
 * @brief Create POST request payload
 * @param payload: Pointer to payload buffer
 * @return: Size of the payload buffer
 */
static uint16_t create_payload(char *payload)
{
    if (payload == NULL)
        return 0;

    char json[128];
    sprintf(json, "{temperature:%.2f, humidity:%.2f, pressure:%.2f, tvoc:%d, eco2:%d}",
            environmental_data.temperature,
            environmental_data.humidity,
            environmental_data.pressure / 100, /* Convert pressure to hPa */
            air_quality.tvoc,
            air_quality.eco2);

    sprintf(payload, "POST /api/v1/%s/telemetry HTTP/1.1\r\n"
            "Host: 192.168.0.218:8080\r\n"
            "Content-Type:application/json\r\n"
            "Content-Length: %d\r\n\r\n%s\r\n",
            wifi_dev.configuration.broker_token,
            strlen(json),
            json);

    return strlen(payload);
}

/**
 * @brief Check if measured data was changed
 * @return: True if measured data like air quality was changed, false otherwise
 */
static bool measured_data_changed()
{
    if ((environmental_data.pressure != old_environmental_data.pressure)
        || (environmental_data.temperature != old_environmental_data.temperature)
        || (environmental_data.humidity != old_environmental_data.humidity))
        return true;

    if ((air_quality.tvoc != old_air_quality.tvoc)
        || (air_quality.eco2 != old_air_quality.eco2))
        return true;

    return false;
}

/**
 * @brief Save WiFi configuration to flash
 * @return: True if the configuration was saved, false otherwise
 */
static bool save_configuration()
{
    bool result = true;
    FLASH_EraseInitTypeDef EraseInitStruct = { 0 };
    uint32_t erase_error = 0;
    uint32_t flash_address = WIFI_CFG_FLASH_ADDRESS;
    uint64_t *record = (uint64_t *)&wifi_dev.configuration;

    HAL_FLASH_Unlock();

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Page      = 63;
    EraseInitStruct.NbPages   = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &erase_error) != HAL_OK)
    {
        return false;
    }

    for (int index = 0; index < sizeof(wifi_dev.configuration);
            index += 8, flash_address += 8, record++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                flash_address, (uint64_t)*record) != HAL_OK)
        {
            result = false;
            break;
        }
    }

    HAL_FLASH_Lock();
    return result;
}

/**
 * @brief Read the WiFi configuration from flash
 */
static void read_configuration()
{
    uint8_t *record = (uint8_t*)&wifi_dev.configuration;

    for (int index = 0; index < sizeof(wifi_dev.configuration); index++)
    {
        *record++ = wifi_user_data[index];
    }
}
