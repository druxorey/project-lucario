/**
 * @file core.h
 * @brief Core definitions and process management for the Lucario OS Minikernel.
 *
 * This header defines the Process Control Block (PCB), the process states,
 * and the main functions to initialize, start, and manage the operating
 * system's lifecycle and background execution thread.
 *
 * @version 1.1
 */

#ifndef CORE_H
#define CORE_H

#include "../definitions.h"

/**
 * @brief Initializes the core components of the Operating System.
 *
 * Sets all entries in the ProcessTable to FINISHED and resets global
 * kernel variables before the system starts accepting programs.
 *
 * @return OSStatus_t OS_SUCCESS if initialization was successful.
 */
OSStatus_t initOS(void);

/**
 * @brief Starts the Operating System background thread.
 *
 * Spawns the secondary thread (`cpuThreadWorker`) that will continuously 
 * execute the Fetch-Decode-Execute cycle and invoke the Round Robin scheduler.
 *
 * @return OSStatus_t OS_SUCCESS on success, OS_ERR_THREAD_CREATION on failure.
 */
OSStatus_t osStart(void);

/**
 * @brief Stops the Operating System background thread.
 *
 * Flags `os_running` as false and waits for the background thread to join,
 * ensuring a safe and clean shutdown of the kernel.
 */
OSStatus_t osStop(void);

/**
 * @brief Finds the first available index in the Process Table.
 *
 * Scans the ProcessTable for an entry with the FINISHED state, which
 * indicates the slot is free to be reused by a new process.
 *
 * @return int The index of the free PCB, or -1 if the table is full.
 */
int getFreePCBIndex(void);

/**
 * @brief Creates a new process from an executable file.
 *
 * Orchestrates the process creation: finds a free PCB, allocates memory,
 * loads the program from the VFS to RAM, initializes the CPU context, 
 * and sets the state to READY.
 *
 * @param progName The filename of the program to execute (e.g., "prog1.txt").
 * @return OSStatus_t OS_SUCCESS or an error code if resources are unavailable.
 */
OSStatus_t createProcess(char* progName);

extern int currentActiveProcess;  /**< @brief Index of the currently active process in the Process Table. */
extern bool osYield;              /**< @brief Flag to request a context switch from the CPU to the OS. */

#endif /* CORE_H */
