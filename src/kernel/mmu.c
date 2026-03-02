#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "../../inc/kernel/mmu.h"

bool FREE_PARTITIONS[MAX_PROCESSES]; // false = occupied, true = free


void mmuInit(void) {
	memset(FREE_PARTITIONS, true, sizeof(FREE_PARTITIONS));
}


int calculateRequiredBlocks(int wordCount) {
	int total = wordCount + MIN_STACK_SIZE;
	if (total <= MIN_STACK_SIZE || total > (PARTITION_SIZE * MAX_PROCESSES)) return 0;
	int blocks = (total + PARTITION_SIZE - 1) / PARTITION_SIZE;
	return blocks;
}


int allocateMemory(int requiredBlocks) {
	if (requiredBlocks <= 0 || requiredBlocks > MAX_PROCESSES) return -1;

	int contiguousCount = 0;
	for (int i = 0; i < MAX_PROCESSES; i++) {
		if (FREE_PARTITIONS[i]) {
			contiguousCount++;
			if (contiguousCount == requiredBlocks) {
				int startIndex = i - requiredBlocks + 1;
				for (int j = startIndex; j <= i; j++) {
					FREE_PARTITIONS[j] = false;
				}
				return startIndex;
			}
		} else {
			contiguousCount = 0;
		}
	}
	return -1;
}


OSStatus_t freeMemory(int startBlock, int blockCount) {
	if (startBlock < 0 || startBlock >= MAX_PROCESSES || blockCount <= 0 || (startBlock + blockCount) > MAX_PROCESSES) {
		return OS_ERR_MEMORY;
	}
	for (int i = startBlock; i < startBlock + blockCount; i++) {
		FREE_PARTITIONS[i] = true;
	}
	return OS_SUCCESS;
}
