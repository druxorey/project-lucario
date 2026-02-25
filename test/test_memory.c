#include <stdbool.h>

#include "../lib/utest.h"
#include "../inc/hardware/memory.h"

// Global mock CPU instance
CPU_t CPU;

UTEST_MAIN();

// Auxiliary function for thread
void* threadWorker(void* arg) {
	int id = *((int*)arg);
	free(arg);
	writeMemory(OS_RESERVED_SIZE + id, id * 10);
	return NULL;
}

// Verify that logical address translates correctly.
UTEST(Memory, ReadWriteValidUserMode) {
	memoryInit();
	CPU.PSW.mode = MODE_USER;
	CPU.RB = 300;
	CPU.RL = 400;

	address logicalAddr = 50;
	word data = 1234567;
	word out = 0;

	MemoryStatus_t wStatus = writeMemory(logicalAddr, data);
	MemoryStatus_t rStatus = readMemory(logicalAddr, &out);

	EXPECT_EQ((unsigned)MEM_SUCCESS, wStatus);
	EXPECT_EQ((unsigned)MEM_SUCCESS, rStatus);
	EXPECT_EQ(data, out);
	
	// Verify the white-box: Was it written to physical 350?
	// Switch to Kernel to read the absolute address and verify translation
	CPU.PSW.mode = MODE_KERNEL;
	word physVal = 0;
	readMemory(350, &physVal);
	EXPECT_EQ(data, physVal);
}

// Verify that user mode cannot access memory outside its RB/RL bounds.
UTEST(Memory, UserModeProtectionFault) {
	memoryInit();
	CPU.PSW.mode = MODE_USER;
	CPU.RB = 300;
	CPU.RL = 340;

	address invalidLogicalAddr = 50;
	word data = 4321;
	word out = 0;

	MemoryStatus_t status = writeMemory(invalidLogicalAddr, data);
	EXPECT_EQ((unsigned)MEM_ERR_PROTECTION, status);

	status = readMemory(invalidLogicalAddr, &out);
	EXPECT_EQ((unsigned)MEM_ERR_PROTECTION, status);
}

// The Kernel should be able to write anywhere (e.g., load the OS).
UTEST(Memory, KernelBypassesProtection) {
	memoryInit();
	CPU.PSW.mode = MODE_KERNEL;

	address addr = 100;
	word data = 999;
	word out = 0;

	MemoryStatus_t res = writeMemory(addr, data);
	EXPECT_EQ((unsigned)MEM_SUCCESS, res);
	
	readMemory(addr, &out);
	EXPECT_EQ(data, out);
}

// Verify that it does not allow storing numbers larger than 7 digits.
UTEST(Memory, DataMagnitudeCheck) {
	memoryInit();
	CPU.PSW.mode = MODE_KERNEL;

	word invalidData = 199999999;

	MemoryStatus_t status = writeMemory(500, invalidData);
	EXPECT_EQ((unsigned)MEM_ERR_INVALID_DATA, status);
}

// Verify that it allows storing negative numbers within the 7-digit limit.
UTEST(Memory, NegativeNumberWrite) {
	memoryInit();
	CPU.PSW.mode = MODE_USER;
	CPU.RB = 300;
	CPU.RL = 400;

	word negativeFive = 10000005; 

	MemoryStatus_t status = writeMemory(0, negativeFive);

	EXPECT_EQ((unsigned)MEM_SUCCESS, status); // Ahora debería pasar
}

// Verify that multiple threads can safely write to different memory locations.
UTEST(Memory, ThreadSafetyCheck) {
	memoryInit();
	CPU.PSW.mode = MODE_KERNEL;
	
	pthread_t threads[5];
	for(int i=0; i<5; i++) {
		int* arg = malloc(sizeof(int));
		*arg = i;
		pthread_create(&threads[i], NULL, threadWorker, arg);
	}
	
	for(int i=0; i<5; i++) pthread_join(threads[i], NULL);
	
	for(int i=0; i<5; i++) {
		word out;
		readMemory(OS_RESERVED_SIZE + i, &out);
		EXPECT_EQ(i * 10, out);
	}
}
