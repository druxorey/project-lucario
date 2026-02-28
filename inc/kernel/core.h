/**
 * @file core.h
 * @brief Core definitions and process management for the Lucario OS Minikernel.
 *
 * This header defines the Process Control Block (PCB), the process states,
 * and the main functions to initialize, start, and manage the operating
 * system's lifecycle and background execution thread.
 *
 * @version 1.0
 */

#ifndef CORE_H
#define CORE_H

#include "../hardware/cpu.h"

/**
 * @brief Represents the possible lifecycle states of a process in the OS.
 */
typedef enum {
    NEW,                /**< Process is being created and loaded into memory. */
    READY,              /**< Process is in the queue, waiting for CPU time. */
    EXECUTING,          /**< Process is currently running on the CPU. */
    BLOCKED,            /**< Process is sleeping (SVC 4) or waiting for an event. */
    BLOCKED_IO,         /**< Process is waiting for the user to open the monitor for I/O. */
    FINISHED            /**< Process has terminated or was aborted due to an error. */
} ProcessState;

/**
 * @brief Operating System Status Codes.
 *
 * Used to indicate the result of kernel-level operations such as
 * process creation, thread initialization, and resource allocation.
 */
typedef enum {
    OS_SUCCESS             = 0, /**< Operation completed successfully. */
    OS_ERR_MAX_PROCESSES   = 1, /**< Cannot create process: Process table is full. */
    OS_ERR_MEMORY          = 2, /**< Cannot create process: Insufficient contiguous RAM blocks. */
    OS_ERR_DISK            = 3, /**< Cannot create process: File not found or disk error. */
    OS_ERR_THREAD          = 4  /**< Failed to create the background OS thread. */
} OSStatus_t;

/**
 * @brief Process Control Block (PCB).
 *
 * Data structure used by the OS to store all the information about a process.
 * It holds the CPU context, memory boundaries, and scheduling metadata.
 */
typedef struct {
    int pid;                    /**< Process ID (Unique identifier). */
    ProcessState state;         /**< Current state of the process. */
    CPU_t context;              /**< Snapshot of the CPU registers (PC, AC, SP, etc.). */
    char programName[256];      /**< Name of the executable file (e.g., "calc.txt"). */
    int startBlock;             /**< Starting RAM block index assigned to this process. */
    int blockCount;             /**< Number of contiguous RAM blocks assigned. */
    int sleepTics;              /**< Remaining CPU cycles to sleep (used by SVC 4). */
} PCB_t;

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

extern PCB_t ProcessTable[MAX_PROCESSES]; /**< @brief The System Process Table. */
extern int currentActiveProcess;          /**< @brief Index of the process currently executing in the CPU. */
extern bool osRunning;                    /**< @brief Flag to control the lifecycle of the background OS thread. */

#endif /* CORE_H */
