#include <stdbool.h>
#include <stdio.h>

#include "../inc/definitions.h"
#include "../inc/logger.h"
#include "../inc/console.h"
#include "../inc/hardware/dma.h"
#include "../inc/hardware/memory.h"
#include "../inc/kernel/core.h"

CPU_t CPU;

int main() {
	loggerInit();
	memoryInit();

	pthread_t dmaThread;
	pthread_create(&dmaThread, NULL, &dmaInit, NULL);
	pthread_detach(dmaThread);

	loggerLogHardware(LOG_INFO, "System Boot sequence initiated");

	if (initOS() != OS_SUCCESS) {
		printf("\x1b[1;31mCRITICAL ERROR: Could not initialize OS structures.\x1b[0m\n");
		return 1;
	}

	if (osStart() != OS_SUCCESS) {
		printf("\x1b[1;31mCRITICAL ERROR: Could not start OS Kernel.\x1b[0m\n");
		return 1;
	}

	ConsoleStatus_t consoleStatus = consoleStart();

	if (consoleStatus == CONSOLE_SUCCESS) {
		loggerLogHardware(LOG_INFO, "System Shutdown completed successfully");
	} else if (consoleStatus == CONSOLE_RUNTIME_ERROR) {
		loggerLogHardware(LOG_ERROR, "System Shutdown incorrectly");
	}

	osStop();

	loggerClose();
	pthread_mutex_destroy(&BUS_LOCK);

	return 0;
}
