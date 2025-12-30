/**
 * @file console.h
 * @brief Command Line Interface (CLI) module for the Virtual Machine.
 *
 * This file defines the entry point for user interaction. It handles the
 * REPL (Read-Eval-Print Loop), parses commands (LOAD, RUN, DEBUG, EXIT),
 * and manages the system execution modes.
 *
 * @version 1.1
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>
#include "../inc/definitions.h"

/** @brief Maximum character length for a console input line. */
#define CONSOLE_BUFFER_SIZE 512

/**
 * @brief Command Execution Status Codes.
 * Internal status codes used by the command handlers to indicate the result
 * of a specific operation (LOAD, RUN, etc.) to the main loop.
 */
typedef enum {
	CMD_SUCCESS       = 0,  /**< Command executed successfully. The loop continues. */
	CMD_EMPTY         = 1,  /**< Input was empty (User just pressed Enter). */
	CMD_UNKNOWN       = 2,  /**< The entered command is not recognized. */
	CMD_MISSING_ARGS  = 3,  /**< Command exists (e.g., LOAD) but required argument is missing. */
	CMD_LOAD_ERROR    = 4,  /**< Loader failed to open or parse the specified file. */
	CMD_RUNTIME_ERROR = 5   /**< Generic error during execution (CPU/Memory fault). */
} CommandStatus_t;

/**
 * @brief Console System Status Codes.
 * Return codes for the main console application loop.
 */
typedef enum {
	CONSOLE_SUCCESS       = 0, /**< The console session ended normally (User typed EXIT). */
	CONSOLE_RUNTIME_ERROR = 1  /**< The console session ended due to a critical system error. */
} ConsoleStatus_t;

/**
 * @brief Starts the main Console loop (REPL).
 *
 * This is the main entry point for the UI. It initializes the interface,
 * displays the prompt, reads user input, and dispatches commands to the
 * appropriate internal handlers until the session is terminated.
 *
 * @return ConsoleStatus_t CONSOLE_SUCCESS on normal exit, or CONSOLE_RUNTIME_ERROR on failure.
 */
ConsoleStatus_t consoleStart(void);

#endif // CONSOLE_H
