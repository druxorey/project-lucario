#include <stdbool.h>
#include "../lib/utest.h"
#include "../inc/cpu.h"
#include "../inc/memory.h"

CPU_t CPU;
word RAM[RAM_SIZE];

// Mock function for writing in memory
MemoryStatus_t writeMemory(address addr, word value) {
	if (addr >= OS_RESERVED_SIZE && addr < RAM_SIZE) {
		RAM[addr] = value;
		return MEM_SUCCESS;
	}
	return MEM_ERR_OUT_OF_BOUNDS;
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
	cpuReset();
	writeMemory(300, 4100005);
	CPU.PSW.pc = 300;
	fetch();
	ASSERT_EQ(CPU.MAR, 300);
	ASSERT_EQ(CPU.MDR, 4100005);
	ASSERT_EQ(CPU.IR, 4100005);
	ASSERT_EQ(CPU.PSW.pc, 301);
}

// Verify that the fetch stage correctly manages out-of-bounds memory access
UTEST(CPU, FetchStageOutOfBounds) {
	cpuReset();
	writeMemory(2001, 4100005);
	CPU.PSW.pc = 2001;
	fetch();
	ASSERT_EQ(CPU.MAR, 2001);
	ASSERT_EQ(CPU.MDR, 0);
	ASSERT_EQ(CPU.IR, 0);
	ASSERT_EQ(CPU.PSW.pc, 2001);
}

// Verify that the decode stage correctly interprets the instruction in IR
UTEST(CPU, DecodeStage) {
	cpuReset();
	CPU.IR = 4100005;
	Instruction_t inst;
	inst = decode();
	ASSERT_EQ(inst.opCode, (unsigned)OP_LOAD);
	ASSERT_EQ(inst.direction, (unsigned)DIR_IMMEDIATE);
	ASSERT_EQ(inst.value, 5);
}

// Verify that the execute stage correctly handles an valid instruction
UTEST(CPU, ExecuteStage) {
	cpuReset();
	CPU.AC = 7;
	CPU.IR = 100014; // OP_SUM, Immediate, 14
	Instruction_t inst;
	inst = decode();
	ASSERT_EQ(inst.opCode, (unsigned)OP_SUM);
	ASSERT_EQ(inst.direction, (unsigned)DIR_IMMEDIATE);
	ASSERT_EQ(inst.value, 14);
	CPUStatus_t status = execute(inst);
	ASSERT_EQ(CPU.AC, 21);
	ASSERT_EQ(status, (unsigned)CPU_OK);
}

// Verify that the execute stage correctly handles an invalid instruction
UTEST(CPU, ExecuteStageDefault) {
	cpuReset();
	CPU.IR = 34100005; // Invalid OpCode (0-33 are valid)
	Instruction_t inst;
	inst = decode();
	ASSERT_EQ(inst.opCode, (unsigned)34);
	ASSERT_EQ(inst.direction, (unsigned)DIR_IMMEDIATE);
	ASSERT_EQ(inst.value, 5);
	CPUStatus_t status = execute(inst);
	ASSERT_EQ(status, (unsigned)CPU_STOP);
}

// Verify that instruction cycle goes correctly through fetch, decode, and execute given a valid instruction
UTEST(CPU, InstructionCycleValidInstruction) {
	cpuReset();
	writeMemory(400, 100010);
	CPU.PSW.pc = 400;
	bool stepResult = cpuStep();
	ASSERT_TRUE(stepResult);
	ASSERT_EQ(CPU.PSW.pc, 401);
}

// Verify that instruction cycle manages correctly an invalid instruction
UTEST(CPU, InstructionCycleInvalidInstruction) {
	cpuReset();
	writeMemory(321, 45678901);
	CPU.PSW.pc = 321;
	bool stepResult = cpuStep();
	ASSERT_FALSE(stepResult);
	ASSERT_EQ(CPU.PSW.pc, 322);
}

// Verify that instruction cycle manages correctly an invalid address
UTEST(CPU, InstructionCycleInvalidAddress) {
	cpuReset();
	writeMemory(2001, 45678901);
	CPU.PSW.pc = 2001;
	bool stepResult = cpuStep();
	ASSERT_FALSE(stepResult);
	ASSERT_EQ(CPU.PSW.pc, 2001);
}

// Verify CPU resets correctly
UTEST(CPU, CPUReset) {
	CPU.AC = 12345;
	CPU.IR = 4100005;
	CPU.MAR = 500;
	CPU.MDR = 4100005;
	CPU.RB = 300;
	CPU.RL = 800;
	CPU.RX = 50;
	CPU.SP = 1000;
	CPU.PSW.mode = MODE_KERNEL;
	CPU.PSW.conditionCode = CC_OVERFLOW;
	CPU.PSW.interruptEnable = true;
	CPU.PSW.pc = 501;

	cpuReset();

	ASSERT_EQ(CPU.AC, 0);
	ASSERT_EQ(CPU.IR, 0);
	ASSERT_EQ(CPU.MAR, 0);
	ASSERT_EQ(CPU.MDR, 0);
	ASSERT_EQ(CPU.RB, 0);
	ASSERT_EQ(CPU.RL, 0);
	ASSERT_EQ(CPU.RX, 0);
	ASSERT_EQ(CPU.SP, 0);
	ASSERT_EQ(CPU.PSW.mode, (unsigned)MODE_USER);
	ASSERT_EQ(CPU.PSW.conditionCode, (unsigned)CC_ZERO);
	ASSERT_EQ(CPU.PSW.interruptEnable, (unsigned)ITR_DISABLED);
	ASSERT_EQ(CPU.PSW.pc, 0);
}
