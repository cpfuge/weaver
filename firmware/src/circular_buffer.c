#include "circular_buffer.h"

void circular_buffer_push(circular_buffer *cbuf, uint8_t data)
{
    if (cbuf == NULL)
        return;

    cbuf->buffer[cbuf->head] = data;

    cbuf->head++;
    if (cbuf->head >= cbuf->capacity)
    {
        cbuf->head = 0;
    }
}

void circular_buffer_pop(circular_buffer *cbuf, uint8_t *data)
{
    if (cbuf == NULL)
        return;

    *data = cbuf->buffer[cbuf->tail];

    cbuf->tail++;
    if (cbuf->tail >= cbuf->capacity)
    {
        cbuf->tail = 0;
    }
}

bool circular_buffer_has_data(circular_buffer *cbuf)
{
    return (cbuf->head != cbuf->tail);
}
