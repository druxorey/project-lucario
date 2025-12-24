#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

#include "../inc/console.h"
#include "../inc/loader.h"
#include "../inc/logger.h"
#include "../inc/cpu.h"

char* trimWhitespace(char* string) {
	char* source = string;
	char* destiny = string;
	int inWord = 0;

	while (*source && isspace((unsigned char)*source)) {
		source++;
	}

	while (*source) {
		if (isspace((unsigned char)*source)) {
			// If we are inside a word, write a single space
			if (inWord) {
				*destiny++ = ' ';
				inWord = 0;
			}
		} else {
			// Copy non-space characters
			*destiny++ = *source;
			inWord = 1;
		}
		source++;
	}

	if (destiny > string && isspace((unsigned char)*(destiny - 1))) {
		destiny--;
	}

	*destiny = '\0';
	return string;
}


void splitInput(const char* input, char* command, char* argument) {
	command[0] = '\0';
	argument[0] = '\0';

	// %s reads one word (up to the first space)
	// %[^\n] reads "everything up to the newline" (the rest of the string)
	// The space between %s and %[^\n] consumes the separating space
	int matches = sscanf(input, "%19s %99[^\n]", command, argument);

	// sscanf returns how many variables it successfully filled
	// If input is "RUN", matches will be 1 (argument remains empty due to the initial clearing)
	// If input is "LOAD file", matches will be 2
}


int consoleStart(void) {
	printf("\033[2J\033[H");
	char buffer[CONSOLE_BUFFER_SIZE];
	printf("=== LUCARIO REPL ===\n");
	
	while(true) {
		printf("LUCARIO > ");
		if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
		
		ConsoleStatus_t output = consoleProcessCommand(buffer);
		#ifdef DEBUG
		printf("\x1b[36mDEBUG: Command Output = [%d] \x1b[0m\n", output);
		#endif
		if (output == CMD_EXIT) break;
	}

	return 0;
}


ConsoleStatus_t consoleProcessCommand(char* input) {
	char logBuffer[256];
	input[strcspn(input, "\n")] = 0;
	input = trimWhitespace(input);

	if (strlen(input) == 0) return CMD_EMPTY;

	char command[CONSOLE_BUFFER_SIZE], argument[CONSOLE_BUFFER_SIZE];
	splitInput(input, command, argument);

	#ifdef DEBUG
	printf("\x1b[36mDEBUG: Command = [%s]; Argument = [%s] \x1b[0m\n", command, argument);
	#endif

	if (strcmp(command, "EXIT") == 0) {
		loggerLog(LOG_INFO, "System shutdown requested via CLI (EXIT command).");
		return CMD_EXIT;
	}

	if (strcmp(command, "LOAD") == 0) {
		if (strlen(argument) == 0) {
			printf("Error: Missing filename.\n");
			loggerLog(LOG_WARNING, "User attempted LOAD command without filename argument.");
			return CMD_MISSING_ARGS;
		}

		if (loaderLoadProgram(argument) == LOADER_SUCCESS) {
			printf("File '%s' loaded successfully.\n", argument);
			snprintf(logBuffer, sizeof(logBuffer), "Program loaded successfully: %s", argument);
			loggerLog(LOG_INFO, logBuffer);
			return CMD_SUCCESS;
		} else {
			printf("Error loading file.\n");
			snprintf(logBuffer, sizeof(logBuffer), "Failed to load program file: %s", argument);
			loggerLog(LOG_ERROR, logBuffer);
			return CMD_LOAD_ERROR;
		}
	}

	if (strcmp(command, "RUN") == 0) {
		printf("Executing in Normal Mode...\n");
		loggerLog(LOG_INFO, "Starting execution in Normal Mode.");

		if (cpuRun()) {
			printf("Execution finished.\n");
			loggerLog(LOG_INFO, "Normal Mode execution finished successfully.");
			return CMD_SUCCESS;
		}
		
		loggerLog(LOG_ERROR, "Normal Mode execution terminated abnormally.");
		return CMD_RUNTIME_ERROR;
	}

	if (strcmp(command, "DEBUG") == 0) {
		printf("Executing in Debug Mode...\n");
		loggerLog(LOG_INFO, "Starting execution in Debug Mode.");

		if (runDebugMode()) {
			printf("Execution finished.\n");
			loggerLog(LOG_INFO, "Debug Mode session finished.");
			return CMD_SUCCESS;
		}
		
		loggerLog(LOG_ERROR, "Debug Mode execution terminated abnormally.");
		return CMD_RUNTIME_ERROR;
	}

	printf("Unknown command: %s\n", input);
	snprintf(logBuffer, sizeof(logBuffer), "Unknown command received: %s", command);
	loggerLog(LOG_WARNING, logBuffer);
	
	return CMD_UNKNOWN;
}


void printCpuState() {
	printf("[DEBUG] PC:%03d | IR:%08d | AC:%08d\n", CPU.PSW.pc, CPU.IR, CPU.AC);
}


int runDebugMode() {
	printf("--- DEBUG MODE STARTED ---\n");
	printf("Press [ENTER] to step, 'q' to quit debug mode.\n");
	
	char debugBuf[10];
	bool running = true;
	
	while(running) {
		printCpuState();
		bool active = cpuStep();
		
		if (!active) {
			printf("--- PROGRAM FINISHED (HALT) ---\n");
			break;
		}

		printf(">> ");
		if (fgets(debugBuf, sizeof(debugBuf), stdin) != NULL) {
			if (debugBuf[0] == 'q') break;
		}
	}
	return 0;
}
