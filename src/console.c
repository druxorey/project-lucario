#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <termios.h>
#include <unistd.h>

#include "../inc/definitions.h"
#include "../inc/console.h"
#include "../inc/logger.h"
#include "../inc/hardware/cpu.h"
#include "../inc/kernel/loader.h"

static char logBuffer[LOG_BUFFER_SIZE];
static char monitorHistory[MAX_HISTORY_LINES][MAX_LINE_LENGTH];
static int historyCount = 0;
static struct termios origTermios;
bool OS_MONITOR_ACTIVE;

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


static CommandStatus_t printCommandList(void) {
	printf("\n\x1b[35mAVAILABLE COMMANDS:\x1b[0m");
	printf(" run, debug, list, ps, memstat, monitor, restart,\n  shutdown, commands, help\n");
	printf("\n  Use \x1b[1mhelp <command>\x1b[0m for detailed information.\n\n");
	return CMD_SUCCESS;
}


static CommandStatus_t handleHelpCommand(char* argument) {
	if (argument == NULL || strlen(argument) == 0) {
		printf("Usage: help <command>\nAvailable commands: run, debug, list, ps, memstat, monitor, restart, shutdown, commands, help\n");
		return CMD_SUCCESS;
	}

	printf("\n\x1b[1;34mMANUAL: %s\x1b[0m\n", argument);
	if (strcmp(argument, "run") == 0) {
		printf("Description: Executes programs in Normal Mode.\n");
		printf("Usage:       run <file1> [file2] ... [file20]\n");
		printf("Arguments:   Accepts 1 to 20 program files.\n");
	} else if (strcmp(argument, "debug") == 0) {
		printf("Description: Executes a single program in Debug Mode with step-by-step control.\n");
		printf("Usage:       debug <file>\n");
		printf("Arguments:   Exactly 1 program file.\n");
	} else if (strcmp(argument, "ps") == 0) {
		printf("Description: Displays all active processes in the system, showing PID, state, memory percentage, and program name.\n");
		printf("Usage:       ps\n");
		printf("Arguments:   None.\n");
	} else if (strcmp(argument, "memstat") == 0) {
		printf("Description: Shows the physical memory content and the current usage percentage.\n");
		printf("Usage:       memstat\n");
		printf("Arguments:   None.\n");
	} else if (strcmp(argument, "monitor") == 0) {
		printf("Description: Opens a secondary terminal to display the Input/Output of running programs.\n");
		printf("Usage:       monitor\n");
		printf("Arguments:   None.\n");
	} else if (strcmp(argument, "list") == 0) {
		printf("Description: Lists all files available in the current directory.\n");
		printf("Usage:       list\n");
		printf("Arguments:   None.\n");
	} else if (strcmp(argument, "restart") == 0) {
		printf("Description: Reboots the Lucario System.\n");
		printf("Usage:       restart\n");
		printf("Arguments:   None.\n");
	} else if (strcmp(argument, "shutdown") == 0) {
		printf("Description: Safely shuts down the system and exits the simulator.\n");
		printf("Usage:       shutdown\n");
		printf("Arguments:   None.\n");
	} else if (strcmp(argument, "commands") == 0) {
		printf("Description: Lists all available commands names.\n");
		printf("Usage:       commands\n");
		printf("Arguments:   None.\n");
	} else if (strcmp(argument, "help") == 0) {
		printf("Description: Provides detailed information about a specific command.\n");
		printf("Usage:       help <command>\n");
		printf("Arguments:   1 command name.\n");
	} else {
		printf("\x1b[1;31mNo manual entry for:\x1b[0m %s\n", argument);
	}
	printf("\n");
	return CMD_SUCCESS;
}


static CommandStatus_t printFilesList(void) {
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
				perror("\x1b[1;31mError: Could not allocate memory for file list\x1b[0m");
				closedir(actualDirectory);
				return CMD_RUNTIME_ERROR;
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
		loggerLogKernel(LOG_ERROR, "Failed to open current directory for LIST command");
		printf("\x1b[1;31mError: Could not open directory\x1b[0m");
		return CMD_RUNTIME_ERROR;
	}
	loggerLogKernel(LOG_INFO, "User requested directory listing (LIST)");
	return CMD_SUCCESS;
}


static CommandStatus_t enableRawMode(void) {
	if (tcgetattr(STDIN_FILENO, &origTermios) == -1) return CMD_RUNTIME_ERROR;
	
	struct termios raw = origTermios;
	// We shutdown ECHO (don't print letters alone) and ICANON (read letter by letter without Enter)
	raw.c_lflag &= ~(ECHO | ICANON);
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return CMD_RUNTIME_ERROR;
	return CMD_SUCCESS;
}

static CommandStatus_t disableRawMode(void) {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios) == -1) return CMD_RUNTIME_ERROR;
	return CMD_SUCCESS;
}


CommandStatus_t parseInput(char* input, char* command, char** args, int* argCount) {
	command[0] = '\0';
	*argCount = 0;

	input[strcspn(input, "\n")] = '\0';
	input = trimWhitespace(input);

	if (strlen(input) == 0) return CMD_EMPTY;

	char* token = strtok(input, " \t");
	if (token == NULL) return CMD_EMPTY;

	strncpy(command, token, 19);
	command[19] = '\0';

	token = strtok(NULL, " \t");
	while (token != NULL && *argCount < MAX_PROCESSES) {
		args[*argCount] = token;
		(*argCount)++;
		token = strtok(NULL, " \t");
	}

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Parsed -> Command: [%s]; Args Count: [%d]\x1b[0m\n", command, *argCount);
	for (int i = 0; i < *argCount; i++) {
		printf("\x1b[36m[DEBUG]: Arg[%d]: [%s]\x1b[0m\n", i, args[i]);
	}
	#endif

	return CMD_SUCCESS;
}


CommandStatus_t monitorPrint(const char* message) {
    if (historyCount < MAX_HISTORY_LINES) {
        strncpy(monitorHistory[historyCount], message, MAX_LINE_LENGTH - 1);
        monitorHistory[historyCount][MAX_LINE_LENGTH - 1] = '\0';
        historyCount++;
    } else {
        for (int i = 1; i < MAX_HISTORY_LINES; i++) strcpy(monitorHistory[i - 1], monitorHistory[i]);
        strncpy(monitorHistory[MAX_HISTORY_LINES - 1], message, MAX_LINE_LENGTH - 1);
        monitorHistory[MAX_HISTORY_LINES - 1][MAX_LINE_LENGTH - 1] = '\0';
    }

    snprintf(logBuffer, LOG_BUFFER_SIZE, "Message sent to monitor: %s", message);
    loggerLogKernel(LOG_INFO, logBuffer);

    if (OS_MONITOR_ACTIVE) {
        printf("\r\x1b[2K%s\r\n", message);
        fflush(stdout);
    }

    return CMD_SUCCESS;
}


CommandStatus_t startMonitorSession(void) {
	OS_MONITOR_ACTIVE = true;

	printf("\x1b[s\x1b[?1049h"); // ANSI Sequence to switch to alternate screen and save cursor position
	printf("\x1b[2J\x1b[H"); // Clear screen and move cursor to top-left
	
	printf("\x1b[36m--- MONITOR MODE (Press ESC to return to console) ---\x1b[0m\n\n");

	for (int i = 0; i < historyCount; i++) printf("%s\n", monitorHistory[i]);

	if (enableRawMode() != CMD_SUCCESS) {
		OS_MONITOR_ACTIVE = false;
		printf("\x1b[?1049l\x1b[u");
		return CMD_RUNTIME_ERROR;
	}

	char inputBuffer[MAX_LINE_LENGTH];
	int inputPos = 0;

	while (true) {
		char c = getchar();
		if (c == 27) break; // ESC key to exit monitor
		else if (c == '\n' || c == '\r') {
			inputBuffer[inputPos] = '\0';
			
			char formattedMsg[MAX_LINE_LENGTH];
			snprintf(formattedMsg, sizeof(formattedMsg), "> [User]: %s", inputBuffer);
			
			printf("\r\n");
			monitorPrint(formattedMsg);
			
			inputPos = 0;
		}
		else if (c == 127 || c == 8) {
			if (inputPos > 0) {
				inputPos--;
				printf("\b \b");
				fflush(stdout);
			}
		}
		else if (isprint(c) && inputPos < (MAX_LINE_LENGTH - 1)) {
			inputBuffer[inputPos++] = c;
			putchar(c);
			fflush(stdout);
		}
	}

	disableRawMode();
	OS_MONITOR_ACTIVE = false;

	printf("\x1b[?1049l\x1b[u"); // ANSI Sequence to switch back to main screen and restore cursor position
	fflush(stdout);

	return CMD_SUCCESS;
}


CommandStatus_t handleLoadCommand(char* argument) {
	if (strlen(argument) == 0) {
		loggerLogKernel(LOG_WARNING, "User attempted to load program without filename argument");
		printf("\x1b[1;31mError: Missing filename\x1b[0m\n");
		return CMD_MISSING_ARGS;
	}

	ProgramInfo_t info = loadProgram(argument);

	if (info.status == LOAD_SUCCESS) {
		snprintf(logBuffer, LOG_BUFFER_SIZE, "Program loaded: %s (Words: %d, Start: %d)", argument, info.wordCount, info._start);
		loggerLogKernel(LOG_INFO, logBuffer);
		printf("File '%s' loaded successfully\n", argument);
		return CMD_SUCCESS;
	}

	snprintf(logBuffer, LOG_BUFFER_SIZE, "Failed to load program file: %s", argument);
	loggerLogKernel(LOG_ERROR, logBuffer);
	printf("\x1b[1;31mError: Loading file failed\x1b[0m\n");
	return CMD_LOAD_ERROR;
}


CommandStatus_t handleRunCommand(char** args, int argCount) {
	if (argCount == 0) {
		printf("\x1b[1;31mError: Missing program files to execute\x1b[0m\n");
		return CMD_MISSING_ARGS;
	}

	printf("Loading processes into OS...\n");

	for (int i = 0; i < argCount; i++) {
		// Here eventually we will call createProcess(args[i]);
		printf(" -> [QUEUED] Process %s created successfully.\n", args[i]);
		
		snprintf(logBuffer, LOG_BUFFER_SIZE, "Process requested via CLI: %s", args[i]);
		loggerLogKernel(LOG_INFO, logBuffer);
	}

	printf("\x1b[32mAll processes loaded and executing in background.\x1b[0m\n");
	
	return CMD_SUCCESS;
}


CommandStatus_t handleDebugCommand(char* argument) {
	cpuReset();

	CommandStatus_t loadStatus = handleLoadCommand(argument);
	if (loadStatus != CMD_SUCCESS) return loadStatus;

	printf("Executing in Debug Mode...\n");
	loggerLogKernel(LOG_INFO, "Starting execution in Debug Mode.");

	char debugBuf[CONSOLE_BUFFER_SIZE];
	bool isRunning = true;
	
	printf("\n\x1b[32mDEBUGGER COMMANDS:\x1b[0m\n");
	printf("  \x1b[1mstep\x1b[0m (or ENTER) : Execute next instruction.\n");
	printf("  \x1b[1mregs\x1b[0m            : View all registers detailed.\n");
	printf("  \x1b[1mquit\x1b[0m            : Exit debugger (stops execution).\n");

	printFullRegisters();

	while (isRunning) {
		printf("\x1b[32mDEBUG > \x1b[0m");
		
		if (fgets(debugBuf, sizeof(debugBuf), stdin) == NULL) break;
		
		debugBuf[strcspn(debugBuf, "\n")] = 0;
		char* cmd = trimWhitespace(debugBuf);

		if (strlen(cmd) == 0 || strcmp(cmd, "step") == 0) {
			int currentPC = CPU.PSW.pc;
			bool active = cpuStep();

			printf(" -> Executed Addr: \x1b[33m%03d\x1b[0m | Instr: \x1b[33m%08d\x1b[0m | Result AC: \x1b[33m%08d\x1b[0m\n", currentPC, CPU.IR, CPU.AC);

			if (!active) {
				printf("\x1b[31mPROGRAM HALTED\x1b[0m\n");
				loggerLogKernel(LOG_INFO, "Debug Mode session finished (CPU Halted).");
				return CMD_SUCCESS;
			}
		} else if (strcmp(cmd, "regs") == 0) {
			printFullRegisters();
		} else if (strcmp(cmd, "quit") == 0) {
			printf("Exiting debugger...\n");
			loggerLogKernel(LOG_INFO, "Debug Mode session aborted by user.");
			isRunning = false;
		} else {
			printf("Unknown debug command. Use: step, regs, quit.\n");
		}
	}
	
	return CMD_SUCCESS;
}


ConsoleStatus_t consoleStart(void) {
	char buffer[CONSOLE_BUFFER_SIZE];
	char command[CONSOLE_BUFFER_SIZE];
	char* argument[MAX_PROCESSES];
	
	printf("\x1b[2J\x1b[H\n");

	printf("  █     █  █  █▀▀▀  █▀▀█  █▀▀█  ▀█▀  █▀▀█     █▀▀█ █▀▀▀  █▀▀█   █   \n");
	printf("  █     █  █  █     █▀▀█  █▀▀▄   █   █  █     █▀▀▄ █▀▀▀  █▀▀▀   █   \n");
	printf("  █     █  █  █     █  █  █  █   █   █  █     █  █ █     █      █   \n");
	printf("  ▀▀▀▀  ▀▀▀▀  ▀▀▀▀  ▀  ▀  ▀  ▀  ▀▀▀  ▀▀▀▀     ▀  ▀ ▀▀▀▀  ▀      ▀▀▀▀\n");

	loggerLogKernel(LOG_INFO, "Console interface initialized and ready");
	printCommandList();
	
	while(true) {
		printf("\x1b[35mLUCARIO\x1b[0m > ");
		if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
			loggerLogKernel(LOG_ERROR, "Input stream closed or failed (stdin)");
			break;
		}
		
		int argCount = 0;
		CommandStatus_t output = CMD_UNKNOWN;
		CommandStatus_t parseStatus = parseInput(buffer, command, argument, &argCount);

		if (parseStatus == CMD_EMPTY) continue;

		if (strcmp(command, "shutdown") == 0) {
			if (argCount > 0) {
				loggerLogKernel(LOG_WARNING, "Too many arguments for 'shutdown' command");
				printf("\x1b[1;31mError: The 'shutdown' command does not accept arguments\x1b[0m\n");
			} else {
				loggerLogKernel(LOG_INFO, "System shutdown requested via CLI (EXIT command)");
				return CONSOLE_SUCCESS;
			}
		} else if (strcmp(command, "run") == 0) {
			if (argCount == 0) {
				loggerLogKernel(LOG_WARNING, "Missing arguments for 'run' command");
				printf("\x1b[1;31mError: Missing program file(s) to execute\x1b[0m\n");
			} else {
				output = handleRunCommand(argument, argCount);
			}
		} else if (strcmp(command, "debug") == 0) {
			if (argCount == 1) {
				output = handleDebugCommand(argument[0]);
			} else if (argCount > 1) {
				printf("\x1b[1;31mError: Too many arguments for 'debug' command\x1b[0m\n");
				loggerLogKernel(LOG_WARNING, "Too many arguments for 'debug' command");
			} else {
				printf("\x1b[1;31mError: Missing program file(s) to execute\x1b[0m\n");
				loggerLogKernel(LOG_WARNING, "Missing arguments for 'debug' command");
			}
		} else if (strcmp(command, "list") == 0) {
			if (argCount > 0) {
				printf("\x1b[1;31mError: The 'list' command does not accept arguments\x1b[0m\n");
				loggerLogKernel(LOG_WARNING, "Too many arguments for 'list' command");
			} else {
				output = printFilesList();
			}
		} else if (strcmp(command, "commands") == 0) {
			if (argCount > 0) {
				printf("\x1b[1;31mError: The 'commands' command does not accept arguments\x1b[0m\n");
				loggerLogKernel(LOG_WARNING, "Too many arguments for 'commands' command");
			} else {
				output = printCommandList();
			}
		} else if (strcmp(command, "help") == 0) {
			if (argCount > 1) {
				printf("\x1b[1;31mError: Too many arguments for 'help' command\x1b[0m\n");
			} else {
				output = handleHelpCommand(argCount == 1 ? argument[0] : NULL);
			}
		} else if (strcmp(command, "monitor") == 0) {
			output = startMonitorSession();
		} else if (strcmp(command, "testprint") == 0) { // Debug command to test monitor output without running a program
			char msg[256];
			snprintf(msg, sizeof(msg), "[PID 99] Prueba de salida asincrona numero %d", rand() % 100 + 1);
			output = monitorPrint(msg);
			printf("\x1b[32mMensaje enviado al monitor en segundo plano.\x1b[0m\n");
		} else {
			printf("\x1b[1;31mUnknown command:\x1b[0m %s\n", command);
			snprintf(logBuffer, LOG_BUFFER_SIZE, "Unknown command received: %s", command);
			loggerLogKernel(LOG_WARNING, logBuffer);
		}
		#ifdef DEBUG
		printf("\x1b[36m[DEBUG]: Command Output = [%d] \x1b[0m\n", output);
		#endif
	}

	return CONSOLE_RUNTIME_ERROR;
}
