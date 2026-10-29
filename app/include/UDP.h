/**
 * @file UDP.h
 * @author Madden Wynans
 * @brief UDP on port 12345
 * @version 0.1
 * @date 2026-10-28
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef UDP_H
#define UDP_H

/**
 * @brief Opens the UDP socket and starts the server
 */
void UDP_init(void);

/**
 * @brief Closes the UDP socket after the server thread finishes
 */
void UDP_cleanup(void);



#endif