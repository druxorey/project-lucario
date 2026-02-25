#include <stdbool.h>

#include "../inc/definitions.h"
#include "../inc/logger.h"
#include "../inc/console.h"
#include "../inc/hardware/dma.h"
#include "../inc/hardware/memory.h"

CPU_t CPU;

int main() {
	loggerInit();
	memoryInit();

	pthread_t dmaThread;
    pthread_create(&dmaThread, NULL, &dmaInit, NULL);
	pthread_detach(dmaThread);
	
	loggerLogHardware(LOG_INFO, "System Boot sequence initiated");

	ConsoleStatus_t consoleStatus = consoleStart();

	if (consoleStatus == CONSOLE_SUCCESS) {
		loggerLogHardware(LOG_INFO, "System Shutdown completed successfully");
	} else if (consoleStatus == CONSOLE_RUNTIME_ERROR) {
		loggerLogHardware(LOG_ERROR, "System Shutdown incorrectly");
	}

	loggerClose();
	pthread_mutex_destroy(&BUS_LOCK);

	return 0;
}
