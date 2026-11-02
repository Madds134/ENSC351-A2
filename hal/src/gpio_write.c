#include "hal/gpio_write.h"

int GPIO_Write(struct gpiod_line_request *request, unsigned int line_offset, int value)
{
    if (request == NULL)
    {
        return -1;
    }
    return gpiod_line_request_set_value(
               request,
               line_offset,
               value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE) == 0 ? 0 : -1;
}