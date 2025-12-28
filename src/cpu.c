#include <string.h>
#include "../inc/cpu.h"
#include "../inc/logger.h"

// Global CPU instance
CPU_t CPU = {0};  

// Mock implementations of CPU functions
int cpuRun(void) {
    loggerLog(LOG_INFO, "cpuRun mock");
    return 1; 
}

bool cpuStep(void) {
    loggerLog(LOG_INFO, "cpuStep mock");
    return false; 
}

void cpuReset(void) {
    memset(&CPU, 0, sizeof(CPU)); // Reset all CPU registers to zero
    loggerLog(LOG_INFO, "cpuReset mock");
}