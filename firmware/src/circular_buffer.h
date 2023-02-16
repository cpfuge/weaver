#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32g0xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Macro for circular buffer definition
 * @param name: Circular buffer name
 * @param size: Circular buffer size
 */
#define CIRCULAR_BUFFER_DEF(name, size)      \
    static uint8_t name##_data_buffer[size]; \
    static circular_buffer name = {          \
        .buffer   = name##_data_buffer,      \
        .head     = 0,                       \
        .tail     = 0,                       \
        .capacity = size                     \
    }

/**
 * @brief Circular buffer structure type
 */
typedef struct
{
    uint8_t *buffer;            /**< Pointer to storage buffer */
    uint16_t head;              /**< Head position             */
    uint16_t tail;              /**< Tail position             */
    const uint16_t capacity;    /**< Buffer capacity           */
} circular_buffer;

/**
 * @brief Push a single byte to the circular buffer
 * @param cbuf: Pointer to circular buffer
 * @param data: Single byte value that will be added to the buffer
 */
void circular_buffer_push(circular_buffer *cbuf, uint8_t data);

/**
 * @brief Get a single byte from the circular buffer
 * @param cbuf: Pointer to circular buffer
 * @param data: Single byte value that will be read from the buffer
 */
void circular_buffer_pop(circular_buffer *cbuf, uint8_t *data);

/**
 * @brief Buffer has data that can be read
 * @param cbuf: Pointer to circular buffer
 * @return: true if data is available to be read, false otherwise
 */
bool circular_buffer_has_data(circular_buffer *cbuf);

#ifdef __cplusplus
}
#endif

#endif /* CIRCULAR_BUFFER_H */
