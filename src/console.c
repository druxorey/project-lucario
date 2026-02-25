#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "../inc/definitions.h"
#include "../inc/console.h"
#include "../inc/logger.h"
#include "../inc/hardware/cpu.h"
#include "../inc/kernel/loader.h"

static char logBuffer[LOG_BUFFER_SIZE];

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


static void printFullRegisters(void) {
	printf("\n\x1b[1;33m================= CPU STATE =================\x1b[0m\n");
	printf(" PC:     %05d | AC:  %08d (%d)\n", CPU.PSW.pc, CPU.AC, wordToInt(CPU.AC));
	printf(" IR:  %08d | OP:  %02d | MD: %1d | VAL: %05d\n",
	       CPU.IR,
	       GET_INSTRUCTION_OPCODE(CPU.IR),
	       GET_INSTRUCTION_MODE(CPU.IR),
	       GET_INSTRUCTION_VALUE(CPU.IR));
	printf(" MDR: %08d | MAR: %05d\n", CPU.MDR, CPU.MAR);
	printf("---------------------------------------------\n");
	printf(" RB:  %08d | RL:  %08d\n", CPU.RB, CPU.RL);
	printf(" SP:  %08d | RX:  %08d\n", CPU.SP, CPU.RX);
	printf(" Int: %s      | CC:  %d | PSW Mode: %s\n",
	       (CPU.PSW.interruptEnable) ? "ON " : "OFF",
	       CPU.PSW.conditionCode,
	       (CPU.PSW.mode == MODE_KERNEL) ? "KERNEL" : "USER");
	printf("\x1b[1;33m=============================================\x1b[0m\n\n");
}


static void printCommandList(void) {
	printf("\n\x1b[35mAVAILABLE COMMANDS:\x1b[0m\n");
	printf("  \x1b[1mRUN <file>\x1b[0m     : Execute program in Normal Mode\n");
	printf("  \x1b[1mDEBUG <file>\x1b[0m   : Execute program in Debug Mode\n");
	printf("  \x1b[1mLIST\x1b[0m           : List the files in the current directory\n");
	printf("  \x1b[1mEXIT\x1b[0m           : Shutdown the system\n");
	printf("  \x1b[1mCOMANDS\x1b[0m        : Show this list\n\n");
}


static void printFilesList(void) {
	DIR *actualDirectory;
	struct dirent *dir;
	char **files = NULL;
	int count = 0;

	actualDirectory = opendir(".");

	if (actualDirectory) {
		while ((dir = readdir(actualDirectory)) != NULL) {
			if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
			if (dir->d_type == DT_DIR) continue;

			char **temp = realloc(files, (count + 1) * sizeof(char *));
			if (temp == NULL) {
				perror("Error: Could not allocate memory for file list");
				closedir(actualDirectory);
				return;
			}
			files = temp;
			files[count] = malloc(strlen(dir->d_name) + 1);
			strcpy(files[count], dir->d_name);
			count++;
		}
		for (int i = 0; i < count; i++) {
        	printf("  %s\n", files[i]);
			free(files[i]);
    	}
		free(files);
		closedir(actualDirectory);
	} else {
		printf("Error: Could not open directory\n");
		return;
	}
}


CommandStatus_t parseInput(char* input, char* command, char* argument) {
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
	// If input is "EXIT", matches will be 1 (argument remains empty)
	// If input is "RUN file" or "DEBUG file", matches will be 2
	
	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Parsed -> Command: [%s]; Argument: [%s] \x1b[0m\n", command, argument);
	#endif

	return CMD_SUCCESS;
}


CommandStatus_t handleLoadCommand(char* argument) {
	if (strlen(argument) == 0) {
		printf("Error: Missing filename\n");
		loggerLog(LOG_WARNING, "User attempted to load program without filename argument");
		return CMD_MISSING_ARGS;
	}

	ProgramInfo_t info = loadProgram(argument);

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Program load processed. Status = [%d]\x1b[0m\n", info.status);
	#endif

	if (info.status == LOAD_SUCCESS) {
		printf("File '%s' loaded successfully\n", argument);
		snprintf(logBuffer, sizeof(logBuffer), "Program loaded: %s (Words: %d, Start: %d)", argument, info.wordCount, info._start);
		loggerLog(LOG_INFO, logBuffer);
		return CMD_SUCCESS;
	}

	printf("Error loading file\n");
	snprintf(logBuffer, sizeof(logBuffer), "Failed to load program file: %s", argument);
	loggerLog(LOG_ERROR, logBuffer);
	return CMD_LOAD_ERROR;
}


CommandStatus_t handleRunCommand(char* argument) {
	cpuReset();

	CommandStatus_t loadStatus = handleLoadCommand(argument);
	if (loadStatus != CMD_SUCCESS) return loadStatus;
	
	printf("Executing in Normal Mode...\n");
	loggerLog(LOG_INFO, "Starting execution in Normal Mode");

	if (cpuRun() == 0) {
		printf("Execution finished successfully\n");
		loggerLog(LOG_INFO, "Normal Mode execution finished successfully");
		return CMD_SUCCESS;
	}

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: RUN execution terminated abnormally.\x1b[0m\n");
	#endif
	
	loggerLog(LOG_ERROR, "Normal Mode execution terminated abnormally");
	return CMD_RUNTIME_ERROR;
}


CommandStatus_t handleDebugCommand(char* argument) {
	cpuReset();

	CommandStatus_t loadStatus = handleLoadCommand(argument);
	if (loadStatus != CMD_SUCCESS) return loadStatus;

	printf("Executing in Debug Mode...\n");
	loggerLog(LOG_INFO, "Starting execution in Debug Mode.");

	char debugBuf[CONSOLE_BUFFER_SIZE];
	bool isRunning = true;
	
	printf("\n\x1b[32mDEBUGGER COMMANDS:\x1b[0m\n");
	printf("  \x1b[1mSTEP\x1b[0m (or ENTER) : Execute next instruction.\n");
	printf("  \x1b[1mREGS\x1b[0m            : View all registers detailed.\n");
	printf("  \x1b[1mQUIT\x1b[0m            : Exit debugger (stops execution).\n");

	printFullRegisters();

	while (isRunning) {
		printf("\x1b[32mDEBUG > \x1b[0m");
		
		if (fgets(debugBuf, sizeof(debugBuf), stdin) == NULL) break;
		
		debugBuf[strcspn(debugBuf, "\n")] = 0;
		char* cmd = trimWhitespace(debugBuf);

		if (strlen(cmd) == 0 || strcmp(cmd, "STEP") == 0) {
			int currentPC = CPU.PSW.pc;
			bool active = cpuStep();

			printf(" -> Executed Addr: \x1b[33m%03d\x1b[0m | Instr: \x1b[33m%08d\x1b[0m | Result AC: \x1b[33m%08d\x1b[0m\n", currentPC, CPU.IR, CPU.AC);

			if (!active) {
				printf("\x1b[31mPROGRAM HALTED\x1b[0m\n");
				loggerLog(LOG_INFO, "Debug Mode session finished (CPU Halted).");
				return CMD_SUCCESS;
			}
		} else if (strcmp(cmd, "REGS") == 0) {
			printFullRegisters();
		} else if (strcmp(cmd, "QUIT") == 0) {
			printf("Exiting debugger...\n");
			loggerLog(LOG_INFO, "Debug Mode session aborted by user.");
			isRunning = false;
		} else {
			printf("Unknown debug command. Use: STEP, REGS, QUIT.\n");
		}
	}
	
	return CMD_SUCCESS;
}


ConsoleStatus_t consoleStart(void) {
	char buffer[CONSOLE_BUFFER_SIZE];
	char command[CONSOLE_BUFFER_SIZE];
	char argument[CONSOLE_BUFFER_SIZE];
	
	printf("\x1b[2J\x1b[H\n");

	printf("  █     █  █  █▀▀▀  █▀▀█  █▀▀█  ▀█▀  █▀▀█     █▀▀█ █▀▀▀  █▀▀█   █   \n");
	printf("  █     █  █  █     █▀▀█  █▀▀▄   █   █  █     █▀▀▄ █▀▀▀  █▀▀▀   █   \n");
	printf("  █     █  █  █     █  █  █  █   █   █  █     █  █ █     █      █   \n");
	printf("  ▀▀▀▀  ▀▀▀▀  ▀▀▀▀  ▀  ▀  ▀  ▀  ▀▀▀  ▀▀▀▀     ▀  ▀ ▀▀▀▀  ▀      ▀▀▀▀\n");

	printCommandList();
	
	while(true) {
		printf("\x1b[35mLUCARIO\x1b[0m > ");
		if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
		
		CommandStatus_t output = CMD_SUCCESS;
		CommandStatus_t parseStatus = parseInput(buffer, command, argument);

		if (parseStatus == CMD_EMPTY) continue;

		if (strcmp(command, "EXIT") == 0) {
			loggerLog(LOG_INFO, "System shutdown requested via CLI (EXIT command)");
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: EXIT command received. Shutting down console.\x1b[0m\n");
			#endif
			return CONSOLE_SUCCESS;
		} else if (strcmp(command, "RUN") == 0) {
			output = handleRunCommand(argument);
		} else if (strcmp(command, "DEBUG") == 0) {
			output = handleDebugCommand(argument);
		} else if (strcmp(command, "LIST") == 0) {
			printFilesList();
		} else if (strcmp(command, "COMANDS") == 0) {
			printCommandList();
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
