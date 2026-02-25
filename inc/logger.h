/**
 * @file logger.h
 * @brief Logging subsystem interface.
 *
 * Handles writing execution logs to a file and standard output,
 * supporting thread safety and specific formats for debug/interrupts.
 *
 * @version 1.2
 */

#ifndef LOGGER_H
#define LOGGER_H

#include "../inc/definitions.h"

/**
 * @brief Logging Severity Levels.
 * Used to categorize messages in the log file.
 */
typedef enum {
	LOG_INFO    = 0, /**< Standard informational message */
	LOG_WARNING = 1, /**< Warning conditions */
	LOG_ERROR   = 2, /**< Critical errors */
} LogLevel_t;

/**
 * @brief Logging Severity Levels.
 * Used to categorize messages in the log file.
 */
typedef enum {
	HARDWARE_LOG  = 0, /**< Warning conditions */
	KERNEL_LOG    = 1, /**< Standard informational message */
} LogType_t;

/**
 * @brief Initializes the logging system.
 * Opens the log file (creating it if it doesn't exist) and initializes
 * the mutex for thread safety.
 */
void loggerInit(void);

/**
 * @brief Closes the logging system.
 * Flushes any pending writes, closes the file handle, and destroys the mutex.
 */
void loggerClose(void);

/**
 * @brief Writes a generic message to the log (Hardware Source).
 * This function is thread-safe.
 *
 * @param level The severity level (LOG_INFO, LOG_ERROR, etc.).
 * @param message The string message to record.
 */
void loggerLogHardware(LogLevel_t level, const char* message);

/**
 * @brief Writes a generic message to the log (Kernel Source).
 *
 * @param level The severity level.
 * @param message The string message to record.
 */
void loggerLogKernel(LogLevel_t level, const char* message);

/**
 * @brief Logs a System Interrupt event (Hardware Source).
 * Prints to stdout AND writes to log file.
 *
 * @param code The hardware interrupt code.
 */
void loggerLogInterrupt(InterruptCode_t code);

#endif // LOGGER_H
