/**
 * @file led_flash.h
 * @brief HAL for the LED emitter on GPIO25 (header pin 22)
 * @details Flashes the LED at a set rate from a background thread, which is
 *          hidden inside this module.
 */
#ifndef LED_FLASH_H
#define LED_FLASH_H

/**
 * @brief Requests the LED's GPIO line and starts the flashing thread
 * @details The LED starts off (0Hz) until LedFlash_setHz() is called
 * @return 0 on success, -1 on failure
 */
int LedFlash_init(void);

/**
 * @brief Stops the flashing thread, turns the LED off, and releases the GPIO line
 */
void LedFlash_cleanup(void);

/**
 * @brief Sets the flash rate
 * @details Takes effect immediately. Setting the current rate again does
 *          nothing, so the flashing is not interrupted.
 * @param hz Full on/off cycles per second. 0 (or negative) turns the LED off.
 */
void LedFlash_setHz(int hz);

#endif
