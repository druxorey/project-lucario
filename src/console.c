#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

#include "../inc/console.h"
#include "../inc/loader.h"
#include "../inc/logger.h"
#include "../inc/cpu.h"

char logBuffer[LOG_BUFFER_SIZE];

static char* trimWhitespace(char* string) {
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


static CommandStatus_t parseInput(char* input, char* command, char* argument) {
	command[0] = '\0';
	argument[0] = '\0';
	input[strcspn(input, "\n")] = 0;
	input = trimWhitespace(input);

	if (strlen(input) == 0) return CMD_EMPTY;

	// %s reads one word (up to the first space)
	// %[^\n] reads "everything up to the newline" (the rest of the string)
	// The space between %s and %[^\n] consumes the separating space
	sscanf(input, "%19s %99[^\n]", command, argument);
	// sscanf returns how many variables it successfully filled
	// If input is "RUN", matches will be 1 (argument remains empty due to the initial clearing)
	// If input is "LOAD file", matches will be 2
	
	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Parsed -> Command: [%s]; Argument: [%s] \x1b[0m\n", command, argument);
	#endif

	return CMD_SUCCESS;
}


static CommandStatus_t handleLoadCommand(char* argument) {
	if (strlen(argument) == 0) {
		printf("Error: Missing filename.\n");
		loggerLog(LOG_WARNING, "User attempted LOAD command without filename argument");
		return CMD_MISSING_ARGS;
	}

	ProgramInfo_t info = loadProgram(argument);

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: LOAD processed. Status = [%d]\x1b[0m\n", info.status);
	#endif

	if (info.status == LOAD_SUCCESS) {
		printf("File '%s' loaded successfully.\n", argument);
		snprintf(logBuffer, sizeof(logBuffer), "Program loaded: %s (Words: %d, Start: %d)", argument, info.wordCount, info._start);
		loggerLog(LOG_INFO, logBuffer);
		return CMD_SUCCESS;
	}

	printf("Error loading file.\n");
	snprintf(logBuffer, sizeof(logBuffer), "Failed to load program file: %s", argument);
	loggerLog(LOG_ERROR, logBuffer);
	return CMD_LOAD_ERROR;
}


static CommandStatus_t handleRunCommand(void) {
	printf("Executing in Normal Mode...\n");
	loggerLog(LOG_INFO, "Starting execution in Normal Mode");

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: RUN received. Starting CPU execution.\x1b[0m\n");
	#endif

	if (cpuRun() == 0) { // Asumiendo que cpuRun retorna 0 en éxito
		printf("Execution finished.\n");
		loggerLog(LOG_INFO, "Normal Mode execution finished successfully");
		return CMD_SUCCESS;
	}

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: RUN execution terminated abnormally.\x1b[0m\n");
	#endif
	
	loggerLog(LOG_ERROR, "Normal Mode execution terminated abnormally");
	return CMD_RUNTIME_ERROR;
}


static CommandStatus_t handleDebugCommand(void) {
	printf("Executing in Debug Mode...\n");
	loggerLog(LOG_INFO, "Starting execution in Debug Mode");

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: DEBUG received. Starting Debug Mode execution.\x1b[0m\n");
	#endif

	char debugBuf[10];

	printf("--- DEBUG MODE STARTED ---\n");
	printf("Press [ENTER] to step, 'q' to quit debug mode.\n");

	while(true) {
		printf("[DEBUG] PC:%03d | IR:%08d | AC:%08d\n", CPU.PSW.pc, CPU.IR, CPU.AC);
		
		bool active = cpuStep();
		
		if (!active) {
			printf("--- PROGRAM FINISHED (HALT) ---\n");
			loggerLog(LOG_INFO, "Debug Mode session finished (Program End)");
			return CMD_SUCCESS;
		}

		printf(">> ");
		if (fgets(debugBuf, sizeof(debugBuf), stdin) != NULL) {
			if (debugBuf[0] == 'q') {
				loggerLog(LOG_INFO, "Debug Mode session aborted by user");
				return CMD_SUCCESS;
			}
		}
	}
	
	return CMD_SUCCESS;
}


ConsoleStatus_t consoleStart(void) {
	char buffer[CONSOLE_BUFFER_SIZE];
	char command[CONSOLE_BUFFER_SIZE];
	char argument[CONSOLE_BUFFER_SIZE];
	
	printf("\033[2J\033[H");
	printf("=== LUCARIO REPL ===\n");
	
	while(true) {
		printf("LUCARIO > ");
		if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
		
		CommandStatus_t output = CMD_SUCCESS;
		CommandStatus_t parseStatus = parseInput(buffer, command, argument);

		if (parseStatus == CMD_EMPTY) continue;

		if (strcmp(command, "EXIT") == 0) {
			loggerLog(LOG_INFO, "System shutdown requested via CLI (EXIT command)");
			return CONSOLE_SUCCESS;
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: EXIT command received. Shutting down console.\x1b[0m\n");
			#endif
		} else if (strcmp(command, "LOAD") == 0) {
			output = handleLoadCommand(argument);
		} else if (strcmp(command, "RUN") == 0) {
			output = handleRunCommand();
		} else if (strcmp(command, "DEBUG") == 0) {
			output = handleDebugCommand();
		} else {
			printf("Unknown command: %s\n", command);
			snprintf(logBuffer, sizeof(logBuffer), "Unknown command received: %s", command);
			loggerLog(LOG_WARNING, logBuffer);
			output = CMD_UNKNOWN;
		}
		#ifdef DEBUG
		printf("\x1b[36m[DEBUG]: Command Output = [%d] \x1b[0m\n", output);
		#endif
	}

	return CONSOLE_RUNTIME_ERROR;
}
