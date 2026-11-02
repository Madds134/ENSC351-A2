/**
 * @file flash_control.h
 * @brief Lets the rotary encoder set the LED flash rate (uses a thread)
 * @details Starts the LED flashing at 10Hz. Each clockwise step adds 1Hz and
 *          each counter-clockwise step removes 1Hz, limited to 0..500Hz.
 */
#ifndef FLASH_CONTROL_H
#define FLASH_CONTROL_H

/**
 * @brief Starts the LED flashing and the encoder-reading thread
 * @details Exits the program if the GPIO lines or thread cannot be set up
 */
void FlashControl_init(void);

/**
 * @brief Stops the encoder thread, turns the LED off, and releases the GPIO lines
 */
void FlashControl_cleanup(void);

/**
 * @brief Gets the current LED flash rate
 * @return Flash rate in Hz
 */
int FlashControl_getHz(void);

#endif
