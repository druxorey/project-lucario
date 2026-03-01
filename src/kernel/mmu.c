#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

#include "../../inc/kernel/mmu.h"

bool freePartitions[PARTITIONS]; // false = occupied, true = free

void mmuInit(void) {
    memset(freePartitions, true, sizeof(freePartitions));
}

int calculateRequiredBlocks(int wordCount) {
    int total = wordCount + MIN_STACK_SIZE;
    if (total <= MIN_STACK_SIZE || total > (PARTITION_SIZE * PARTITIONS)) return 0;
    int blocks = ceil(total / (double)PARTITION_SIZE);
    return blocks;
}

int allocateMemory(int requiredBlocks) {
    if (requiredBlocks <= 0 || requiredBlocks > PARTITIONS) return -1;

    int contiguousCount = 0;
    for (int i = 0; i < PARTITIONS; i++) {
        if (freePartitions[i]) {
            contiguousCount++;
            if (contiguousCount == requiredBlocks) {
                int startIndex = i - requiredBlocks + 1;
                for (int j = startIndex; j <= i; j++) {
                    freePartitions[j] = false;
                }
                return startIndex;
            }
        } else {
            contiguousCount = 0;
        }
    }
    return -1;
}

int freeMemory(int startBlock, int blockCount) {
    if (startBlock < 0 || startBlock >= PARTITIONS || blockCount <= 0 || (startBlock + blockCount) > PARTITIONS) {
        return -1;
    }
    for (int i = startBlock; i < startBlock + blockCount; i++) {
        freePartitions[i] = true;
    }
    return 0;
}