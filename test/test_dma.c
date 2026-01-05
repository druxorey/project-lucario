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

UTEST_MAIN();

UTEST(DMA, ExecuteSDMAOperations) {
    InstructionStatus_t ret;
    Instruction_t instruction;

    dmaReset();
    
    writeMemory(456, 1234567); // Preload memory at address 456
    
    pthread_t dmaThread;
    pthread_create(&dmaThread, NULL, &dmaInit, NULL);

    usleep(100000); // Allow DMA thread to initialize

    // Test for set platter (SDMAP)
    cpuReset();
    CPU.IR = 28100001; // SDMAP Inmediate with value 1
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.track, 1); // DMA track set to 1
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
    
    // Test for set cylinder (SDMAC)
    cpuReset();
    CPU.IR = 29100002; // SDMAC Inmediate with value 2
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.cylinder, 2); // DMA cylinder set to 2
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
        
    // Test for set sector (SDMAS)
    cpuReset();
    CPU.IR = 30100003; // SDMAS Inmediate with value 3
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.sector, 3); // DMA sector set to 3
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
        
    // Test for set operation type (SDMAIO)
    cpuReset();
    CPU.IR = 31100000; // SDMAIO Inmediate with value 0
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.ioDirection, 0); // DMA I/O direction set to Read (0)
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

    cpuReset();
    CPU.IR = 31100001; // SDMAIO Inmediate with value 1
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.ioDirection, 1); // DMA I/O direction set to Write (1)
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
        
    // Test for set memory position (SDMAM)
    cpuReset();
    CPU.IR = 32100456; // SDMAM Inmediate with value 456
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.memAddr, 456); // DMA memory address set to 456
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

    // Test for set operation on (SDMAON)
    cpuReset();
    CPU.IR = 33100001; // SDMAON Inmediate 1
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

    usleep(50000); // Allow some time for DMA operation to complete

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
    ASSERT_EQ(DMA.track, 0); // DMA track still 0 due to invalid argument
    ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
    
    // Test for set cylinder (SDMAC)
    cpuReset();
    CPU.IR = 29100010; // SDMAC Inmediate with value 10 (Invalid cylinder)
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.cylinder, 0); // DMA cylinder still 0 due to invalid argument
    ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
        
    // Test for set sector (SDMAS)
    cpuReset();
    CPU.IR = 30100100; // SDMAS Inmediate with value 100 (Invalid sector)
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.sector, 0); // DMA sector still 0 due to invalid argument
    ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
        
    // Test for set operation type (SDMAIO)
    cpuReset();
    CPU.IR = 31100003; // SDMAIO Inmediate with value 3 (Invalid I/O direction)
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.ioDirection, 0); // DMA I/O direction still set to Read (0) due to invalid argument
    ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);

    cpuReset();
    CPU.IR = 31100001; // SDMAIO Inmediate with value 1
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.ioDirection, 1); // DMA I/O direction set to Write (1)
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

    cpuReset();
    CPU.IR = 31100003; // SDMAIO Inmediate with value 3 (Invalid I/O direction)
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.ioDirection, 1); // DMA I/O direction still set to Write (1) due to invalid argument
    ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
        
    // Test for set memory position (SDMAM)
    cpuReset();
    CPU.IR = 32102000; // SDMAM Inmediate with value 2000 (Invalid memory address)
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.memAddr, 0); // DMA memory address still 0 due to invalid argument
    ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);

    cpuReset();
    CPU.IR = 32100299; // SDMAM Inmediate with value 299 (OS reserved memory address)
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ(DMA.memAddr, 0); // DMA memory address still 0 due to invalid argument
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
    
    writeMemory(456, 1234567); // Preload memory at address 456
    writeMemory(789, 7654321); // Preload memory at address 789
    
    pthread_t dmaThread;
    pthread_create(&dmaThread, NULL, &dmaInit, NULL);

    usleep(50000); // Allow DMA thread to initialize

    // Set up DMA parameters
    cpuReset();
    CPU.IR = 28100001; // SDMAP Inmediate with value 1
    instruction = decode();
    ret = executeDMAInstruction(instruction);

    cpuReset();
    CPU.IR = 29100002; // SDMAC Inmediate with value 2
    instruction = decode();
    ret = executeDMAInstruction(instruction);
        
    cpuReset();
    CPU.IR = 30100003; // SDMAS Inmediate with value 3
    instruction = decode();
    ret = executeDMAInstruction(instruction);
        
    cpuReset();
    CPU.IR = 31100001; // SDMAIO Inmediate with value 1 (Write)
    instruction = decode();
    ret = executeDMAInstruction(instruction);
        
    cpuReset();
    CPU.IR = 32100456; // SDMAM Inmediate with value 456
    instruction = decode();
    ret = executeDMAInstruction(instruction);

    // Start DMA operation
    cpuReset();
    CPU.IR = 33100000; // SDMAON Inmediate 0
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
    
    // Attempt to set I/O direction again while DMA is active          
    cpuReset();
    CPU.IR = 31100000; // SDMAIO Inmediate with value 0 (Read)
    instruction = decode();
    ret = executeDMAInstruction(instruction);

    cpuReset();
    CPU.IR = 32100789; // SDMAM Inmediate with value 789
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    
    // Attempt to start DMA operation again while it's active
    cpuReset();
    CPU.IR = 33100000; // SDMAON Inmediate 0
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret); // Should still succeed

    usleep(50000); // Allow some time for DMA operation to complete

    ASSERT_FALSE(DMA.pending);
    ASSERT_FALSE(DMA.active);
    ASSERT_EQ(DMA.status, 0);
    ASSERT_EQ((word)1234567, DISK[1][2][3].data);
    ASSERT_EQ((word)1234567, RAM[789]);
}

// Tests CPU stops at mutex when DMA is actively transferring data
// We cant verify the CPU is actually waiting, but we can execute a memory related instruction after verifying DMA is active and see if it completes correctly
UTEST(DMA, CPUWaitsWhenDMAActive) {
    InstructionStatus_t ret;
    Instruction_t instruction;

    dmaReset();

    DISK[1][2][3].data = 7654321; // Preload disk sector with data
    
    pthread_t dmaThread;
    pthread_create(&dmaThread, NULL, &dmaInit, NULL);

    usleep(100000); // Allow DMA thread to initialize

    // Set up DMA parameters
    cpuReset();
    CPU.IR = 28100001; // SDMAP Inmediate with value 1
    instruction = decode();
    ret = executeDMAInstruction(instruction);

    cpuReset();
    CPU.IR = 29100002; // SDMAC Inmediate with value 2
    instruction = decode();
    ret = executeDMAInstruction(instruction);
        
    cpuReset();
    CPU.IR = 30100003; // SDMAS Inmediate with value 3
    instruction = decode();
    ret = executeDMAInstruction(instruction);
        
    cpuReset();
    CPU.IR = 31100000; // SDMAIO Inmediate with value 0 (Read)
    instruction = decode();
    ret = executeDMAInstruction(instruction);
        
    cpuReset();
    CPU.IR = 32100456; // SDMAM Inmediate with value 456
    instruction = decode();
    ret = executeDMAInstruction(instruction);

    // Start DMA operation
    cpuReset();
    CPU.IR = 33100000; // SDMAON Inmediate 0
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
    
    // Prepare memory related instruction that should wait for DMA to complete
    RAM[300] = 4000456; // Preload memory with LOAD instruction
    CPU.PSW.pc = 300;

    // Wait until DMA is active
    while (!DMA.active) {
        usleep(1000);
    }
    
    // Execute instruction while DMA is active
    ASSERT_TRUE(DMA.pending);
    ASSERT_TRUE(DMA.active);
    cpuStep(); // This should wait until DMA is done

    ASSERT_FALSE(DMA.pending);
    ASSERT_FALSE(DMA.active);
    ASSERT_EQ(DMA.status, 0);
    ASSERT_EQ((word)7654321, RAM[456]);
    ASSERT_EQ((word)7654321, CPU.AC);
}