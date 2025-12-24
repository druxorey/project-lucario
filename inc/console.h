/**
 * @file console.h
 * @brief Command Line Interface (CLI) module for the Virtual Machine.
 *
 * This file defines the functions and structures responsible for handling
 * user interaction, parsing commands (LOAD, RUN, DEBUG), and managing
 * the execution modes (Normal vs. Debugger).
 *
 * @version 1.0
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include "../inc/definitions.h"

/** @brief Maximum character length for a console input line. */
#define CONSOLE_BUFFER_SIZE 256

/**
 * @brief Console Status Codes.
 * Used to indicate the result of a processed command to the main loop.
 */
typedef enum {
	CMD_SUCCESS       = 0,  /**< Command executed successfully. The loop continues. */
	CMD_EXIT          = 1,  /**< User requested to exit (EXIT command). Break the loop. */
	CMD_EMPTY         = 2,  /**< Input was empty (User just pressed Enter). */
	CMD_UNKNOWN       = 3,  /**< The entered command is not recognized. */
	CMD_MISSING_ARGS  = 4,  /**< Command exists (e.g., LOAD) but required argument is missing. */
	CMD_LOAD_ERROR    = 5,  /**< Loader failed to open or parse the specified file. */
	CMD_RUNTIME_ERROR = 6   /**< Generic error during execution (CPU/Memory fault). */
} ConsoleStatus_t;

/**
 * @brief Starts the main Console loop.
 *
 * Initializes the CLI, displays the prompt, and continuously waits for
 * user input until the EXIT command is issued.
 *
 * @return 0 on normal termination, non-zero on error.
 */
int consoleStart(void);

/**
 * @brief Processes a single line of input from the user.
 *
 * Parses the raw input string, identifies the command (LOAD, RUN, DEBUG, EXIT),
 * and dispatches the execution to the corresponding subsystem.
 *
 * @param input The raw string entered by the user (modifiable).
 * @return A ConsoleStatus_t code indicating the outcome of the operation.
 */
ConsoleStatus_t consoleProcessCommand(char *input);

/**
 * @brief Enters the interactive Debug Mode.
 *
 * Executes the program currently in memory step-by-step. It pauses after
 * each instruction to display the CPU state and waits for user confirmation.
 *
 * @return 0 on successful completion, non-zero if interrupted or on error.
 */
int runDebugMode(void);

/**
 * @brief Prints the current state of the CPU registers.
 *
 * Displays the values of the Program Counter (PC), Accumulator (AC),
 * Instruction Register (IR), and other relevant flags. Used primarily
 * in Debug Mode.
 */
void printCpuState(void);

#endif // CONSOLE_H
