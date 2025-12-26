/**
 * @file logger.h
 * @brief Logging subsystem interface.
 *
 * Handles writing execution logs to a file and standard output,
 * supporting thread safety and specific formats for debug/interrupts.
 *
 * @version 1.0
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
	LOG_DEBUG   = 3, /**< Detailed debug information */
} LogLevel_t;

/**
 * @brief Interrupt Codes.
 * Used to identify specific system interrupts.
 */
#define COLOR_WARNING "\x1b[33m"
#define COLOR_ERROR   "\x1b[31m"
#define COLOR_DEBUG   "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

/**
 * @brief Initializes the logging system.
 * Opens the log file (creating it if it doesn't exist) and initializes
 * the mutex for thread safety.
 *
 * @param filename Path to the output log file.
 */
void loggerInit(void);

/**
 * @brief Closes the logging system.
 * Flushes any pending writes, closes the file handle, and destroys the mutex.
 */
void loggerClose(void);

/**
 * @brief Writes a generic message to the log.
 * This function is thread-safe.
 *
 * @param level The severity level (LOG_INFO, LOG_ERROR, etc.).
 * @param message The string message to record.
 */
void loggerLog(LogLevel_t level, const char* message);

/**
 * @brief Logs a System Interrupt event.
 * Prints to stdout AND writes to log file.
 *
 * @param code The interrupt code (e.g., IC_OVERFLOW for Overflow).
 */
void loggerLogInterrupt(InterruptCode_t code);

#endif // LOGGER_H
