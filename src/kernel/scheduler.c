#include <stdbool.h>
#include <stdio.h>

#include "../../inc/logger.h"
#include "../../inc/console.h"
#include "../../inc/kernel/scheduler.h"
#include "../../inc/kernel/core.h"

void schedulerTick(void) {
	char logBuffer[LOG_BUFFER_SIZE];

	for (int i = 0; i < MAX_PROCESSES; i++) {
		if (PROCESS_TABLE[i].state == BLOCKED && PROCESS_TABLE[i].sleepTics > 0) {
			PROCESS_TABLE[i].sleepTics--;
			if (PROCESS_TABLE[i].sleepTics == 0) {
				PROCESS_TABLE[i].state = READY;
				snprintf(logBuffer, LOG_BUFFER_SIZE, "[SCHEDULER] Process PID [%d] woke up and is now READY", PROCESS_TABLE[i].pid);
				loggerLogKernel(LOG_INFO, logBuffer);
			}
		}
	}

	if (currentActiveProcess != -1) {
		PROCESS_TABLE[currentActiveProcess].context = CPU;
		if (PROCESS_TABLE[currentActiveProcess].state == EXECUTING) {
			PROCESS_TABLE[currentActiveProcess].state = READY;
		}
	}

	int nextProcess = -1;
	
	int startIndex = (currentActiveProcess == -1) ? 0 : (currentActiveProcess + 1) % MAX_PROCESSES;
	
	for (int i = 0; i < MAX_PROCESSES; i++) {
		int checkIndex = (startIndex + i) % MAX_PROCESSES;
		
		if (PROCESS_TABLE[checkIndex].state == READY) {
			nextProcess = checkIndex;
			break;
		}
	}

	if (nextProcess != -1) {
		if (currentActiveProcess != nextProcess) {
			int oldPid = (currentActiveProcess != -1) ? PROCESS_TABLE[currentActiveProcess].pid : 0;
			snprintf(logBuffer, LOG_BUFFER_SIZE, "[SCHEDULER] Context Switch: Out PID [%d], In PID [%d]", oldPid, PROCESS_TABLE[nextProcess].pid);
			loggerLogKernel(LOG_INFO, logBuffer);
		}

		currentActiveProcess = nextProcess;
		PROCESS_TABLE[currentActiveProcess].state = EXECUTING;
		
		CPU = PROCESS_TABLE[currentActiveProcess].context;

	} else {
		if (currentActiveProcess != -1) {
			loggerLogKernel(LOG_INFO, "[SCHEDULER] No READY processes. System is now IDLE.");
		}
		currentActiveProcess = -1;
	}
}
