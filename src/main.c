#include <stdio.h>
#include <stdlib.h>

#include "../inc/definitions.h"
#include "../inc/logger.h"
#include "../inc/memory.h"
#include "../inc/console.h"

// Global debug flag definition 
bool isDebugMode = false; 

int main(int argc, char* argv[]) {
	#ifdef DEBUG
	printf("Debug Mode\n");
	#endif

	// Boot Sequence
	loggerInit();
	memoryInit();
	loggerLog(LOG_INFO, "System Boot sequence initiated,");

	// Shutdown Sequence
	if(consoleStart() == 0) {
		loggerLog(LOG_INFO, "System Shutdown completed successfully");
		loggerClose();
		pthread_mutex_destroy(&BUS_LOCK);
	}

	return 0;
}
