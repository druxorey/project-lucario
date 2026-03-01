/**
 * @file: mmu.h
 * @brief: Memory Management Unit (MMU) definitions and function prototypes.
 * 
 * This header defines the interface for the MMU subsystem, including functions 
 * for initializing the MMU, calculating required memory blocks, allocating and 
 * freeing memory, and macros for calculating base and limit register values.
 * 
 * @version 1.0
 */

#ifndef MMU_H
#define MMU_H

#include "../../inc/definitions.h"

#define PARTITIONS 20  /**< Maximun number of user processes that can be loaded in main memory. */
#define PARTITION_SIZE ((RAM_SIZE - OS_RESERVED_SIZE) / PARTITIONS) /**< Size of each partition in words. */

/** 
 * @brief Initializes the MMU subsystem.
 * 
 * Initializes all the partition management bitmap entries to indicate they're all free. 
 */
void mmuInit(void);

/** 
 * @brief Calculates the number of memory blocks required for a given program's word count.
 * 
 * Calculates the total memory needed by adding the program's word count to the 
 * minimum stack size, then determines how many partitions (blocks) are required 
 * to accommodate that total. If the total exceeds the maximum memory available, 
 * it returns 0.
 * 
 * @param wordCount The program's number of words.
 * @return The number of memory blocks (partitions) required to accommodate the request.
 */
int calculateRequiredBlocks(int wordCount);

/** 
 * @brief Allocates memory blocks for a program.
 * 
 * Searches for the first range of required number of contiguous free blocks in 
 * the partition bitmap and marks them as occupied. 
 * 
 * @param requiredBlocks The number of contiguous blocks needed for the program.
 * @return The index of the first allocated block, or -1 if allocation fails.
 */
int allocateMemory(int requiredBlocks);

/** 
 * @brief Frees previously allocated memory blocks.
 * 
 * Marks the specified blocks as free in the partition bitmap.
 * 
 * @param startBlock The index of the first block to free.
 * @param blockCount The number of contiguous blocks to free.
 * @return OSStatus_t indicating success or failure of the operation.
 *         **TEMPORARILY** returning 0 on success, -1 on failure, should be updated to OSStatus_t in future refactor.
 */
/*TEMPORARILY int*/ int freeMemory(int startBlock, int blockCount);

#define GET_BASE_REGISTER(startBlock) (OS_RESERVED_SIZE + (startBlock * PARTITION_SIZE))      /**< @brief Calculates the base register value for a given block index. */
#define GET_LIMIT_REGISTER(rb, blockCount) (rb + (blockCount * PARTITION_SIZE) - 1)               /**< @brief Returns the limit register value for a given block index. */

#endif // MMU_H