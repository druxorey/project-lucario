#include <stdbool.h>

#include "../lib/utest.h"
#include "../inc/kernel/mmu.h"

// Global mock RAM and partition tracking for testing
word mockRAM[RAM_SIZE];

// Auxiliary function to peek into the freePartitions array for testing
bool isPartitionFree(int index) {
    extern bool freePartitions[PARTITIONS];
    return freePartitions[index];
}

UTEST_MAIN();

// Verify that the MMU initializes all partitions as free
UTEST(mmu, initialization) {
    mmuInit();
    for (int i = 0; i < PARTITIONS; i++) {
        ASSERT_TRUE(isPartitionFree(i));
    }
}

// Threshold tests
UTEST(mmu, calculateRequiredBlocks) {
    // Test with 0 words (should not be allocated any blocks)
    ASSERT_EQ(calculateRequiredBlocks(0), 0);
    
    // Test with exactly one partition's worth of words (minus stack)
    int wordsForOneBlock = PARTITION_SIZE - MIN_STACK_SIZE;
    ASSERT_EQ(calculateRequiredBlocks(wordsForOneBlock), 1);
    
    // Test with just over one block's worth of words
    ASSERT_EQ(calculateRequiredBlocks(wordsForOneBlock + 1), 2);
    
    // Test with multiple blocks
    int wordsForTwoBlocks = (PARTITION_SIZE * 2) - MIN_STACK_SIZE;
    ASSERT_EQ(calculateRequiredBlocks(wordsForTwoBlocks), 2);
    
    // Test with just over two blocks
    ASSERT_EQ(calculateRequiredBlocks(wordsForTwoBlocks + 1), 3);

    // Test with a large number of words
    int wordsForTenBlocks = (PARTITION_SIZE * (PARTITIONS/2)) - MIN_STACK_SIZE;
    ASSERT_EQ(calculateRequiredBlocks(wordsForTenBlocks), PARTITIONS/2);

    //Test with just under maximum blocks (should return maximum number of blocks)
    int wordsForMaxBlocks = (PARTITION_SIZE * PARTITIONS) - MIN_STACK_SIZE;
    ASSERT_EQ(calculateRequiredBlocks(wordsForMaxBlocks), PARTITIONS);

    // Test with just over maximum blocks (should return maximum number of blocks)
    int wordsForTooManyBlocks = (PARTITION_SIZE * (PARTITIONS + 1)) - MIN_STACK_SIZE;
    ASSERT_EQ(calculateRequiredBlocks(wordsForTooManyBlocks), 0); // Assuming we dont allocate for requests that exceed total partitions
}

// Contiguous block allocation tests
UTEST(mmu, allocateMemory) {
    mmuInit(); // Reset state before test

    // Allocate 1 block
    int blockIndex = allocateMemory(1);
    ASSERT_EQ(blockIndex, 0);
    ASSERT_FALSE(isPartitionFree(0));

    // Allocate another block
    blockIndex = allocateMemory(1);
    ASSERT_EQ(blockIndex, 1);
    ASSERT_FALSE(isPartitionFree(1));

    // Allocate 2 blocks
    blockIndex = allocateMemory(2);
    ASSERT_EQ(blockIndex, 2);
    ASSERT_FALSE(isPartitionFree(2));
    ASSERT_FALSE(isPartitionFree(3));

    // Attempt to allocate more blocks than available
    blockIndex = allocateMemory(PARTITIONS); // Request more blocks than total
    ASSERT_EQ(blockIndex, -1); // Should fail

    // Attempt to allocate remaining blocks
    blockIndex = allocateMemory(PARTITIONS - 4); // Request remaining blocks
    ASSERT_EQ(blockIndex, 4); // Should get the next free block
    for (int i = 4; i < PARTITIONS; i++) {
        ASSERT_FALSE(isPartitionFree(i));
    }

    // Free the first two blocks
    freeMemory(0, 3);
    ASSERT_TRUE(isPartitionFree(0));
    ASSERT_TRUE(isPartitionFree(1));
    ASSERT_TRUE(isPartitionFree(2));

    // Allocate 3 blocks (should reuse freed blocks)
    blockIndex = allocateMemory(3);
    ASSERT_EQ(blockIndex, 0); // Should get the first free block
    for (int i = 0; i < 3; i++) {
        ASSERT_FALSE(isPartitionFree(i));
    }

    // Free two separate ranges of blocks
    freeMemory(0, 2); // Free first blocks 0 and 1
    freeMemory(4, PARTITIONS - 4);
    for (int i = 0; i < 2; i++) {
        ASSERT_TRUE(isPartitionFree(i));
    }
    for (int i = 4; i < PARTITIONS; i++) {
        ASSERT_TRUE(isPartitionFree(i));
    }

    // Attempt to allocate more blocks than available in first contiguous range
    blockIndex = allocateMemory(3); // Request 3 blocks but only 2 are free at the start
    ASSERT_EQ(blockIndex, 4); // Should allocate from the next available range of free blocks
    for (int i = 4; i < 7; i++) {
        ASSERT_FALSE(isPartitionFree(i));
    }
}

// Base and limit register calculation tests
UTEST(mmu, baseAndLimitRegisters) {
    // Test base register calculation
    int startBlockIndex = 3;
    address expectedBase = OS_RESERVED_SIZE + (startBlockIndex * PARTITION_SIZE);
    ASSERT_EQ(GET_BASE_REGISTER(startBlockIndex), expectedBase);

    // Test limit register calculation
    int endBlockIndex = 5;
    address expectedLimit = expectedBase + (endBlockIndex * PARTITION_SIZE) - 1;
    ASSERT_EQ(GET_LIMIT_REGISTER(expectedBase, endBlockIndex), expectedLimit);
}