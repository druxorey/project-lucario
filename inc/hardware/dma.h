/**
 * @file dma.h
 * @brief Direct Memory Access (DMA) controller simulation.
 *
 * Handles high-speed data transfers between memory and I/O devices
 *
 * @version 1.2
 */
#ifndef DMA_H
#define DMA_H

#include "../../inc/definitions.h"

/** @brief Status codes for DMA operations
 * Indicates success or cause of failure of DMA operations.
 */
typedef enum {
	DMA_SUCCESS = 0,           /**< DMA operation completed successfully. */
	DMA_ERR_INVALID_GEOM = 1,  /**< Invalid memory disk geometry specified. */
} DMAStatus_t;

/**
 * @brief Initializes the DMA controller.
 * Once initialized, DMA sleeps at cond var pthread_cond_wait until a transfer is requested.
 */
void *dmaInit(void*);

/**
 * @brief Resets the DMA controller to its initial state.
 */
void dmaReset(void);

#endif // DMA_H
