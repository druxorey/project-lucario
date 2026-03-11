#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "../../inc/kernel/syscalls.h"
#include "../../inc/kernel/core.h"
#include "../../inc/hardware/cpu.h"
#include "../../inc/hardware/memory.h"
#include "../../inc/logger.h"
#include "../../inc/console.h"

SyscallStatus_t handleSyscall(void) {
	int syscallCode = wordToInt(CPU.AC);
	word param;
	char logBuffer[LOG_BUFFER_SIZE];
	MemoryStatus_t status;

	address userSP = CPU.SP + 7; // This is because saveContext pushes 7 words onto the stack, so the original user SP is 7 words above the current SP.

	switch(syscallCode) {
		case 1:
			status = readMemory(userSP, &param);
			if (status == MEM_SUCCESS) {
				int exitCode = wordToInt(param);
				snprintf(logBuffer, LOG_BUFFER_SIZE, "SYSCALL [1]: Process PID [%d] requested EXIT with code %d", PROCESS_TABLE[currentActiveProcess].pid, exitCode);
				loggerLogKernel(LOG_INFO, logBuffer);
			} else {
				loggerLogKernel(LOG_WARNING, "SYSCALL [1]: EXIT requested, but failed to read exit code from stack.");
			}
			
			return SYSCALL_HALT;

		case 2:
			status = readMemory(userSP, &param);
			if (status == MEM_SUCCESS) {
				int valueToPrint = wordToInt(param);
				
				if (OS_MONITOR_ACTIVE) {
					char msg[256];
					snprintf(msg, sizeof(msg), "[PID %02d - %s] Output: %d", PROCESS_TABLE[currentActiveProcess].pid, PROCESS_TABLE[currentActiveProcess].programName, valueToPrint);
					monitorPrint(msg);
					
					snprintf(logBuffer, LOG_BUFFER_SIZE, "SYSCALL [2]: Process PID [%d] printed value %d", PROCESS_TABLE[currentActiveProcess].pid, valueToPrint);
					loggerLogKernel(LOG_INFO, logBuffer);
					
					return SYSCALL_SUCCESS;
				} else {
					PROCESS_TABLE[currentActiveProcess].state = BLOCKED_IO;
					
					snprintf(logBuffer, LOG_BUFFER_SIZE, "SYSCALL [2]: Process PID [%d] BLOCKED_IO waiting for monitor", PROCESS_TABLE[currentActiveProcess].pid);
					loggerLogKernel(LOG_INFO, logBuffer);
					
					word savedPcWord;
					readMemory(CPU.SP + 1, &savedPcWord);
					int savedPc = wordToInt(savedPcWord);
					writeMemory(CPU.SP + 1, intToWord(savedPc - 1, &CPU.PSW));
					
					return SYSCALL_BLOCK;
				}
				
			} else {
				loggerLogKernel(LOG_ERROR, "SYSCALL [2]: Print requested, but failed to read value from stack.");
				return SYSCALL_HALT;
			}

		case 3:
			if (OS_MONITOR_ACTIVE) {
				isSyscallReading = true;
				usleep(150000);

				disableRawMode();
				clearerr(stdin);
				
				int64_t userInput = 0;
				bool validInput = false;
				char msg[256];

				while (!validInput) {
					printf("\r\x1b[2K\x1b[33m[PID %02d - %s] request input:\x1b[0m ", PROCESS_TABLE[currentActiveProcess].pid, PROCESS_TABLE[currentActiveProcess].programName);
					fflush(stdout);
					
					if (scanf("%ld", &userInput) == 1) {
						validInput = true;
					} else {
						clearerr(stdin);
						while (getchar() != '\n' && !feof(stdin));
						printf("\r\x1b[2K\x1b[31m[ERROR] Invalid input. Please enter a numeric value\x1b[0m\n");
					}
				}

				if (userInput > MAX_MAGNITUDE || userInput < -MAX_MAGNITUDE) {
					char msg[256];
					snprintf(msg, sizeof(msg), "\x1b[31m[WARN] Input exceeds architecture limits (7 digits). Truncating...\x1b[0m");
					printf("%s\n", msg);
					monitorSaveHistory(msg);
					
					snprintf(logBuffer, LOG_BUFFER_SIZE, "SYSCALL [3]: User input '%ld' caused hardware overflow. Truncating.", userInput);
					loggerLogKernel(LOG_WARNING, logBuffer);
				}
				
				snprintf(msg, sizeof(msg), "\x1b[33m[PID %02d - %s] request input:\x1b[0m %d", PROCESS_TABLE[currentActiveProcess].pid, PROCESS_TABLE[currentActiveProcess].programName, (int32_t)userInput);
				monitorSaveHistory(msg);
				writeMemory(CPU.SP, intToWord((int32_t)userInput, &CPU.PSW));
				
				snprintf(logBuffer, LOG_BUFFER_SIZE, "SYSCALL [3]: Process PID [%d] read value %d", PROCESS_TABLE[currentActiveProcess].pid, (int32_t)userInput);
				loggerLogKernel(LOG_INFO, logBuffer);
				
				enableRawMode();
				isSyscallReading = false;
				
				return SYSCALL_SUCCESS;
			} else {
				PROCESS_TABLE[currentActiveProcess].state = BLOCKED_IO;
				
				snprintf(logBuffer, LOG_BUFFER_SIZE, "SYSCALL [3]: Process PID [%d] BLOCKED_IO waiting for monitor", PROCESS_TABLE[currentActiveProcess].pid);
				loggerLogKernel(LOG_INFO, logBuffer);
				
				word savedPcWord;
				readMemory(CPU.SP + 1, &savedPcWord);
				int savedPc = wordToInt(savedPcWord);
				writeMemory(CPU.SP + 1, intToWord(savedPc - 1, &CPU.PSW));
				
				return SYSCALL_BLOCK;
			}

		case 4:
			status = readMemory(userSP, &param);
			if (status == MEM_SUCCESS) {
				int sleepTics = wordToInt(param);
				
				PROCESS_TABLE[currentActiveProcess].sleepTics = sleepTics;
				PROCESS_TABLE[currentActiveProcess].state = BLOCKED;
				
				snprintf(logBuffer, LOG_BUFFER_SIZE, "SYSCALL [4]: Process PID [%d] sleeping for %d tics", PROCESS_TABLE[currentActiveProcess].pid, sleepTics);
				loggerLogKernel(LOG_INFO, logBuffer);
				
				return SYSCALL_BLOCK;
			} else {
				loggerLogKernel(LOG_ERROR, "SYSCALL [4]: Sleep requested, but failed to read tics from stack.");
				return SYSCALL_HALT;
			}

		default:
			snprintf(logBuffer, LOG_BUFFER_SIZE, "SYSCALL [%d]: Unknown service requested", syscallCode);
			loggerLogKernel(LOG_WARNING, logBuffer);
			return SYSCALL_UNKNOWN;
	}
}
