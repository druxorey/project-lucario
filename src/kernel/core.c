#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#include "../../inc/logger.h"
#include "../../inc/hardware/cpu.h"
#include "../../inc/kernel/core.h"

bool osRunning = false;
pthread_t cpuThread;
PCB_t ProcessTable[MAX_PROCESSES];
int currentActiveProcess = -1;

void* cpuThreadWorker(void* arg) {
	(void)arg;
	loggerLogKernel(LOG_INFO, "CPU Background Thread started");

	while (osRunning) {
		// Here we will check if there are READY processes before calling cpuStep()
		
		// Simulation: Check for active processes (this is a placeholder, actual implementation would check the process scheduler)
		// if (procesosActivos > 0) {
		//     cpuStep();
		// } else {
		//     usleep(10000); // 10ms de descanso si no hay procesos
		// }
		
		usleep(100000); // Temporal to keep the thread alive and sleeping to avoid burning the real CPU.
	}

	loggerLogKernel(LOG_INFO, "CPU Background Thread stopped");
	return NULL;
}


OSStatus_t osStart(void) {
	osRunning = true;
	if (pthread_create(&cpuThread, NULL, cpuThreadWorker, NULL) != 0) {
		loggerLogKernel(LOG_ERROR, "Failed to create CPU thread");
		return OS_ERR_THREAD;
	}
	return OS_SUCCESS;
}


OSStatus_t osStop(void) {
	osRunning = false;
	if (pthread_join(cpuThread, NULL) != 0) {
		loggerLogKernel(LOG_ERROR, "Failed to join CPU thread");
		return OS_ERR_THREAD;
	}
	return OS_SUCCESS;
}


OSStatus_t initOS(void) {
	for (int i = 0; i < MAX_PROCESSES; i++) {
		ProcessTable[i].state = FINISHED;
		ProcessTable[i].pid = -1;
	}
	
	currentActiveProcess = -1;
	
	loggerLogKernel(LOG_INFO, "OS initialized: Process Table completely flushed and ready");
	return OS_SUCCESS;
}


int getFreePCBIndex(void) {
	for (int i = 0; i < MAX_PROCESSES; i++) {
		if (ProcessTable[i].state == FINISHED) return i;
	}
	
	loggerLogKernel(LOG_WARNING, "Process limit reached. No free PCB available");
	return -1;
}
