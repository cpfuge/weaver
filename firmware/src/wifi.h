#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32g0xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi size related macros
 */
#define WIFI_RX_BUFFER_SIZE 1024
#define WIFI_CFG_STR_SIZE   32

/**
 * @brief Maximum retry count for error recovery
 */
#define WIFI_INIT_MAX_RETRY    3
#define WIFI_NETWORK_MAX_RETRY 5
#define WIFI_MQTT_MAX_RETRY    5

/**
 * @brief Configuration flash address
 */
#define WIFI_CFG_FLASH_ADDRESS 0x801F800

/**
 * @brief WiFi states for the state machine
 */
typedef enum
{
    WIFI_NONE,                       /**< Do nothing */
    WIFI_INITIALIZE,                 /**< Initialize WiFi chip */
    WIFI_RESTARTING,                 /**< WiFi chip is restarting */
    WIFI_MODE_CONFIGURE,             /**< Configure WiFi chip in station mode */
    WIFI_MODE_CONFIGURING,           /**< WiFi chip is configuring in station mode */
    WIFI_NETWORK_CONNECT,            /**< Connect to WiFi network */
    WIFI_NETWORK_CONNECTING,         /**< WiFi chip is connecting to WiFi network */
    WIFI_NETWORK_CONNECTED,          /**< WiFi chip is connected to WiFi network */
    WIFI_MQTT_CONNECT,               /**< Connect to MQTT broker */
    WIFI_MQTT_CONNECTING,            /**< WiFi chip is connecting to MQTT broker */
    WIFI_MQTT_CONNECTED,             /**< WiFi chip is connected to WiFi network */
    WIFI_MQTT_PUBLISH_START,         /**< Start MQTT publish */
    WIFI_MQTT_PUBLISH,               /**< Publish measured data to MQTT broker */
    WIFI_MQTT_PUBLISH_WAIT_REPLY,    /**< Wait for publish reply from MQTT broker */
    WIFI_ERROR_INITIALIZE,           /**< WiFi chip encountered an error during initialization */
    WIFI_ERROR_NETWORK,              /**< WiFi chip encountered an error while connecting to network */
    WIFI_ERROR_MQTT_BROKER,          /**< WiFi chip encountered an error while connecting to MQTT broker */
    WIFI_ERROR_MQTT_PUBLISH          /**< WiFi chip encountered an error while publishing data to MQTT broker */
} wifi_state;

/**
 * @brief WiFi network and broker configuration
 */
typedef struct
{
    char network_ssid[WIFI_CFG_STR_SIZE];        /**< WiFi network ssid */
    char network_password[WIFI_CFG_STR_SIZE];    /**< WiFi network password */
    char broker_address[WIFI_CFG_STR_SIZE];      /**< MQTT broker ip address */
    uint32_t broker_port;                        /**< MQTT broker port */
    char broker_token[WIFI_CFG_STR_SIZE];        /**< MQTT broker authentication token */
} wifi_config;

/**
 * @brief WiFi device definition
 */
typedef struct
{
    UART_HandleTypeDef *uart_handle;           /**< UART handle connected to WiFi chip */
    uint8_t rx_buffer[WIFI_RX_BUFFER_SIZE];    /**< Received data from WiFi chip */
    uint16_t rx_index;                         /**< Current index of received data */
    wifi_state state;                          /**< Current WiFi state */
    bool lf_received;                          /**< Line feed received, parse received data */
    uint8_t init_retry_count;                  /**< Initialization retry count */
    uint8_t network_retry_count;               /**< WiFi network connection retry count */
    uint8_t mqtt_retry_count;                  /**< MQTT configuration retry count */
    uint8_t publish_retry_count;               /**< MQTT publish retry count */
    wifi_config configuration;                 /**< WiFi network and broker configuration */
} wifi_device;

/**
 * @brief Start WiFi initialization
 * @param huart: Handle of the UART device connected to the WiFi chip
 * @return: true if WiFi was started successfully, false otherwise
 */
bool wifi_init(UART_HandleTypeDef *huart);

/**
 * @brief Configure WiFi network and broker
 * @param config: Pointer to configuration structure
 */
bool wifi_set_configuration(wifi_config *config);

/**
 * @brief Returns wifi state
 */
wifi_state wifi_get_state(void);

/**
 * @brief Read the WiFi configuration
 * @param config: Pointer to wifi_config structure
 */
void wifi_get_configuration(wifi_config *config);

/**
 * @brief WiFi handler for the state machine
 */
void wifi_handler(void);

/**
 * @brief WiFi UART rx callback
 * This function is called everytime a byte was received by the UART
 * @param huart: UART handle that received data
 */
void wifi_rx_callback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_H */
