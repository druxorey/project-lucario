#include <stdio.h>
#include <string.h>

#include "../inc/loader.h"
#include "../inc/memory.h"
#include "../inc/logger.h"

static pthread_mutex_t program_mutex = PTHREAD_MUTEX_INITIALIZER;

ProgramInfo_t loadProgram(char* filePath) {
	ProgramInfo_t programInfo = {0};
	char logBuffer[256];

	snprintf(logBuffer, sizeof(logBuffer), "Loader: Attempting to load program from file '%s'.", filePath);
	loggerLog(LOG_INFO, logBuffer);

	FILE* programFile = fopen(filePath, "r");
	
	if (programFile) {
		loggerLog(LOG_INFO, "Loader: File opened successfully. Parsing metadata...");

		fscanf(programFile, "%*s %d", &programInfo._start);
		fscanf(programFile, "%*s %d", &programInfo.wordCount);
		fscanf(programFile, "%*s %s", programInfo.programName);

		snprintf(logBuffer, sizeof(logBuffer), "Loader: Metadata parsed - Name: %s, Words: %d, Start Line: %d",
				 programInfo.programName, programInfo.wordCount, programInfo._start);
		loggerLog(LOG_INFO, logBuffer);

		CPU.PSW.mode = MODE_KERNEL;
		loggerLog(LOG_INFO, "Loader: Switched to KERNEL MODE for memory injection.");
		
		for (int i = 0 ; i < programInfo.wordCount; i++) {
			word instruction = readProgramWord(programFile);
			
			if (writeMemory(OS_RESERVED_SIZE + i, instruction) != 0) {
				snprintf(logBuffer, sizeof(logBuffer), "Loader Error: Memory write failed at physical address %d. Possible overflow or violation.", OS_RESERVED_SIZE + i);
				loggerLog(LOG_ERROR, logBuffer);

				fclose(programFile);

				ProgramInfo_t programInfoError = {0};
				programInfoError.status = LOAD_MEMORY_ERROR;
				return programInfoError;
			}
		}
		
		loggerLog(LOG_INFO, "Loader: All instructions written to RAM successfully.");

		CPU.RB = OS_RESERVED_SIZE;
		CPU.RL = OS_RESERVED_SIZE + programInfo.wordCount;
		CPU.PSW.pc = OS_RESERVED_SIZE + programInfo._start - 1;

		snprintf(logBuffer, sizeof(logBuffer), "Loader: Context Set - RB: %d | RL: %d | PC: %d", CPU.RB, CPU.RL, CPU.PSW.pc);
		loggerLog(LOG_INFO, logBuffer);

		CPU.PSW.mode = MODE_USER;
		programInfo.status = LOAD_SUCCESS;

		fclose(programFile);
		loggerLog(LOG_INFO, "Loader: File closed. Program ready for execution.");

	} else {
		snprintf(logBuffer, sizeof(logBuffer), "Loader Error: Could not open file '%s'. Check path or permissions.", filePath);
		loggerLog(LOG_ERROR, logBuffer);

		ProgramInfo_t programInfoError = {0};
		programInfoError.status = LOAD_FILE_ERROR;
		return programInfoError;
	}
	return programInfo;
}
