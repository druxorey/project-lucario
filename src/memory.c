#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "../inc/memory.h"
#include "../inc/logger.h"

// Physical memory storage
word RAM[RAM_SIZE];

// Mutex for Bus arbitration
pthread_mutex_t BUS_LOCK;

void memoryInit(void) {
	pthread_mutex_init(&BUS_LOCK, NULL);
	loggerLog(LOG_INFO, "Memory Subsystem Initialized.");
}

static bool isPhysicalAddressValid(int physAddr) {
	// Prevents buffer overflow on the actual C array
	return (physAddr >= 0 && physAddr < RAM_SIZE);
}

static bool isProtectionViolation(int physAddr) {
	// Kernel Mode bypasses memory protection (God Mode)
	if (CPU.PSW.mode == MODE_KERNEL) {
		return false;
	}

	// User Mode must stay within its assigned partition [RB, RL]
	if (physAddr < CPU.RB || physAddr > CPU.RL) {
		return true;
	}

	return false;
}

static int getPhysicalAddress(address logicalAddr, MemoryStatus_t* status) {
	int physAddr;

	// Translate: Absolute addressing for Kernel, Relative for User
	if (CPU.PSW.mode == MODE_KERNEL) {
		physAddr = logicalAddr;
	} else {
		physAddr = logicalAddr + CPU.RB;
	}

	if (isProtectionViolation(physAddr)) {
		*status = MEM_ERR_PROTECTION;
		return -1;
	}

	if (!isPhysicalAddressValid(physAddr)) {
		*status = MEM_ERR_OUT_OF_BOUNDS;
		return -1;
	}

	*status = MEM_SUCCESS;
	return physAddr;
}


MemoryStatus_t readMemory(address logicalAddr, word* outData) {
	char logBuffer[LOG_BUFFER_SIZE];
	
	pthread_mutex_lock(&BUS_LOCK);

	MemoryStatus_t status;
	int physAddr = getPhysicalAddress(logicalAddr, &status);

	if (status != MEM_SUCCESS) {
		pthread_mutex_unlock(&BUS_LOCK);
		
		if (status == MEM_ERR_PROTECTION) {
			loggerLog(LOG_ERROR, "Segmentation Fault: Read Access Violation.");
		} else {
			loggerLog(LOG_ERROR, "Bus Error: Physical Address Out of Bounds.");
		}
		return status;
	}

	*outData = RAM[physAddr];

	snprintf(logBuffer, sizeof(logBuffer), "Mem READ: Logic[%d] -> Phys[%d] = Val[%d]", logicalAddr, physAddr, *outData);
	loggerLog(LOG_DEBUG, logBuffer);

	pthread_mutex_unlock(&BUS_LOCK);
	return MEM_SUCCESS;
}


MemoryStatus_t writeMemory(address logicalAddr, word data) {
	char logBuffer[LOG_BUFFER_SIZE];
	
	pthread_mutex_lock(&BUS_LOCK);

	// Validate data structure (Sign bit + 7 magnitude digits)
	if (!IS_VALID_WORD(data)) {
		pthread_mutex_unlock(&BUS_LOCK);
		loggerLog(LOG_ERROR, "Memory Error: Invalid word format (sign or magnitude).");
		return MEM_ERR_INVALID_DATA;
	}

	MemoryStatus_t status;
	int physAddr = getPhysicalAddress(logicalAddr, &status);

	if (status != MEM_SUCCESS) {
		pthread_mutex_unlock(&BUS_LOCK);
		
		if (status == MEM_ERR_PROTECTION) {
			loggerLog(LOG_ERROR, "Segmentation Fault: Write Access Violation.");
		} else {
			loggerLog(LOG_ERROR, "Bus Error: Physical Address Out of Bounds.");
		}
		return status;
	}

	RAM[physAddr] = data;

	snprintf(logBuffer, sizeof(logBuffer), "Mem WRITE: Logic[%d] -> Phys[%d] = Val[%d]", logicalAddr, physAddr, data);
	loggerLog(LOG_DEBUG, logBuffer);

	pthread_mutex_unlock(&BUS_LOCK);
	return MEM_SUCCESS;
}
