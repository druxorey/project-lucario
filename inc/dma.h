/**
 * @file dma.h
 * @brief Direct Memory Access (DMA) controller simulation.
 *
 * Handles high-speed data transfers between memory and I/O devices
 *
 * @version 1.0
 */
#ifndef DMA_H
#define DMA_H

#include "../inc/definitions.h"

/**
 * @brief Initializes the DMA controller.
 * 
 * Once initialized, DMA sleeps at cond var pthread_cond_wait until a transfer is requested.
 */
void dmaInit(void);

/**
 * @brief Resets the DMA controller to its initial state.
 */
void dmaReset(void);

#endif // DMA_H