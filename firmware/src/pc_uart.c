#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pc_uart.h"
#include "wifi.h"
#include "bme280.h"
#include "ccs811.h"
#include "circular_buffer.h"

/**
 * @brief PC communication
 */
static pc_uart pc_uart_dev;

/**
 * @brief PC circular buffer for UART communication
 */
CIRCULAR_BUFFER_DEF(pc_uart_cbuff, 512);

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
 * @brief PC private functions
 */
static void clear_rx_buffer(void);
static void parse_received_data(void);
static void parse_wifi_configuration(void);
static void send_sensors_values(void);
static void send_device_status(void);
static void send_config_reply(bool reply);

bool pc_uart_init(UART_HandleTypeDef *huart)
{
    if (huart == NULL)
        return false;

    pc_uart_dev.uart_handle = huart;
    clear_rx_buffer();

    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_UART_Receive_IT(pc_uart_dev.uart_handle, &received_byte, sizeof(received_byte));

    return (result == HAL_OK);
}

void pc_uart_handler(void)
{
    if (circular_buffer_has_data(&pc_uart_cbuff))
    {
        uint8_t data = 0;
        circular_buffer_pop(&pc_uart_cbuff, &data);

        if (pc_uart_dev.rx_index >= PC_RX_BUFFER_SIZE)
        {
            clear_rx_buffer();
        }

        pc_uart_dev.rx_buffer[pc_uart_dev.rx_index] = data;
        pc_uart_dev.rx_index++;

        if (data == 0x0A)
        {
            parse_received_data();
        }
    }
}

void pc_uart_rx_callback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != pc_uart_dev.uart_handle->Instance)
        return;

    circular_buffer_push(&pc_uart_cbuff, received_byte);
    HAL_UART_Receive_IT(pc_uart_dev.uart_handle, &received_byte, sizeof(received_byte));
}

static void clear_rx_buffer()
{
    memset(pc_uart_dev.rx_buffer, 0, PC_RX_BUFFER_SIZE);
    pc_uart_dev.rx_index = 0;
}

static void parse_received_data()
{
    if (strstr((char *)pc_uart_dev.rx_buffer, "WIFICFG") != NULL)
    {
        parse_wifi_configuration();
    }
    else if (strstr((char *)pc_uart_dev.rx_buffer, "SENSORS") != NULL)
    {
        send_sensors_values();
    }
    else if (strstr((char *)pc_uart_dev.rx_buffer, "STATUS") != NULL)
    {
        send_device_status();
    }

    clear_rx_buffer();
}

static void parse_wifi_configuration()
{
    char wifi_cfg[6][32];
    uint8_t index = 0;

    char *str = strtok(pc_uart_dev.rx_buffer, "|");
    strcpy(wifi_cfg[index++], str);

    while ((str != NULL) && (index < 6))
    {
        str = strtok(NULL, "|");
        strcpy(wifi_cfg[index++], str);
    }

    if (index != 6)
        return;

    wifi_config wifi_configuration;
    strcpy(wifi_configuration.network_ssid, wifi_cfg[1]);
    strcpy(wifi_configuration.network_password, wifi_cfg[2]);
    strcpy(wifi_configuration.broker_address, wifi_cfg[3]);
    wifi_configuration.broker_port = strtoul(wifi_cfg[4], NULL, 0);
    strcpy(wifi_configuration.broker_token, wifi_cfg[5]);

    bool status = wifi_set_configuration(&wifi_configuration);
    send_config_reply(status);
}

static void send_sensors_values()
{
    char values_str[128];

    sprintf(values_str, "{\"temperature\":\"%.2f\", \"humidity\":\"%.2f\","
            " \"pressure\":\"%.2f\", \"tvoc\":\"%d\", \"eco2\":\"%d\"}\r\n",
            environmental_data.temperature,
            environmental_data.humidity,
            environmental_data.pressure / 100, /* Convert pressure to hPa */
            air_quality.tvoc,
            air_quality.eco2);

    HAL_UART_Transmit(pc_uart_dev.uart_handle, (uint8_t*)values_str, strlen(values_str), HAL_MAX_DELAY);
}

static void send_device_status()
{
    char status_str[256];

    wifi_state state = wifi_get_state();
    wifi_config configuration;
    wifi_get_configuration(&configuration);

    sprintf(status_str, "{\"wifi_state\":%d, \"wifi_ssid\":\"%s\", \"wifi_password\":\"%s\","
            "\"broker_address\":\"%s\", \"broker_port\":\"%lu\", \"broker_token\":\"%s\"}\r\n",
            state,
            configuration.network_ssid,
            configuration.network_password,
            configuration.broker_address,
            configuration.broker_port,
            configuration.broker_token);

    HAL_UART_Transmit(pc_uart_dev.uart_handle, (uint8_t*)status_str, strlen(status_str), HAL_MAX_DELAY);
}

static void send_config_reply(bool reply)
{
    char reply_str[32];
    sprintf(reply_str, "{\"status\":\"%s\"}\r\n", reply ? "OK" : "FAIL");
    HAL_UART_Transmit(pc_uart_dev.uart_handle, (uint8_t*)reply_str, strlen(reply_str), HAL_MAX_DELAY);
}
