#include <gpiod.h>
#include <stdio.h>
#include "hal/gpio_init.h"

// Kernel-side queue for edge events. The kernel default (16 per line) can
// overflow when an input changes quickly, silently dropping events.
#define EDGE_EVENT_QUEUE_SIZE 256

struct gpiod_line_request *GPIO_Init(
    const char *chip_path,
    unsigned int *line_offsets,
    size_t num_lines,
    enum gpiod_line_direction direction,
    enum gpiod_line_edge edge,
    unsigned long debounce_us)
{
    struct gpiod_chip *chip = NULL;
    struct gpiod_line_settings *settings = NULL;
    struct gpiod_line_config *line_config = NULL;
    struct gpiod_request_config *request_config = NULL;
    struct gpiod_line_request *request = NULL;

    if (chip_path == NULL || line_offsets == NULL || num_lines == 0)
    {
        fprintf(stderr, "GPIO_Init: bad argument\n");
        return NULL;
    }

    // Edge detection only makes sense on input pins
    if (edge != GPIOD_LINE_EDGE_NONE && direction != GPIOD_LINE_DIRECTION_INPUT)
    {
        fprintf(stderr, "GPIO_Init: edge detection requires GPIOD_LINE_DIRECTION_INPUT\n");
        return NULL;
    }

    // Open GPIO chip
    chip = gpiod_chip_open(chip_path);

    if (chip == NULL)
    {
        perror("GPIO chip open failed");
        return NULL;
    }

    settings = gpiod_line_settings_new();
    line_config = gpiod_line_config_new();
    request_config = gpiod_request_config_new();

    if (settings == NULL || line_config == NULL || request_config == NULL)
    {
        perror("GPIO config allocation failed");
        goto cleanup;
    }

    // Line settings
    gpiod_line_settings_set_direction(settings, direction);
    gpiod_line_settings_set_edge_detection(settings, edge);

    if (direction == GPIOD_LINE_DIRECTION_INPUT)
    {
        // Internal pull-ups so idle reads high (needed for the PEC12R encoder).
        // Remove this if you have external pull-ups or need a floating input.
        gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

        if (debounce_us > 0)
        {
            gpiod_line_settings_set_debounce_period_us(settings, debounce_us);
        }
    }
    else if (direction == GPIOD_LINE_DIRECTION_OUTPUT)
    {
        gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);
    }

    // Configure GPIO lines
    if (gpiod_line_config_add_line_settings(
            line_config,
            line_offsets,
            num_lines,
            settings) < 0)
    {
        perror("GPIO line config failed");
        goto cleanup;
    }

    // Set consumer name
    gpiod_request_config_set_consumer(request_config, "GPIO_HAL");

    if (edge != GPIOD_LINE_EDGE_NONE)
    {
        gpiod_request_config_set_event_buffer_size(request_config, EDGE_EVENT_QUEUE_SIZE);
    }

    // Request GPIO lines
    request = gpiod_chip_request_lines(
        chip,
        request_config,
        line_config);

    if (request == NULL)
    {
        perror("GPIO line request failed");
    }

cleanup:
    // Clean up temporary configuration objects
    gpiod_request_config_free(request_config);
    gpiod_line_config_free(line_config);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);

    return request;
}