#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "../../inc/logger.h"
#include "../../inc/hardware/memory.h"
#include "../../inc/kernel/loader.h"

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

	snprintf(logBuffer, LOG_BUFFER_SIZE, "Loader: Attempting to load program from file '%s'.", filePath);
	loggerLogHardware(LOG_INFO, logBuffer);

	FILE* programFile = fopen(filePath, "r");
	
	if (programFile) {
		loggerLogHardware(LOG_INFO, "Loader: File opened successfully. Parsing metadata...");

		fscanf(programFile, "%*s %d", &programInfo._start);
		fscanf(programFile, "%*s %d", &programInfo.wordCount);
		fscanf(programFile, "%*s %s", programInfo.programName);

		snprintf(logBuffer, LOG_BUFFER_SIZE, "Loader: Metadata parsed - Name: %s, Words: %d, Start Line: %d", programInfo.programName, programInfo.wordCount, programInfo._start);
		loggerLogHardware(LOG_INFO, logBuffer);

		CPU.PSW.mode = MODE_KERNEL;

		loggerLogHardware(LOG_INFO, "Loader: Switched to KERNEL MODE for memory injection.");

		if (OS_RESERVED_SIZE + MIN_STACK_SIZE + programInfo.wordCount > RAM_SIZE) {
			snprintf(logBuffer, LOG_BUFFER_SIZE, "Loader Error: Program size exceeds available memory. Required: %d, Available: %d",
					OS_RESERVED_SIZE + MIN_STACK_SIZE + programInfo.wordCount, RAM_SIZE);
			loggerLogHardware(LOG_ERROR, logBuffer);

			programInfo.status = LOAD_FILE_ERROR;
			return programInfo;
		}

		int stackMemory = RAM_SIZE - OS_RESERVED_SIZE - programInfo.wordCount;
		if (stackMemory > DEFAULT_STACK_SIZE) {
			stackMemory = DEFAULT_STACK_SIZE;
		}

		snprintf(logBuffer, LOG_BUFFER_SIZE, "Stack memory available: %d words", stackMemory);
		loggerLogHardware(LOG_INFO, logBuffer);

		for (int i = 0 ; i < programInfo.wordCount; i++) {
			word instruction = readProgramWord(programFile);

			snprintf(logBuffer, LOG_BUFFER_SIZE, "Read instruction %08d for address %d", instruction, OS_RESERVED_SIZE + i);
			loggerLogHardware(LOG_INFO, logBuffer);

			MemoryStatus_t ret = writeMemory(OS_RESERVED_SIZE + i, instruction);
			
			if (ret != MEM_SUCCESS) {
				snprintf(logBuffer, LOG_BUFFER_SIZE, "Loader Error: Memory write failed at physical address %d (Error Code: %d).", OS_RESERVED_SIZE + i, ret);
				loggerLogHardware(LOG_ERROR, logBuffer);

				fclose(programFile);

				programInfo.status = LOAD_MEMORY_ERROR;
				return programInfo;
			}
		}
		
		loggerLogHardware(LOG_INFO, "Loader: All instructions written to RAM successfully.");

		CPU.RB = OS_RESERVED_SIZE;
		CPU.RL = OS_RESERVED_SIZE + stackMemory + programInfo.wordCount;
		snprintf(logBuffer, LOG_BUFFER_SIZE, "Loader: Context Set - RB: %d | RL: %d | PC: %d", CPU.RB, CPU.RL, CPU.PSW.pc);
		loggerLogHardware(LOG_INFO, logBuffer);

		CPU.RX = programInfo.wordCount;
		CPU.SP = programInfo.wordCount + stackMemory;
		snprintf(logBuffer, LOG_BUFFER_SIZE, "Loader: Stack Set - SP: %d | RX (Stack Base): %d", CPU.SP, CPU.RX);
		loggerLogHardware(LOG_INFO, logBuffer);

		CPU.PSW.pc = programInfo._start - 1;
		snprintf(logBuffer, LOG_BUFFER_SIZE, "Loader: PC set to start of program at address %d.", CPU.PSW.pc);
		loggerLogHardware(LOG_INFO, logBuffer);

		CPU.timerLimit = 16;
		CPU.cyclesCounter = 0;

		CPU.PSW.mode = MODE_USER;
		CPU.PSW.interruptEnable = ITR_ENABLED;
		programInfo.status = LOAD_SUCCESS;

		fclose(programFile);
		loggerLogHardware(LOG_INFO, "Loader: File closed. Program ready for execution.");

	} else {
		snprintf(logBuffer, LOG_BUFFER_SIZE, "Loader Error: Could not open file '%s'. Check path or permissions.", filePath);
		loggerLogHardware(LOG_ERROR, logBuffer);

		programInfo.status = LOAD_FILE_ERROR;
		return programInfo;
	}
	return programInfo;
}
