#include <stdbool.h>
#include "../lib/utest.h"
#include "../inc/dma.h"
#include "../inc/memory.h"
#include "../inc/cpu.h"

DMA_t DMA;
CPU_t CPU;
word RAM[RAM_SIZE];
Sector_t DISK[DISK_TRACKS][DISK_CYLINDERS][DISK_SECTORS][SECTOR_SIZE];

// Mock function for writing in memory
MemoryStatus_t writeMemory(address addr, word value) {
	if (addr >= OS_RESERVED_SIZE && addr < RAM_SIZE) {
		RAM[addr] = value;
		return MEM_SUCCESS;
	}
	return MEM_ERR_OUT_OF_BOUNDS;
}

UTEST_MAIN();

UTEST(DMA, PlaceholderTest) {
    // Placeholder test to ensure DMA module is included in the build.
    ASSERT_TRUE(true);
}

UTEST(DMA, ExecuteSDMAOperations) {
    InstructionStatus_t ret;
    Instruction_t instruction;

    dmaReset();

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
    // Its tricky to test the active status, so we just check for a successful operation
    cpuReset();
    writeMemory(456, 1234567); // Preload memory at address 456
    CPU.IR = 33100001; // SDMAON Inmediate with value 1
    instruction = decode();
    ret = executeDMAInstruction(instruction);
    ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
    ASSERT_EQ(DMA.status, 0); // DMA transfer status success
    ASSERT_EQ(DISK[DMA.track][DMA.cylinder][DMA.sector], 1234567); // Word transferred to disk successfully
}