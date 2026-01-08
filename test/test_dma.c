#include <stdbool.h>
#include <pthread.h>

#include "../lib/utest.h"
#include "../inc/dma.h"
#include "../inc/cpu.h"
#include "../inc/memory.h"

DMA_t DMA;
CPU_t CPU;
word RAM[RAM_SIZE];
Sector_t DISK[DISK_TRACKS][DISK_CYLINDERS][DISK_SECTORS];
pthread_mutex_t BUS_LOCK = PTHREAD_MUTEX_INITIALIZER;

static bool mockMemoryFailProtection = false;

//Mock function for writing in memory
MemoryStatus_t writeMemory(address addr, word data) {
	pthread_mutex_lock(&BUS_LOCK);
	if (mockMemoryFailProtection) {
	    pthread_mutex_unlock(&BUS_LOCK);
		return MEM_ERR_PROTECTION;
	}

	if (addr >= 0 && addr < RAM_SIZE) {
		RAM[addr] = data;
	    pthread_mutex_unlock(&BUS_LOCK);
		return MEM_SUCCESS;
	}

	pthread_mutex_unlock(&BUS_LOCK);
	return MEM_ERR_OUT_OF_BOUNDS;
}

// Mock for Read Memory
MemoryStatus_t readMemory(address addr, word* outData) {
	pthread_mutex_lock(&BUS_LOCK);

	if (addr >= 0 && addr < RAM_SIZE) {
		*outData = RAM[addr];
	    pthread_mutex_unlock(&BUS_LOCK);
		return MEM_SUCCESS;
	}

	*outData = 0;

	pthread_mutex_unlock(&BUS_LOCK);
	return MEM_ERR_OUT_OF_BOUNDS;
}

MemoryStatus_t dmaWriteMemory(address addr, word data) {
	pthread_mutex_lock(&BUS_LOCK);
	if (addr >= 0 && addr < RAM_SIZE) {
		RAM[addr] = data;
		pthread_mutex_unlock(&BUS_LOCK);
		return MEM_SUCCESS;
	}
	pthread_mutex_unlock(&BUS_LOCK);
	return MEM_ERR_OUT_OF_BOUNDS;
}

MemoryStatus_t dmaReadMemory(address addr, word* outData) {
	pthread_mutex_lock(&BUS_LOCK);
	if (addr >= 0 && addr < RAM_SIZE) {
		*outData = RAM[addr];
		pthread_mutex_unlock(&BUS_LOCK);
		return MEM_SUCCESS;
	}
	*outData = 0;
	pthread_mutex_unlock(&BUS_LOCK);
	return MEM_ERR_OUT_OF_BOUNDS;
}

UTEST_MAIN();

UTEST(DMA, ExecuteSDMAOperations) {
	InstructionStatus_t ret;
	Instruction_t instruction;

	dmaReset();
	
	dmaWriteMemory(456, 1234567); // Preload memory directly
	
	pthread_t dmaThread;
	pthread_create(&dmaThread, NULL, &dmaInit, NULL);

	usleep(100000); // Allow DMA thread to initialize

	// Test for set platter (SDMAP)
	cpuReset();
	CPU.IR = 28100001; // SDMAP Inmediate with value 1
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.track, 1);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
	
	// Test for set cylinder (SDMAC)
	cpuReset();
	CPU.IR = 29100002; // SDMAC Inmediate with value 2
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.cylinder, 2);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
		
	// Test for set sector (SDMAS)
	cpuReset();
	CPU.IR = 30100003; // SDMAS Inmediate with value 3
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.sector, 3);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
		
	// Test for set operation type (SDMAIO)
	cpuReset();
	CPU.IR = 31100000; // SDMAIO Inmediate with value 0
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.ioDirection, 0);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

	cpuReset();
	CPU.IR = 31100001; // SDMAIO Inmediate with value 1
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.ioDirection, 1);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
		
	// Test for set memory position (SDMAM)
	cpuReset();
	// IMPORTANTE: Seteamos RL para que la validación de seguridad en cpu.c pase
	CPU.RL = RAM_SIZE; 
	CPU.IR = 32100456; // SDMAM Inmediate with value 456
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.memAddr, 456);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

	// Test for set operation on (SDMAON)
	cpuReset();
	CPU.IR = 33100001; // SDMAON Inmediate 1
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

	usleep(20000); // Allow some time for DMA operation to complete

	ASSERT_FALSE(DMA.pending);
	ASSERT_FALSE(DMA.active);
	ASSERT_EQ(DMA.status, 0);
	ASSERT_EQ((word)1234567, DISK[1][2][3].data);
}

UTEST(DMA, ExecuteInvalidSDMAOperations) {
	InstructionStatus_t ret;
	Instruction_t instruction;

	dmaReset();

	// Test for set platter (SDMAP)
	cpuReset();
	CPU.IR = 28100010; // SDMAP Inmediate with value 10 (Invalid track)
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.track, 0);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
	
	// Test for set cylinder (SDMAC)
	cpuReset();
	CPU.IR = 29100010; // SDMAC Inmediate with value 10 (Invalid cylinder)
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.cylinder, 0);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
		
	// Test for set sector (SDMAS)
	cpuReset();
	CPU.IR = 30100100; // SDMAS Inmediate with value 100 (Invalid sector)
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.sector, 0);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
		
	// Test for set operation type (SDMAIO)
	cpuReset();
	CPU.IR = 31100003; 
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.ioDirection, 0);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);

	cpuReset();
	CPU.IR = 31100001; 
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.ioDirection, 1);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

	cpuReset();
	CPU.IR = 31100003; 
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.ioDirection, 1);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
		
	// Test for set memory position (SDMAM)
	cpuReset();
	// Aquí NO seteamos RL (es 0), por lo que cualquier dirección > 0 fallará.
	// Esto verifica que OP_SDMAM ahora sí chequea protección.
	CPU.IR = 32102000; // SDMAM Inmediate with value 2000 (Invalid memory address)
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.memAddr, 0);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);

	cpuReset();
	CPU.IR = 32100299; // SDMAM 299
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ(DMA.memAddr, 0);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
}

//Test execution of non-DMA instructions with DMA execution function
UTEST(DMA, ExecuteNonDMAInstruction) {
	InstructionStatus_t ret;
	Instruction_t instruction;

	dmaReset();

	// Test for non-DMA instruction (LOAD)
	cpuReset();
	CPU.IR = 10000456; // LOAD Inmediate with value 456
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
}

// Test execution of SDMAON when DMA is already active
UTEST(DMA, ExecuteSDMAONWhenActive) {
	InstructionStatus_t ret;
	Instruction_t instruction;

	dmaReset();
	
	dmaWriteMemory(456, 1234567);
	dmaWriteMemory(789, 7654321);
	
	pthread_t dmaThread;
	pthread_create(&dmaThread, NULL, &dmaInit, NULL);

	usleep(100000); // Allow DMA thread to initialize

	// Set up DMA parameters
	cpuReset();
	CPU.IR = 28100001; 
	instruction = decode();
	ret = executeDMAInstruction(instruction);

	cpuReset();
	CPU.IR = 29100002; 
	instruction = decode();
	ret = executeDMAInstruction(instruction);

	cpuReset();
	CPU.IR = 30100003; 
	instruction = decode();
	ret = executeDMAInstruction(instruction);

	cpuReset();
	CPU.IR = 31100001; // Write
	instruction = decode();
	ret = executeDMAInstruction(instruction);

	cpuReset();
	CPU.RL = RAM_SIZE; // Seteamos RL para permitir la configuración
	CPU.IR = 32100456; 
	instruction = decode();
	ret = executeDMAInstruction(instruction);

	// Start DMA operation
	cpuReset();
	CPU.IR = 33100000; 
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

	// Attempt to set I/O direction again while DMA is active
	cpuReset();
	CPU.IR = 31100000; 
	instruction = decode();
	ret = executeDMAInstruction(instruction);

	cpuReset();
	CPU.RL = RAM_SIZE;
	CPU.IR = 32100789; 
	instruction = decode();
	ret = executeDMAInstruction(instruction);

	// Attempt to start DMA operation again while it's active
	cpuReset();
	CPU.IR = 33100000; 
	instruction = decode();
	ret = executeDMAInstruction(instruction);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret); 

	usleep(20000); // Allow some time for DMA operation to complete

	ASSERT_FALSE(DMA.pending);
	ASSERT_FALSE(DMA.active);
	ASSERT_EQ(DMA.status, 0);
	ASSERT_EQ((word)1234567, DISK[1][2][3].data);
	ASSERT_EQ((word)1234567, RAM[789]);
}
