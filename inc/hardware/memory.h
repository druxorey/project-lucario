/**
 * @file memory.h
 * @brief Memory Management Unit (MMU) and Physical RAM simulation.
 *
 * Handles secure access to the shared memory array, including address translation
 * (Logical -> Physical), protection (Base/Limit registers), and thread safety.
 *
 * @version 2.0
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "../../inc/definitions.h"

/**
 * @brief Status codes for memory operations.
 * Replaces generic integers for better type safety and readability.
 */
typedef enum {
    MEM_SUCCESS           = 0, /**< Operation completed successfully. */
    MEM_ERR_OUT_OF_BOUNDS = 1, /**< Bus Error: Physical address > RAM_SIZE. */
    MEM_ERR_PROTECTION    = 2, /**< SegFault: User tried to access outside RB/RL. */
    MEM_ERR_INVALID_DATA  = 3  /**< Data corruption: Value exceeds 8-digit limit. */
} MemoryStatus_t;

/**
 * @brief Initializes the memory subsystem.
 * Creates the mutex for bus arbitration.
 */
void memoryInit(void);

/**
 * @brief Thread-safe memory read with MMU translation.
 *
 * @param logicalAddr Address requested by the CPU (Relative to process).
 * @param outData Pointer where the read value will be stored.
 * @return MemoryStatus_t result code.
 */
MemoryStatus_t readMemory(address logicalAddr, word* outData);

/**
 * @brief Thread-safe memory write with MMU translation.
 *
 * @param logicalAddr Address requested by the CPU.
 * @param data The 8-digit word to write.
 * @return MemoryStatus_t result code.
 */
MemoryStatus_t writeMemory(address logicalAddr, word data);

/**
 * @brief Direct Physical Memory Read (Bypasses MMU protection).
 * Used exclusively by DMA to access pre-validated physical addresses.
 */
MemoryStatus_t dmaReadMemory(address physAddr, word* outData);

/**
 * @brief Direct Physical Memory Write (Bypasses MMU protection).
 * Used exclusively by DMA to write to pre-validated physical addresses.
 */
MemoryStatus_t dmaWriteMemory(address physAddr, word data);

/**
 * @brief Resets the entire memory to its initial state (all zeros).
 * Used during system restart to ensure a clean slate.
 */
void memoryReset(void);

#endif // MEMORY_H
