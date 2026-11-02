/**
 * @file gpio_read.h
 * @brief Reads GPIO input levels and edge events
 * @details Works on a line request created by GPIO_Init(): reads a pin's current
 *          level, or blocks until rising/falling edges occur.
 */
#ifndef GPIO_READ_H
#define GPIO_READ_H

#include <gpiod.h>
#include <stddef.h>
#include <stdint.h>

/** Describes a single edge event reported by GPIO_WaitEdges() */
typedef struct
{
    unsigned int line_offset; /**< GPIO line that changed */
    int value;                /**< New state: 1 = rising edge, 0 = falling edge */
    uint64_t timestamp_ns;    /**< Kernel timestamp (CLOCK_MONOTONIC) in nanoseconds */
} GPIO_EdgeEvent;

/**
 * @brief Reads the current value of a single GPIO pin that was previously
 *        requested with GPIO_Init()
 *
 * @param request     Handle returned by GPIO_Init()
 * @param line_offset GPIO line number to read. Must be one of the lines
 *                    passed to GPIO_Init() for this request.
 * @return int 1 if the pin is high, 0 if low, -1 on error
 */
int GPIO_Read(struct gpiod_line_request *request, unsigned int line_offset);

/**
 * @brief Reads the current values of several GPIO pins at the same instant
 *
 * @param request      Handle returned by GPIO_Init()
 * @param line_offsets GPIO line numbers to read (all from this request)
 * @param num_lines    number of lines in line_offsets
 * @param values       filled in with 1 (high) or 0 (low) for each line
 * @return int 0 on success, -1 on error
 */
int GPIO_ReadLines(
    struct gpiod_line_request *request,
    const unsigned int *line_offsets,
    size_t num_lines,
    int *values);

/**
 * @brief Waits for edge events on any pin in the request, then returns all
 *        that are queued (up to max_events), oldest first
 *
 * Reading the queue in batches keeps up with fast-changing inputs, so the
 * kernel's event queue does not overflow.
 *
 * When the system is busy, the kernel may merge quick edges on a line and
 * deliver events from different lines out of order. To know the pins' state,
 * read their levels with GPIO_ReadLines() rather than replaying the events.
 *
 * The request must have been created with GPIO_Init() using an edge setting
 * other than GPIOD_LINE_EDGE_NONE.
 *
 * @param request    Handle returned by GPIO_Init()
 * @param timeout_ms how long to wait in milliseconds (negative = wait forever)
 * @param events     array filled in with the events received
 * @param max_events number of elements in the events array
 * @return int number of events received (> 0), 0 on timeout (or if
 *             interrupted by a signal), -1 on error
 */
int GPIO_WaitEdges(
    struct gpiod_line_request *request,
    int timeout_ms,
    GPIO_EdgeEvent *events,
    size_t max_events);

#endif
