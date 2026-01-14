#include <stdbool.h>
#include "../lib/utest.h"
#include "../inc/cpu.h"
#include "../inc/memory.h"

CPU_t CPU;
DMA_t DMA;
word RAM[RAM_SIZE];

pthread_cond_t DMA_COND = PTHREAD_COND_INITIALIZER;
pthread_mutex_t BUS_LOCK = PTHREAD_MUTEX_INITIALIZER;

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

// Auxiliary function for CPU setting
static void cpuSetup(void) {
	cpuReset();
	CPU.PSW.interruptEnable = ITR_ENABLED;
}

UTEST_MAIN();

// Verify that the fetch stage correctly loads instruction into IR
UTEST(CPU, FetchStage) {
	cpuSetup();
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
	cpuSetup();
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
	cpuSetup();
	CPU.IR = 4100005;
	Instruction_t inst;
	inst = decode();
	ASSERT_EQ(inst.opCode, (unsigned)OP_LOAD);
	ASSERT_EQ(inst.direction, (unsigned)ADDR_MODE_IMMEDIATE);
	ASSERT_EQ(inst.value, 5);
}

// Verify that the execute stage correctly handles an valid instruction
UTEST(CPU, ExecuteStage) {
	cpuSetup();
	CPU.AC = 7;
	CPU.IR = 100014; // OP_SUM, Immediate, 14
	Instruction_t inst;
	inst = decode();
	ASSERT_EQ(inst.opCode, (unsigned)OP_SUM);
	ASSERT_EQ(inst.direction, (unsigned)ADDR_MODE_IMMEDIATE);
	ASSERT_EQ(inst.value, 14);
	CPUStatus_t status = execute(inst);
	ASSERT_EQ(CPU.AC, 21);
	ASSERT_EQ(status, (unsigned)CPU_OK);
}

// Verify that the execute stage correctly handles an invalid instruction
UTEST(CPU, ExecuteStageDefault) {
	cpuSetup();
	CPU.IR = 34100005; // Invalid OpCode (0-33 are valid)
	Instruction_t inst;
	inst = decode();
	ASSERT_EQ(inst.opCode, (unsigned)34);
	ASSERT_EQ(inst.direction, (unsigned)ADDR_MODE_IMMEDIATE);
	ASSERT_EQ(inst.value, 5);
	CPUStatus_t status = execute(inst);
	ASSERT_EQ(status, (unsigned)CPU_STOP);
}

// Verify that instruction cycle goes correctly through fetch, decode, and execute given a valid instruction
UTEST(CPU, InstructionCycleValidInstruction) {
	cpuSetup();
	writeMemory(400, 100001); // SUM Inmediate with value 1
	CPU.PSW.pc = 400;
	bool stepResult = cpuStep();
	ASSERT_TRUE(stepResult);
	ASSERT_EQ(CPU.PSW.pc, 401);
}

// Verify that instruction cycle manages correctly an invalid instruction
UTEST(CPU, InstructionCycleInvalidInstruction) {
	cpuSetup();
	writeMemory(321, 45678901);
	CPU.PSW.pc = 321;
	bool stepResult = cpuStep();
	ASSERT_FALSE(stepResult);
	ASSERT_EQ(CPU.PSW.pc, 322);
}

// Verify that instruction cycle manages correctly an invalid address
UTEST(CPU, InstructionCycleInvalidAddress) {
	cpuSetup();
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

UTEST(CPU, CheckNoPendingInterrupts) {
	cpuSetup();
	bool result = checkInterrupts();
	ASSERT_TRUE(result);
}

UTEST(CPU, CheckPendingInterrupts) {
	Instruction_t instruction;
	bool result;

	// Invalid Instruction Interrupt
	cpuSetup();
	raiseInterrupt(IC_INVALID_INSTR);
	result = checkInterrupts();
	ASSERT_FALSE(result);

	cpuSetup();
	CPU.IR = 34100005; // Invalid instruction
	instruction = decode();
	result = execute(instruction);
	ASSERT_EQ(result, CPU_STOP);
	result = checkInterrupts();
	ASSERT_FALSE(result);

	// Invalid Address Interrupt
	cpuSetup();
	raiseInterrupt(IC_INVALID_ADDR);
	result = checkInterrupts();
	ASSERT_FALSE(result);

	cpuSetup();
	CPU.IR = 4002000; // LOAD from invalid address
	instruction = decode();
	result = execute(instruction);
	ASSERT_EQ(result, CPU_STOP);
	result = checkInterrupts();
	ASSERT_FALSE(result);

	// Overflow Interrupt
	cpuSetup();
	CPU.AC = MAX_MAGNITUDE+1;
	raiseInterruptRelated(IC_OVERFLOW, &CPU.AC);
	result = checkInterrupts();
	ASSERT_TRUE(result);

	cpuSetup();
	CPU.AC = MAX_MAGNITUDE;
	CPU.IR = 100001; // SUM Immediate 1
	instruction = decode();
	result = execute(instruction);
	ASSERT_EQ(result, CPU_OK);
	result = checkInterrupts();
	ASSERT_TRUE(result);
	ASSERT_EQ(CPU.AC, 0); // Value wrapped around after overflow

	// Underflow Interrupt
	// There's not current implementation that generates underflow, so we manually raise it
	cpuSetup();
	CPU.AC = -MAX_MAGNITUDE-1;
	raiseInterruptRelated(IC_UNDERFLOW, &CPU.AC);
	result = checkInterrupts();
	ASSERT_TRUE(result);

	// Timer Interrupt
	// Timer interrupts are usually periodic, so we just test that the interrupt is handled correctly
	cpuSetup();
	raiseInterrupt(IC_TIMER);
	result = checkInterrupts();
	ASSERT_TRUE(result);

	// IO Done Interrupt
	cpuSetup();
	raiseInterrupt(IC_IO_DONE);
	result = checkInterrupts();
	ASSERT_TRUE(result);

	// System Call Interrupt
	cpuSetup();
	CPU.AC = 0; // EXIT syscall
	raiseInterrupt(IC_SYSCALL);
	result = checkInterrupts();
	ASSERT_FALSE(result);

	cpuSetup();
	CPU.AC = 10; // NOT EXIT syscall
	raiseInterrupt(IC_SYSCALL);
	result = checkInterrupts();
	ASSERT_TRUE(result);

	cpuSetup();
	CPU.AC = 0; // EXIT syscall
	CPU.IR = 13100000; // SVC
	instruction = decode();
	result = execute(instruction);
	ASSERT_EQ(result, CPU_OK);
	result = checkInterrupts();
	ASSERT_FALSE(result);

	cpuSetup();
	CPU.AC = 21; // Other syscall
	CPU.IR = 13100000; // SVC
	instruction = decode();
	result = execute(instruction);
	ASSERT_EQ(result, CPU_OK);
	result = checkInterrupts();
	ASSERT_TRUE(result);

	// Invalid Syscall Interrupt
	// There's not current implementation that generates invalid syscall, so we manually raise it
	cpuSetup();
	raiseInterrupt(IC_INVALID_SYSCALL);
	result = checkInterrupts();
	ASSERT_TRUE(result);

	// Invalid Interrupt Code Interrupt
	cpuSetup();
	raiseInterrupt(IC_INVALID_INT_CODE);
	result = checkInterrupts();
	ASSERT_TRUE(result);
}
