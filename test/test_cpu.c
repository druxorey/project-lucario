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

// Helper function to get a clean PSW
PSW_t getCleanPSW(void) {
	PSW_t psw;
	psw.conditionCode = CC_ZERO;
	psw.mode = MODE_KERNEL;
	psw.interruptEnable = ITR_DISABLED;
	psw.pc = 0;
	return psw;
}

// Helper function to setup a clean CPU state
void setupCpuClean(void) {
	CPU = (CPU_t){0};
	CPU.PSW.mode = MODE_KERNEL;
	CPU.PSW.interruptEnable = ITR_DISABLED;
	CPU.PSW.conditionCode = CC_ZERO;
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

// Test conversion from word (sign-magnitude) to int
UTEST(CPU_ALU, WordToInt) {
	// 00000005 -> 5
	word w = 5;
	int res = wordToInt(w);
	ASSERT_EQ(5, res);

	// 10000005 -> -5 (Bit de signo activo)
	w = 10000005;
	res = wordToInt(w);
	ASSERT_EQ(-5, res);

	// 00000000 -> 0
	ASSERT_EQ(0, wordToInt(0));
	ASSERT_EQ(0, wordToInt(10000000));
}

// Test conversion from int to word (sign-magnitude)
UTEST(CPU_ALU, IntToWord) {
	PSW_t psw = getCleanPSW();
	word res = intToWord(10, &psw);
	ASSERT_EQ(10, res);
	ASSERT_EQ((unsigned)CC_POS, psw.conditionCode);

	psw = getCleanPSW();
	res = intToWord(-20, &psw);
	ASSERT_EQ(10000020, res);
	ASSERT_EQ((unsigned)CC_NEG, psw.conditionCode);

	psw = getCleanPSW();
	res = intToWord(0, &psw);
	ASSERT_EQ(0, res);
	ASSERT_EQ((unsigned)CC_ZERO, psw.conditionCode);

	psw = getCleanPSW();
	res = intToWord(10000000, &psw);
	ASSERT_EQ((unsigned)CC_OVERFLOW, psw.conditionCode);
	ASSERT_TRUE(res <= MAX_MAGNITUDE);
}

// Test execution of arithmetic operations
UTEST(CPU_ALU, ExecArithmetic) {
	setupCpuClean();
	CPU.AC = 5;
	
	executeArithmetic(OP_SUM, 10);
	ASSERT_EQ(15, CPU.AC);
	ASSERT_EQ((unsigned)CC_POS, CPU.PSW.conditionCode);

	setupCpuClean();
	CPU.AC = 5;

	executeArithmetic(OP_RES, 10);
	ASSERT_EQ(SIGN_BIT + 5, CPU.AC);
	ASSERT_EQ((unsigned)CC_NEG, CPU.PSW.conditionCode);

	setupCpuClean();
	CPU.AC = 10000002;
	
	executeArithmetic(OP_MULT, 10000003);
	ASSERT_EQ(6, CPU.AC);
	ASSERT_EQ((unsigned)CC_POS, CPU.PSW.conditionCode);

	setupCpuClean();
	CPU.AC = 20;

	executeArithmetic(OP_DIVI, 4);
	ASSERT_EQ(5, CPU.AC);
}

// Test division by zero handling
UTEST(CPU_ALU, ExecArithmeticDivByZero) {
	setupCpuClean();
	CPU.AC = 10;
	executeArithmetic(OP_DIVI, 0);
	ASSERT_EQ(10, CPU.AC);
}

// Test execution of non-arithmetic operations using the ALU
UTEST(CPU_ALU, ExecNonArithmeticOperation){
	setupCpuClean();
	CPU.AC = 5;
	executeArithmetic(OP_COMP, 5);
	ASSERT_EQ(5, CPU.AC);
	ASSERT_EQ((unsigned)CC_OVERFLOW, CPU.PSW.conditionCode);
}
