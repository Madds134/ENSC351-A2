#include <errno.h>
#include <gpiod.h>
#include <stdio.h>
#include "hal/gpio_read.h"

#define NS_PER_MS 1000000
#define WAIT_FOREVER (-1)
#define MAX_LINES_PER_READ 64

int GPIO_Read(struct gpiod_line_request *request, unsigned int line_offset)
{
    if (request == NULL)
    {
        fprintf(stderr, "GPIO_Read: invalid request\n");
        return -1;
    }

    enum gpiod_line_value value =
        gpiod_line_request_get_value(request, line_offset);

    if (value == GPIOD_LINE_VALUE_ERROR)
    {
        perror("GPIO read failed");
        return -1;
    }

    return (value == GPIOD_LINE_VALUE_ACTIVE) ? 1 : 0;
}

int GPIO_ReadLines(
    struct gpiod_line_request *request,
    const unsigned int *line_offsets,
    size_t num_lines,
    int *values)
{
    if (request == NULL || line_offsets == NULL || values == NULL ||
        num_lines == 0 || num_lines > MAX_LINES_PER_READ)
    {
        fprintf(stderr, "GPIO_ReadLines: invalid argument\n");
        return -1;
    }

    enum gpiod_line_value raw_values[MAX_LINES_PER_READ];
    if (gpiod_line_request_get_values_subset(request, num_lines, line_offsets, raw_values) < 0)
    {
        perror("GPIO read of multiple lines failed");
        return -1;
    }

    for (size_t i = 0; i < num_lines; i++)
    {
        values[i] = (raw_values[i] == GPIOD_LINE_VALUE_ACTIVE) ? 1 : 0;
    }
    return 0;
}

static void copyEvent(struct gpiod_edge_event *source, GPIO_EdgeEvent *destination)
{
    destination->line_offset = gpiod_edge_event_get_line_offset(source);
    destination->timestamp_ns = gpiod_edge_event_get_timestamp_ns(source);
    destination->value =
        (gpiod_edge_event_get_event_type(source) == GPIOD_EDGE_EVENT_RISING_EDGE) ? 1 : 0;
}

int GPIO_WaitEdges(
    struct gpiod_line_request *request,
    int timeout_ms,
    GPIO_EdgeEvent *events,
    size_t max_events)
{
    if (request == NULL || events == NULL || max_events == 0)
    {
        fprintf(stderr, "GPIO_WaitEdges: invalid argument\n");
        return -1;
    }

    int64_t timeout_ns = (timeout_ms < 0) ? WAIT_FOREVER : (int64_t)timeout_ms * NS_PER_MS;

    int ret = gpiod_line_request_wait_edge_events(request, timeout_ns);
    if (ret < 0)
    {
        if (errno == EINTR)
        {
            return 0; // interrupted by a signal, treat like a timeout
        }
        perror("GPIO wait for edge failed");
        return -1;
    }
    if (ret == 0)
    {
        return 0; // timeout
    }

    struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(max_events);
    if (buffer == NULL)
    {
        perror("GPIO event buffer allocation failed");
        return -1;
    }

    int num_events = gpiod_line_request_read_edge_events(request, buffer, max_events);
    if (num_events < 0)
    {
        perror("GPIO read edge events failed");
    }

    for (int i = 0; i < num_events; i++)
    {
        copyEvent(gpiod_edge_event_buffer_get_event(buffer, i), &events[i]);
    }

    gpiod_edge_event_buffer_free(buffer);
    return num_events;
}
