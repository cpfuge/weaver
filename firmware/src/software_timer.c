#include "software_timer.h"

void timer_start(software_timer *timer)
{
    if (timer == NULL)
        return;

    timer->start = HAL_GetTick();
}

bool timer_is_expired(software_timer *timer)
{
    if (timer == NULL)
        return false;

    return (HAL_GetTick() - timer->start >= timer->expire_interval);
}
