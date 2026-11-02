/**
 * @file gpio_write.h
 * @brief Drives GPIO output pins
 * @details Sets the level of an output line requested with GPIO_Init().
 */
#ifndef GPIO_WRITE_H
#define GPIO_WRITE_H

#include <gpiod.h>

/**
 * @brief Sets the value of a single output pin requested with GPIO_Init()
 *
 * @param request     Handle returned by GPIO_Init() (direction OUTPUT)
 * @param line_offset GPIO line number to write
 * @param value       0 = low, nonzero = high
 * @return int 0 on success, -1 on error
 */
int GPIO_Write(struct gpiod_line_request *request, unsigned int line_offset, int value);

#endif