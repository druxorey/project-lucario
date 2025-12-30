#include "../inc/definitions.h"
#include "../inc/logger.h"
#include "../inc/memory.h"
#include "../inc/console.h"

CPU_t CPU;

int main() {
	loggerInit();
	memoryInit();
	loggerLog(LOG_INFO, "System Boot sequence initiated");

	ConsoleStatus_t consoleStatus = consoleStart();

	if (consoleStatus == CONSOLE_SUCCESS) {
		loggerLog(LOG_INFO, "System Shutdown completed successfully");
	} else if (consoleStatus == CONSOLE_RUNTIME_ERROR) {
		loggerLog(LOG_ERROR, "System Shutdown incorrectly");
	}

	loggerClose();
	pthread_mutex_destroy(&BUS_LOCK);

	return 0;
}
