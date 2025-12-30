/**
 * @file console.h
 * @brief Command Line Interface (CLI) module for the Virtual Machine.
 *
 * This file defines the entry point for user interaction. It handles the
 * REPL (Read-Eval-Print Loop), parses commands (LOAD, RUN, DEBUG, EXIT),
 * and manages the system execution modes.
 *
 * @version 1.2
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

/**
 * @brief Parses and tokenizes the raw user input.
 *
 * Cleans the input string (trims whitespace) and splits it into the command
 * and its argument.
 *
 * @param input The raw input buffer from stdin (modified in place).
 * @param command Buffer to store the extracted command (e.g., "LOAD").
 * @param argument Buffer to store the extracted argument (e.g., "file.txt").
 * @return CommandStatus_t CMD_SUCCESS if parsed, CMD_EMPTY if input was blank.
 */
CommandStatus_t parseInput(char* input, char* command, char* argument);

/**
 * @brief Handles the 'LOAD' command logic.
 *
 * Validates the filename argument and invokes the Loader subsystem to
 * inject the program into memory. Logs the operation result.
 *
 * @param argument The filename string provided by the user.
 * @return CommandStatus_t CMD_SUCCESS, CMD_MISSING_ARGS, or CMD_LOAD_ERROR.
 */
CommandStatus_t handleLoadCommand(char* argument);

/**
 * @brief Handles the 'RUN' command logic.
 *
 * Initiates the CPU execution in Normal Mode (continuous execution).
 * Logs the start and end of the execution.
 *
 * @return CommandStatus_t CMD_SUCCESS if execution finished normally, or CMD_RUNTIME_ERROR.
 */
CommandStatus_t handleRunCommand(void);

/**
 * @brief Handles the 'DEBUG' command logic.
 *
 * Starts an interactive Debug Mode session where the user can step through
 * instructions and inspect registers. Manages its own internal command loop.
 *
 * @return CommandStatus_t CMD_SUCCESS upon completion of the debug session.
 */
CommandStatus_t handleDebugCommand(void);

#endif // CONSOLE_H
