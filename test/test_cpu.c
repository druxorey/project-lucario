#include "../lib/utest.h"
#include "../inc/cpu.h"
#include "../inc/memory.h"

CPU_t CPU;
word RAM[RAM_SIZE];
bool isDebugMode = false;

// Mock for CPU Run (Normal Mode)
int cpuRun(void) {
	return 1;
}

// Mock for CPU Step (Debug Mode)
bool cpuStep(void) {
	return false;
}

// Mock for CPU Reset (Normal Mode)
void cpuReset(void) {
	return;
}

//Mock function for writing in memory
MemoryStatus_t writeMemory(address addr, word value) {
	if (addr >= OS_RESERVED_SIZE && addr < RAM_SIZE) {
		RAM[addr] = value;
		return 0;
	}
	return 1;
}

// Mock for Read Memory (Bypasses MMU and Mutex for Unit Testing)
MemoryStatus_t readMemory(address addr, word* outData) {
    if (addr >= 0 && addr < RAM_SIZE) {
        *outData = RAM[addr];
        return MEM_SUCCESS;
    }
    *outData = 0;
    return MEM_ERR_OUT_OF_BOUNDS;
}

UTEST_MAIN();

// Verify that the fetch stage correctly loads instruction into IR
UTEST(CPU, FetchStage) {
	writeMemory(300, 04100005);
	CPU.PSW.pc = 300;
	fetch();
	ASSERT_EQ(CPU.MAR, 300);
	ASSERT_EQ(CPU.MDR, 04100005);
	ASSERT_EQ(CPU.IR, 04100005);
	ASSERT_EQ(CPU.PSW.pc, 301);
}

// Verify that the decode stage correctly interprets the instruction in IR
UTEST(CPU, DecodeStage) {
	CPU.IR = 04100005;
	Instruction_t inst;
	inst = decode();
	ASSERT_EQ(inst.opCode, OP_LOAD);
	ASSERT_EQ(inst.direction, DIR_DIRECT);
	ASSERT_EQ(inst.value, 5);
}
