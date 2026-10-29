/**
 * @file stop.h
 * @author Madden Wynans
 * @brief Lets thread request to stop the program
 * @date 2026-10-28
 * @copyright Copyright (c) 2026
 */
#ifndef STOP_H
#define STOP_H

/**
 * @brief Initializes the stop semaphore
 * @warning Must be called before any other stop function
 */
void Stop_init(void);

/**
 * @brief Cleans up the stop semaphore
 */
void Stop_cleanup(void);

/**
 * @brief Request that the program stop
 */
void Stop_request(void);

/**
 * @brief Blocks the calling thread until Stop_request() is called
 * @details Returns immeditely if stop already requested
 */
void Stop_waitForRequest(void);
#endif