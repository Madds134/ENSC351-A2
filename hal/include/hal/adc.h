/**
 * @file adc.h
 * @author Madden Wynans 
 * @brief HAL for the MCP3208 ADC
 * 
 * Provides functions for initializing, controlling, and cleaning up the communication with the MCP3208
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

// Returned by ADC_read_channel() when the SPI transfer fails
#define ADC_READ_ERROR (-1)

// Largest value of a 12-bit conversion
#define ADC_MAX_READING 4095

/**
 * @brief Initializes the SPI Hardware to communicate with the ADC
 * 
 * Must be called before ADC_read_channel()
 */
void ADC_init(void);

/**
 * @brief Cleans up the SPI hardware
 * 
 */
void ADC_cleanup(void);

/**
 * @brief Reads a single ADC channel.
 *
 * Performs an SPI transaction with the MCP3208 and returns the
 * 12-bit conversion result for the requested channel.
 *
 * @param channel ADC channel number to read, from 0 through 7.
 *
 * @return 12-bit ADC conversion value in the range 0 through ADC_MAX_READING,
 *         or ADC_READ_ERROR if the SPI transfer fails.
 */
int ADC_read_channel(uint8_t channel);

#endif
