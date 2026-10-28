/**
 * @file led.h
 * @author Madden Wynans 
 * @brief HAL for the BeagleY-AI Status LEDs
 * 
 * Provides functions for initializing, controlling, and cleaning up the status LEDs
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef LED_H
#define LED_H

#include <stdbool.h>

/**
 * @brief Initializes the LED Hardware
 * 
 */
void LED_init(void);

/**
 * @brief Cleans up the LED hardware
 * 
 * Turns both the LEDs off before application exits
 */
void LED_cleanup(void);

/**
 * @brief Turns the green LED on or off
 * 
 * @param on true turns LED on, false turns LED off
 */
void LED_setGreen(bool on);

/**
 * @brief Turns the red LED on or off
 * 
 * @param on true turns LED on, false turns LED off
 */
void LED_setRed(bool on);


#endif
