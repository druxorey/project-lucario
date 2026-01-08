#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "../inc/loader.h"
#include "../inc/memory.h"
#include "../inc/logger.h"

word readProgramWord(FILE* filePtr) {
	char line[512];
	word w = 0;

	while (fgets(line, sizeof(line), filePtr) != NULL) {
		char* comment = strstr(line, "//");
		if (comment) {
			*comment = '\0';
		}

		if (sscanf(line, "%d", &w) == 1) {
			return w;
		}
	}

	return 0;
}


ProgramInfo_t loadProgram(char* filePath) {
	ProgramInfo_t programInfo = {0};
	char logBuffer[LOG_BUFFER_SIZE];

	snprintf(logBuffer, sizeof(logBuffer), "Loader: Attempting to load program from file '%s'.", filePath);
	loggerLog(LOG_INFO, logBuffer);

	FILE* programFile = fopen(filePath, "r");
	
	if (programFile) {
		loggerLog(LOG_INFO, "Loader: File opened successfully. Parsing metadata...");

		fscanf(programFile, "%*s %d", &programInfo._start);
		fscanf(programFile, "%*s %d", &programInfo.wordCount);
		fscanf(programFile, "%*s %s", programInfo.programName);

		snprintf(logBuffer, sizeof(logBuffer), "Loader: Metadata parsed - Name: %s, Words: %d, Start Line: %d", programInfo.programName, programInfo.wordCount, programInfo._start);
		loggerLog(LOG_INFO, logBuffer);

		CPU.PSW.mode = MODE_KERNEL;

		loggerLog(LOG_INFO, "Loader: Switched to KERNEL MODE for memory injection.");

		if (OS_RESERVED_SIZE + MIN_STACK_SIZE + programInfo.wordCount > RAM_SIZE) {
			snprintf(logBuffer, sizeof(logBuffer), "Loader Error: Program size exceeds available memory. Required: %d, Available: %d",
					OS_RESERVED_SIZE + MIN_STACK_SIZE + programInfo.wordCount, RAM_SIZE);
			loggerLog(LOG_ERROR, logBuffer);

			programInfo.status = LOAD_FILE_ERROR;
			return programInfo;
		}

		int stackMemory = RAM_SIZE - OS_RESERVED_SIZE - programInfo.wordCount;
		if (stackMemory > DEFAULT_STACK_SIZE) {
			stackMemory = DEFAULT_STACK_SIZE;
		}

		#ifdef DEBUG
		printf("\x1b[36m[DEBUG]: Stack memory available: %d words\x1b[0m\n", stackMemory);
		#endif

		for (int i = 0 ; i < programInfo.wordCount; i++) {
			word instruction = readProgramWord(programFile);

			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: Read instruction %d for address %d\x1b[0m\n", instruction, OS_RESERVED_SIZE + i);
			#endif

			MemoryStatus_t ret = writeMemory(OS_RESERVED_SIZE + i, instruction);
			
			if (ret != MEM_SUCCESS) {
				snprintf(logBuffer, sizeof(logBuffer), "Loader Error: Memory write failed at physical address %d (Error Code: %d).", OS_RESERVED_SIZE + i, ret);
				loggerLog(LOG_ERROR, logBuffer);

				fclose(programFile);

				programInfo.status = LOAD_MEMORY_ERROR;
				return programInfo;
			}
		}
		
		loggerLog(LOG_INFO, "Loader: All instructions written to RAM successfully.");

		CPU.RB = OS_RESERVED_SIZE;
		CPU.RL = OS_RESERVED_SIZE + stackMemory + programInfo.wordCount;
		snprintf(logBuffer, sizeof(logBuffer), "Loader: Context Set - RB: %d | RL: %d | PC: %d", CPU.RB, CPU.RL, CPU.PSW.pc);
		loggerLog(LOG_INFO, logBuffer);

		CPU.RX = programInfo.wordCount;
		CPU.SP = programInfo.wordCount + stackMemory;
		snprintf(logBuffer, sizeof(logBuffer), "Loader: Stack Set - SP: %d | RX (Stack Base): %d", CPU.SP, CPU.RX);
		loggerLog(LOG_INFO, logBuffer);

		CPU.PSW.pc = programInfo._start - 1;
		snprintf(logBuffer, sizeof(logBuffer), "Loader: PC set to start of program at address %d.", CPU.PSW.pc);
		loggerLog(LOG_INFO, logBuffer);

		CPU.timerLimit = 16;      
		CPU.cyclesCounter = 0;    

		CPU.PSW.mode = MODE_USER;
		CPU.PSW.interruptEnable = ITR_ENABLED;
		programInfo.status = LOAD_SUCCESS;

		fclose(programFile);
		loggerLog(LOG_INFO, "Loader: File closed. Program ready for execution.");

	} else {
		snprintf(logBuffer, sizeof(logBuffer), "Loader Error: Could not open file '%s'. Check path or permissions.", filePath);
		loggerLog(LOG_ERROR, logBuffer);

		programInfo.status = LOAD_FILE_ERROR;
		return programInfo;
	}
	return programInfo;
}
