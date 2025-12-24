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

/** Initializes memory subsystem (mutexes, etc.). */
void memoryInit(void);

/** Checks if address is outside OS-reserved region and within RAM. */
bool is_not_SO_memory(address addr);

/** Validates user-mode access against RB/RL bounds. */
bool access_is_invalid(address addr);

/** Thread-safe memory read. Returns -1 on error. */
word read_mem(address addr);

/** Thread-safe memory write. No-op on error. */
void write_mem(address addr, word data);

#endif