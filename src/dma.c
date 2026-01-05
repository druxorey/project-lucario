#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#include "../inc/dma.h"
#include "../inc/cpu.h"
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
        usleep(10000); // Simulate search time
        
        word data;
        MemoryStatus_t status;
        if (DMA.ioDirection == 1) {
            pthread_mutex_unlock(&BUS_LOCK);
            status = readMemory(DMA.memAddr, &data);
            pthread_mutex_lock(&BUS_LOCK);
            if (status == MEM_SUCCESS) {
                DISK[DMA.track][DMA.cylinder][DMA.sector].data = data;
            }
        } else {
            data = DISK[DMA.track][DMA.cylinder][DMA.sector].data;
            pthread_mutex_unlock(&BUS_LOCK);
            status = writeMemory(DMA.memAddr, data);
            pthread_mutex_lock(&BUS_LOCK);
        }


        if (status != MEM_SUCCESS) {
            DMA.status = 1;
			raiseInterrupt(IC_INVALID_ADDR);
        } else {
			raiseInterrupt(IC_IO_DONE);
        }
        DMA.active = false;
        DMA.pending = false;
        pthread_mutex_unlock(&BUS_LOCK);
    }
}

void dmaReset(void) {
    DMA = (DMA_t){0};
}
