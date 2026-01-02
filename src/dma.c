#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#include "../inc/dma.h"
#include "../inc/logger.h"
#include "../inc/memory.h"

pthread_cond_t DMA_COND;

void *dmaInit(void*) {
    pthread_cond_init(&DMA_COND, NULL);
    dmaReset();

    while (true) {
        pthread_mutex_lock(&BUS_LOCK);
        while (!DMA.pending) {
            pthread_cond_wait(&DMA_COND, &BUS_LOCK);
        }

        DMA.status = 0;
        DMA.active = true;
        pthread_mutex_unlock(&BUS_LOCK);

        word data;
        MemoryStatus_t status;
        if (DMA.ioDirection == 1) {
            status = readMemory(DMA.memAddr, &data);
            if (status == MEM_SUCCESS) {
                DISK[DMA.track][DMA.cylinder][DMA.sector].data = data;
            }
        } else {
            data = DISK[DMA.track][DMA.cylinder][DMA.sector].data;
            status = writeMemory(DMA.memAddr, data);
        }

        pthread_mutex_lock(&BUS_LOCK);
        if (status != MEM_SUCCESS) {
            DMA.status = 1;
            loggerLogInterrupt(IC_INVALID_ADDR);
        } else {
            loggerLogInterrupt(IC_IO_DONE);
        }
        DMA.active = false;
        DMA.pending = false;
        pthread_mutex_unlock(&BUS_LOCK);
    }
}

void dmaReset(void) {
    DMA = (DMA_t){0};
}