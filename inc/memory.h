/**
 * @file memory.h
 * @brief Simulation memory RAM and definition words.
 *
 * Defines the memory structure and constants for the simulation environment.
 *
 * @version 1.0
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "../inc/definitions.h"

/** 
 * @brief Initializes memory subsystem (mutexes, etc.). 
 * Initializes the mutex for thread-safe memory access.
 */
void memoryInit(void);

/**
 *  @brief Checks if address is outside OS-reserved region and within RAM. 
 *  Returns true if address is valid for non-OS use.
 */
bool isNotSOMemory(address addr);

/** 
 * @brief  Validates user-mode access against RB/RL bounds. 
 * Returns true if access is invalid for user mode.
*/
bool accessIsInvalid(address addr);

/** 
 * @brief Thread-safe memory read. 
 * Returns data at address or -1 on error.
 */
word read_mem(address addr);

/** 
 * @brief  Thread-safe memory write. No-op on error. 
 * Writes data to address if valid.
*/
void write_mem(address addr, word data);

#endif