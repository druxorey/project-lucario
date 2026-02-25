#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

#include "../../inc/logger.h"
#include "../../inc/hardware/dma.h"
#include "../../inc/hardware/cpu.h"
#include "../../inc/hardware/memory.h"

DMA_t DMA;
pthread_cond_t DMA_COND;
static char logBuffer[LOG_BUFFER_SIZE];

void *dmaInit(void* tmp) {
	srand(time(NULL));
	pthread_cond_init(&DMA_COND, NULL);
	loggerLogHardware(LOG_INFO, "DMA Controller initialized and worker thread started");
	dmaReset();

	while (true) {
		pthread_mutex_lock(&BUS_LOCK);

		while (!DMA.pending) pthread_cond_wait(&DMA_COND, &BUS_LOCK);

		DMA.status = 0;
		DMA.active = true;

		snprintf(logBuffer, LOG_BUFFER_SIZE, "DMA Transfer started: %s | MemAddr: 0x%04X | Disk: [T:%d, C:%d, S:%d]",
			(DMA.ioDirection == 1 ? "MEM_TO_DISK" : "DISK_TO_MEM"), DMA.memAddr, DMA.track, DMA.cylinder, DMA.sector);
		loggerLogHardware(LOG_INFO, logBuffer);

		usleep(50000 + (rand() % 100000)); // Simulate search time
		
		word data;
		MemoryStatus_t status;

		if (DMA.ioDirection == 1) {
			pthread_mutex_unlock(&BUS_LOCK);
			status = dmaReadMemory(DMA.memAddr, &data);
			pthread_mutex_lock(&BUS_LOCK);
			if (status == MEM_SUCCESS) DISK[DMA.track][DMA.cylinder][DMA.sector].data = data;
		} else {
			data = DISK[DMA.track][DMA.cylinder][DMA.sector].data;
			pthread_mutex_unlock(&BUS_LOCK);
			status = dmaWriteMemory(DMA.memAddr, data);
			pthread_mutex_lock(&BUS_LOCK);
		}

		if (status != MEM_SUCCESS) {
			DMA.status = 1;
			snprintf(logBuffer, LOG_BUFFER_SIZE, "DMA Transfer failed: Invalid memory address 0x%04X", DMA.memAddr);
			loggerLogHardware(LOG_ERROR, logBuffer);
			raiseInterrupt(IC_INVALID_ADDR);
		} else {
			loggerLogHardware(LOG_INFO, "DMA Transfer completed successfully");
			raiseInterrupt(IC_IO_DONE);
		}

		DMA.active = false;
		DMA.pending = false;
		pthread_mutex_unlock(&BUS_LOCK);
	}
}

void dmaReset(void) {
	DMA = (DMA_t){0};
	loggerLogHardware(LOG_INFO, "DMA registers have been reset to default values");
}
