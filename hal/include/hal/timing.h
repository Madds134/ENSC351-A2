/**
 * @file timing.h
 * @author Madden Wynans
 * @brief Shared timing helpers
 *
 * Provides a millisecond sleep used by the modules' threads and test loops.
 * @copyright Copyright (c) 2026
 */
#ifndef TIMING_H
#define TIMING_H

/**
 * @brief Sleeps the current thread for a specified number of ms
 * @param delayInMs Delay duration in ms
 * @note This function is based on example provided in ENSC 351 assignment specification
 */
void Timing_sleepMs(long long delayInMs);

#endif
