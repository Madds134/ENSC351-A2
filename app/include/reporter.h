/**
 * @file reporter.h
 * @author Madden Wynans
 * @brief Once per second analysis and terminal output
 * @version 0.1
 * @date 2026-10-28
 *  * @copyright Copyright (c) 2026
 */
#ifndef REPORTER_H
#define REPORTER_H

/**
 * @brief Starts the reporting thread
 * @warning Period_init() and Sampler_init() must be called first
 */
void Reporter_init(void);

/**
 * @brief Stops the reporting thread
 */
void Reporter_cleanup(void);

/**
 * @brief Gets the number of dips found in the prevous second's samples
 * @return Dip count from the most recent report
 */
int Reporter_getDipCount(void);

#endif