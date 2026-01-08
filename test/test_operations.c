#include <stdbool.h>
#include "../lib/utest.h"
#include "../inc/cpu.h"
#include "../inc/memory.h"

CPU_t CPU;
DMA_t DMA;
word RAM[RAM_SIZE];

pthread_cond_t DMA_COND = PTHREAD_COND_INITIALIZER;
pthread_mutex_t BUS_LOCK = PTHREAD_MUTEX_INITIALIZER;

static bool mockMemoryFailProtection = false;

//Mock function for writing in memory
MemoryStatus_t writeMemory(address addr, word data) {
	if (mockMemoryFailProtection) {
		return MEM_ERR_PROTECTION;
	}

	if (addr >= 0 && addr < RAM_SIZE) {
		RAM[addr] = data;
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

// Helper function to get a clean PSW
static PSW_t setupCleanPSW(void) {
	PSW_t psw;
	psw.mode = MODE_USER;
	psw.interruptEnable = ITR_DISABLED;
	psw.conditionCode = CC_ZERO;
	psw.pc = 0;
	return psw;
}

// Helper function to setup a clean CPU state
static void setupCleanCPU(void) {
	CPU = (CPU_t){0};
	CPU.PSW.mode = MODE_USER;
	CPU.PSW.interruptEnable = ITR_DISABLED;
	CPU.PSW.conditionCode = CC_ZERO;
}


UTEST_MAIN();

// Test conversion from word (sign-magnitude) to int
UTEST(CPU_ALU, ConversionWordToInt) {
	ASSERT_EQ(5, wordToInt(5));
	ASSERT_EQ(-5, wordToInt(SIGN_BIT + 5));

	ASSERT_EQ(0, wordToInt(0));
	ASSERT_EQ(0, wordToInt(SIGN_BIT)); // It should'nt exist -0, just 0

	ASSERT_EQ(9999999, wordToInt(9999999));
	ASSERT_EQ(-9999999, wordToInt(SIGN_BIT + 9999999));
}

// Test conversion from int to word (sign-magnitude)
UTEST(CPU_ALU, ConversionIntToWord) {
	PSW_t psw = setupCleanPSW();
	word result;

	result = intToWord(10, &psw);
	ASSERT_EQ(10, result);
	ASSERT_EQ((unsigned)CC_POS, psw.conditionCode);

	psw = setupCleanPSW();
	result = intToWord(-20, &psw);
	ASSERT_EQ(SIGN_BIT + 20, result);
	ASSERT_EQ((unsigned)CC_NEG, psw.conditionCode);

	psw = setupCleanPSW();
	result = intToWord(0, &psw);
	ASSERT_EQ(0, result);
	ASSERT_EQ((unsigned)CC_ZERO, psw.conditionCode);

	psw = setupCleanPSW();
	// This should trigger overflow on positive side
	result = intToWord(10000000, &psw);
	ASSERT_EQ((unsigned)CC_OVERFLOW, psw.conditionCode);
	ASSERT_EQ(0, result);

	psw = setupCleanPSW();
	// This should trigger overflow on negative side
	result = intToWord(-10000001, &psw);
	ASSERT_EQ((unsigned)CC_OVERFLOW, psw.conditionCode);
	ASSERT_EQ(SIGN_BIT + 1, result);
}

// Test execution of arithmetic operations
UTEST(CPU_ALU, ExecuteArithmeticOperation) {
	InstructionStatus_t ret;
	Instruction_t instruction;

	// Test for sum: 5 + 10 = 15
	setupCleanCPU();
	CPU.AC = 5;
	instruction.opCode = OP_SUM;
	instruction.direction = ADDR_MODE_IMMEDIATE;
	instruction.value = 10;

	ret = executeArithmetic(instruction);
	
	ASSERT_EQ(15, CPU.AC);
	ASSERT_EQ((unsigned)CC_POS, CPU.PSW.conditionCode);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

	// Test for subtraction: 5 - 10 = -5
	setupCleanCPU();
	CPU.AC = 5;
	instruction.opCode = OP_RES;
	instruction.direction = ADDR_MODE_IMMEDIATE;
	instruction.value = 10;

	ret = executeArithmetic(instruction);
	
	ASSERT_EQ(SIGN_BIT + 5, CPU.AC);
	ASSERT_EQ((unsigned)CC_NEG, CPU.PSW.conditionCode);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
	
	// Test for subtraction with Direct Address: -5 - (-10) = 5
	setupCleanCPU();
	CPU.AC = SIGN_BIT + 5; // -5
	writeMemory(300, intToWord(-10, &CPU.PSW)); // RAM[300] = -10
	
	instruction.opCode = OP_RES;
	instruction.direction = ADDR_MODE_DIRECT;
	instruction.value = 300;

	ret = executeArithmetic(instruction);
	
	ASSERT_EQ(5, CPU.AC);
	ASSERT_EQ((unsigned)CC_POS, CPU.PSW.conditionCode);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

	// Test for multiplication: -2 * -3 = 6
	setupCleanCPU();
	CPU.AC = SIGN_BIT + 2;
	writeMemory(300, intToWord(-3, &CPU.PSW)); // RAM[300] = -3
	
	instruction.opCode = OP_MULT;
	instruction.direction = ADDR_MODE_DIRECT;
	instruction.value = 300;

	ret = executeArithmetic(instruction);
	
	ASSERT_EQ(6, CPU.AC);
	ASSERT_EQ((unsigned)CC_POS, CPU.PSW.conditionCode);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

	// Test for division: 20 / 4 = 5
	setupCleanCPU();
	CPU.AC = 20;
	
	instruction.opCode = OP_DIVI;
	instruction.direction = ADDR_MODE_IMMEDIATE;
	instruction.value = 4;

	ret = executeArithmetic(instruction);
	
	ASSERT_EQ(5, CPU.AC);
	ASSERT_EQ((unsigned)CC_POS, CPU.PSW.conditionCode);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

	// Test for integer division: 5 / 2 = 2
	setupCleanCPU();
	CPU.AC = 5;
	
	instruction.opCode = OP_DIVI;
	instruction.direction = ADDR_MODE_IMMEDIATE;
	instruction.value = 2;

	ret = executeArithmetic(instruction);
	
	ASSERT_EQ(2, CPU.AC);
	ASSERT_EQ((unsigned)CC_POS, CPU.PSW.conditionCode);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);

	// Overflow test for addition
	setupCleanCPU();
	CPU.AC = MAX_MAGNITUDE;
	
	instruction.opCode = OP_SUM;
	instruction.direction = ADDR_MODE_IMMEDIATE;
	instruction.value = 1;

	ret = executeArithmetic(instruction);
	
	ASSERT_EQ(0, CPU.AC);
	ASSERT_EQ((unsigned)CC_OVERFLOW, CPU.PSW.conditionCode);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
}

// Test division by zero handling
UTEST(CPU_ALU, ExecuteArithmeticDivByZero) {
	Instruction_t instruction;

	setupCleanCPU();
	CPU.AC = 10;
	
	instruction.opCode = OP_DIVI;
	instruction.direction = ADDR_MODE_IMMEDIATE;
	instruction.value = 0;

	InstructionStatus_t ret = executeArithmetic(instruction);
	
	ASSERT_EQ(10, CPU.AC);
	ASSERT_EQ((unsigned)CC_OVERFLOW, CPU.PSW.conditionCode);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
}

// Test execution of non-arithmetic operations using the ALU
UTEST(CPU_ALU, ExecuteNonArithmeticOperation){
	Instruction_t instruction;

	setupCleanCPU();
	CPU.AC = 5;
	
	instruction.opCode = OP_COMP;
	instruction.direction = ADDR_MODE_IMMEDIATE;
	instruction.value = 5;

	InstructionStatus_t status = executeArithmetic(instruction);
	
	ASSERT_EQ(5, CPU.AC);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, status);
}

// Test immediate addressing mode (ADDR_MODE_IMMEDIATE = 0)
UTEST(CPU_Addressing, FetchImmediateValue) {
	setupCleanCPU();
	Instruction_t instr;
	word result;

	instr.opCode = OP_LOAD;
	instr.direction = ADDR_MODE_IMMEDIATE;
	instr.value = 9999;

	InstructionStatus_t ret = fetchOperand(instr, &result);

	ASSERT_EQ(9999, result);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
}

// Test direct addressing mode (ADDR_MODE_DIRECT = 1)
UTEST(CPU_Addressing, FetchDirectValue) {
	setupCleanCPU();
	address addr = 500;
	Instruction_t instr;
	word result, data = 12345;
	
	writeMemory(addr, data);

	instr.opCode = OP_LOAD;
	instr.direction = ADDR_MODE_DIRECT;
	instr.value = addr;

	InstructionStatus_t ret = fetchOperand(instr, &result);

	ASSERT_EQ(data, result);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
}

// Test indexed addressing mode (ADDR_MODE_INDEXED = 2)
UTEST(CPU_Addressing, FetchIndexedValue) {
	setupCleanCPU();
	address addr = 300;
	Instruction_t instr;
	word result, data = 305, index = 5;

	writeMemory(addr + index, data);

	CPU.AC = intToWord(addr, &CPU.PSW);
	instr.opCode = OP_LOAD;
	instr.direction = ADDR_MODE_INDEXED;
	instr.value = index;

	InstructionStatus_t ret = fetchOperand(instr, &result);

	ASSERT_EQ(data, result);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, ret);
}

// Test out-of-bounds access handling
UTEST(CPU_Addressing, FetchOutOfBoundsValue) {
	setupCleanCPU();
	Instruction_t instr;
	word result;

	instr.opCode = OP_LOAD;
	instr.direction = ADDR_MODE_DIRECT;
	instr.value = 99999;

	InstructionStatus_t ret = fetchOperand(instr, &result);
	
	ASSERT_EQ(0, result);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, ret);
}

// Verify OP_STR in all 3 modes (Direct, Indexed as per PDF, Immediate) and error handling.
UTEST(CPU_DataMov, StoreInstruction) {
	address addr = 400;
	Instruction_t instr;
	word result, data = intToWord(69, &CPU.PSW);
	InstructionStatus_t status;

	instr.opCode = OP_STR;
	instr.value = addr;

	// Direct Addressing (STR [Addr])
	setupCleanCPU();
	CPU.AC = data;
	instr.direction = ADDR_MODE_DIRECT;
	status = executeDataMovement(instr);
	readMemory(addr, &result);

	ASSERT_EQ(data, result);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Inmediate Addressing (STR Literal)
	setupCleanCPU();
	CPU.AC = data;
	instr.direction = ADDR_MODE_IMMEDIATE;
	status = executeDataMovement(instr);
	readMemory(addr, &result);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, status);

	// Indexed Addressing (STR [Base + AC])
	setupCleanCPU();
	CPU.AC = data;
	instr.direction = ADDR_MODE_INDEXED;
	status = executeDataMovement(instr);
	readMemory(addr, &result);

	ASSERT_EQ(data, result);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Invalid Case: Out of Bounds Write
	setupCleanCPU();
	CPU.AC = data;
	instr.direction = ADDR_MODE_DIRECT;
	instr.value = addr + RAM_SIZE;
	status = executeDataMovement(instr);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, status);
}

// Verify OP_LOAD in all 3 modes (Direct, Indexed and Immediate) and error handling.
UTEST(CPU_DataMov, LoadInstruction) {
	Instruction_t instr;
	InstructionStatus_t status;
	address addr = 400, offset = 10;

	instr.opCode = OP_LOAD;
	CPU.AC = addr;

	// Inmediate Addressing (LOAD Literal)
	setupCleanCPU();
	word data = intToWord(13, &CPU.PSW);
	instr.direction = ADDR_MODE_IMMEDIATE;
	instr.value = data;
	status = executeDataMovement(instr);

	ASSERT_EQ(data, CPU.AC);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
	ASSERT_EQ((unsigned)CC_POS, CPU.PSW.conditionCode);

	// Direct Addressing (LOAD [Addr])
	setupCleanCPU();
	data = intToWord(-13, &CPU.PSW);
	writeMemory(addr, data);
	instr.direction = ADDR_MODE_DIRECT;
	instr.value = addr;

	status = executeDataMovement(instr);

	ASSERT_EQ(data, CPU.AC);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
	ASSERT_EQ((unsigned)CC_NEG, CPU.PSW.conditionCode);

	// Indexed Addressing (LOAD [Base + AC])
	setupCleanCPU();
	data = intToWord(13, &CPU.PSW);
	CPU.AC = addr;
	writeMemory(addr + offset, data);
	instr.direction = ADDR_MODE_INDEXED;
	instr.value = offset;

	status = executeDataMovement(instr);

	ASSERT_EQ(data, CPU.AC);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
	ASSERT_EQ((unsigned)CC_POS, CPU.PSW.conditionCode);

	// Invalid Case: Out of Bounds Read
	setupCleanCPU();

	instr.direction = ADDR_MODE_DIRECT;
	instr.value = 99999;

	status = executeDataMovement(instr);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, status);
	ASSERT_EQ(0, wordToInt(CPU.AC));
}

// Verify LOADRX and STRRX instructions for auxiliary registers RX
UTEST(CPU_DataMov, LoadAndStoreRX) {
	setupCleanCPU();
	Instruction_t instr;
	InstructionStatus_t status;
	
	// Test STRRX (AC -> RX)
	CPU.AC = 123;
	CPU.RX = 0;
	instr.opCode = OP_STRRX;
	status = executeDataMovement(instr);
	ASSERT_EQ(123, CPU.RX);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Test LOADRX (RX -> AC)
	CPU.AC = 0;
	instr.opCode = OP_LOADRX;
	status = executeDataMovement(instr);
	ASSERT_EQ(123, CPU.AC);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
}

// Verify unconditional jump instruction (J)
UTEST(CPU_Branching, UnconditionalJump) {
	Instruction_t instr;
	InstructionStatus_t status;
	word initialPc = 100, jumpAddr = 400, offset = 5, expectedPc = 400;

	instr.opCode = OP_J;

	// Successful unconditional jump (J Addr) [Direct Mode]
	setupCleanCPU();
	CPU.PSW.pc = initialPc;
	instr.direction = ADDR_MODE_DIRECT;
	instr.value = jumpAddr;

	status = executeBranching(instr);

	ASSERT_EQ(expectedPc, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Successful unconditional jump (J Addr) [Immediate Mode]
	setupCleanCPU();
	CPU.PSW.pc = initialPc;
	instr.direction = ADDR_MODE_IMMEDIATE;
	instr.value = jumpAddr;

	status = executeBranching(instr);

	ASSERT_EQ(expectedPc, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Successful unconditional jump (J Addr) [Indexed Mode]
	setupCleanCPU();
	CPU.PSW.pc = initialPc;
	CPU.AC = intToWord(jumpAddr, &CPU.PSW);
	instr.direction = ADDR_MODE_INDEXED;
	instr.value = offset;

	status = executeBranching(instr);

	ASSERT_EQ(expectedPc + offset, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
}

// Verify conditional jumps success cases
UTEST(CPU_Branching, JumpEqualSuccess) {
	Instruction_t instr;
	InstructionStatus_t status;
	word initialPC = 200, addr = 550, offset = 10, stackAddr = 1400, value = 69;

	instr.opCode = OP_JMPE;

	// Successful jump when AC == M[SP] [Direct Mode]
	setupCleanCPU();
	writeMemory(stackAddr, value);
	CPU.PSW.pc = initialPC;
	CPU.SP = stackAddr;
	CPU.AC = value;

	instr.direction = ADDR_MODE_DIRECT;
	instr.value = addr;

	status = executeBranching(instr);

	ASSERT_EQ(addr, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Successful jump when AC == M[SP] [Immediate Mode]
	setupCleanCPU();
	writeMemory(stackAddr, value);
	CPU.PSW.pc = initialPC;
	CPU.SP = stackAddr;
	CPU.AC = value;

	instr.direction = ADDR_MODE_IMMEDIATE;
	instr.value = addr;

	status = executeBranching(instr);

	ASSERT_EQ(addr, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Successful jump when AC == M[SP] [Indexed Mode]
	setupCleanCPU();
	writeMemory(stackAddr, addr);
	CPU.PSW.pc = initialPC;
	CPU.SP = stackAddr;
	CPU.AC = addr;

	instr.direction = ADDR_MODE_INDEXED;
	instr.value = offset;

	status = executeBranching(instr);

	ASSERT_EQ(addr + offset, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Jump not taken when AC != M[SP] [Direct Mode]
	setupCleanCPU();
	writeMemory(stackAddr, value);
	CPU.PSW.pc = initialPC;
	CPU.SP = stackAddr;
	CPU.AC = value + 1; // Different value than M[SP]

	instr.direction = ADDR_MODE_DIRECT;
	instr.value = addr;

	status = executeBranching(instr);

	ASSERT_EQ(initialPC, CPU.PSW.pc); // It shouldn't have jumped
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Jump not taken when AC != M[SP] [Immediate Mode]
	setupCleanCPU();
	writeMemory(stackAddr, value);
	CPU.PSW.pc = initialPC;
	CPU.SP = stackAddr;
	CPU.AC = value + 1; // Different value than M[SP]

	instr.direction = ADDR_MODE_IMMEDIATE;
	instr.value = addr;

	status = executeBranching(instr);

	ASSERT_EQ(initialPC, CPU.PSW.pc); // It shouldn't have jumped
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Jump not taken when AC != M[SP] [Indexed Mode]
	setupCleanCPU();
	writeMemory(stackAddr, addr);
	CPU.PSW.pc = initialPC;
	CPU.SP = stackAddr;
	CPU.AC = addr + 1; // Different value than M[SP]

	instr.direction = ADDR_MODE_INDEXED;
	instr.value = offset;

	status = executeBranching(instr);

	ASSERT_EQ(initialPC, CPU.PSW.pc); // It shouldn't have jumped
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
}

// Test JMPNE instruction (Jump Not Equal)
UTEST(CPU_Branching, JumpNotEqual) {
	Instruction_t instr;
	InstructionStatus_t status;
	word initialPC = 200, addr = 550, offset = 10, stackAddr = 1400, value = 69;

	instr.opCode = OP_JMPNE;

	// Successful jump when AC != M[SP] [Direct Mode]
	setupCleanCPU();
	writeMemory(stackAddr, value);
	CPU.PSW.pc = initialPC;
	CPU.SP = stackAddr;
	CPU.AC = value + 1;

	instr.direction = ADDR_MODE_DIRECT;
	instr.value = addr;

	status = executeBranching(instr);

	ASSERT_EQ(addr, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Successful jump when AC != M[SP] [Immediate Mode]
	setupCleanCPU();
	writeMemory(stackAddr, value);
	CPU.PSW.pc = initialPC;
	CPU.SP = stackAddr;
	CPU.AC = value + 1;

	instr.direction = ADDR_MODE_IMMEDIATE;
	instr.value = addr;

	status = executeBranching(instr);

	ASSERT_EQ(addr, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Successful jump when AC != M[SP] [Indexed Mode]
	setupCleanCPU();
	writeMemory(stackAddr, addr + 1);
	CPU.PSW.pc = initialPC;
	CPU.SP = stackAddr;
	CPU.AC = addr;

	instr.direction = ADDR_MODE_INDEXED;
	instr.value = offset;

	status = executeBranching(instr);

	ASSERT_EQ(addr + offset, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
}

// Test JMPLT instruction (Jump Less Than)
UTEST(CPU_Branching, JumpLessThan) {
	Instruction_t instr;
	InstructionStatus_t status;
	word initialPC = 200, addr = 550, offset = 10, stackAddr = 1400, value = 69;

	instr.opCode = OP_JMPLT;

	// Successful jump when AC <= M[SP] [Direct Mode]
	setupCleanCPU();
	writeMemory(stackAddr, value);
	CPU.PSW.pc = initialPC;
	CPU.SP = stackAddr;
	CPU.AC = value - 1;

	instr.direction = ADDR_MODE_DIRECT;
	instr.value = addr;

	status = executeBranching(instr);

	ASSERT_EQ(addr, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Successful jump when AC <= M[SP] [Immediate Mode]
	setupCleanCPU();
	writeMemory(stackAddr, value);
	CPU.PSW.pc = initialPC;
	CPU.SP = stackAddr;
	CPU.AC = value - 1;

	instr.direction = ADDR_MODE_IMMEDIATE;
	instr.value = addr;

	status = executeBranching(instr);

	ASSERT_EQ(addr, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Successful jump when AC <= M[SP] [Indexed Mode]
	setupCleanCPU();
	writeMemory(stackAddr, addr + 1);
	CPU.SP = stackAddr;
	CPU.AC = addr;
	CPU.PSW.pc = initialPC;

	instr.direction = ADDR_MODE_INDEXED;
	instr.value = offset;

	status = executeBranching(instr);

	ASSERT_EQ(addr + offset, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
}

// Test JMPLGT instruction (Jump Greater Than)
UTEST(CPU_Branching, JumpGreaterThan) {
	Instruction_t instr;
	InstructionStatus_t status;
	word initialPC = 200, addr = 550, offset = 10, stackAddr = 1400, value = 69;

	instr.opCode = OP_JMPLGT;

	// Successful jump when AC >= M[SP] [Direct Mode]
	setupCleanCPU();
	writeMemory(stackAddr, value);
	CPU.SP = stackAddr;
	CPU.AC = value + 1;
	CPU.PSW.pc = initialPC;

	instr.direction = ADDR_MODE_DIRECT;
	instr.value = addr;

	status = executeBranching(instr);

	ASSERT_EQ(addr, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Successful jump when AC >= M[SP] [Immediate Mode]
	setupCleanCPU();
	writeMemory(stackAddr, value);
	CPU.SP = stackAddr;
	CPU.AC = value + 1;
	CPU.PSW.pc = initialPC;

	instr.direction = ADDR_MODE_IMMEDIATE;
	instr.value = addr;

	status = executeBranching(instr);

	ASSERT_EQ(addr, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Successful jump when AC >= M[SP] [Indexed Mode]
	setupCleanCPU();
	writeMemory(stackAddr, addr - 1);
	CPU.SP = stackAddr;
	CPU.AC = addr;
	CPU.PSW.pc = initialPC;

	instr.direction = ADDR_MODE_INDEXED;
	instr.value = offset;

	status = executeBranching(instr);

	ASSERT_EQ(addr + offset, CPU.PSW.pc);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
}

// Verify comparison instruction (COMP) logic and flags
UTEST(CPU_ALU, CompareInstruction) {
	Instruction_t instruction;

	// Iquality (5 == 5)
	setupCleanCPU();
	CPU.AC = intToWord(5, &CPU.PSW);
	CPU.IR = 8100005; // COMP Immediate 5
	instruction = decode();

	executeComparison(instruction);
	ASSERT_EQ(5, wordToInt(CPU.AC)); // 5 - 5
	ASSERT_EQ((unsigned)CC_ZERO, CPU.PSW.conditionCode);

	// Minor Than (10 < 20)
	setupCleanCPU();
	CPU.AC = intToWord(10, &CPU.PSW);
	CPU.IR = 8100020; // COMP Immediate 20
	instruction = decode();

	executeComparison(instruction); // 10 - 20
	ASSERT_EQ((unsigned)CC_NEG, CPU.PSW.conditionCode);

	// Greater Than (20 > 10)
	setupCleanCPU();
	CPU.AC = intToWord(20, &CPU.PSW);
	CPU.IR = 8100010; // COMP Immediate 10
	instruction = decode();

	executeComparison(instruction); // 20 - 10
	ASSERT_EQ((unsigned)CC_POS, CPU.PSW.conditionCode);

	// Algebraic Comparison Using Sign-Magnitude (-5 > -10)
	setupCleanCPU();
	CPU.AC = intToWord(-5, &CPU.PSW);
	writeMemory(300, intToWord(-10, &CPU.PSW));
	CPU.IR = 8000300; // COMP Direct RAM[300] = -10
	instruction = decode();

	executeComparison(instruction); // -5 - (-10)
	ASSERT_EQ((unsigned)CC_POS, CPU.PSW.conditionCode);

	// Overflow in Comparison
	// What happens if the distance between two numbers exceeds 7 digits?
	// AC = 6,000,000
	// Op = -5,000,000
	// Subtraction: 6M - (-5M) = 11,000,000 (Exceeds MAX_MAGNITUDE 9,999,999)
	setupCleanCPU();
	CPU.AC = intToWord(6000000, &CPU.PSW);
	writeMemory(300, intToWord(-5000000, &CPU.PSW));
	CPU.IR = 8000300; // COMP Direct RAM[300] = -5,000,000
	instruction = decode();

	executeComparison(instruction); // The CPU must detect overflow here
	ASSERT_EQ((unsigned)CC_OVERFLOW, CPU.PSW.conditionCode);
	ASSERT_EQ(6000000, wordToInt(CPU.AC)); // AC remains unchanged
}

// Verify handling of memory protection faults during data movement
UTEST(CPU_Safety, HandleMemoryProtectionFault) {
	setupCleanCPU();

	Instruction_t instr;
	instr.opCode = OP_STR;
	instr.direction = ADDR_MODE_DIRECT;
	instr.value = 500;

	mockMemoryFailProtection = true;
	InstructionStatus_t status = executeDataMovement(instr);
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, status);

	//ASSERT_TRUE(interruptPending);
	//ASSERT_EQ((unsigned)IC_INVALID_ADDR, pendingInterruptCode);

	mockMemoryFailProtection = false;
}

// Verify LOADSP and STRSP instructions for Stack Pointer (SP)
UTEST(CPU_Stack, LoadAndStoreSP) {
	setupCleanCPU();
	Instruction_t instr;
	InstructionStatus_t status;
	word initialSp = 1500;
	word value = 12;
	
	// Store initial SP value (AC -> SP)
	CPU.AC = value;
	CPU.SP = initialSp;
	instr.opCode = OP_STRSP;
	
	status = executeDataMovement(instr);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Load SP value into AC (SP -> AC)
	CPU.AC = 0;
	instr.opCode = OP_LOADSP;
	
	status = executeDataMovement(instr);
	ASSERT_EQ(value, CPU.AC);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
}

// Verify push operation onto the stack
UTEST(CPU_Stack, PushOperation) {
	setupCleanCPU();
	Instruction_t instr;
	InstructionStatus_t status;
	
	address stackTop = 410;
	word dataToPush = 999;
	word writtenValue = 0;

	CPU.RB = 300;
	CPU.RL = stackTop;
	CPU.RX = 310;
	CPU.SP = stackTop;
	CPU.AC = dataToPush;

	// Store the data at the current SP position (410)
	instr.opCode = OP_STRSP;
	status = executeDataMovement(instr);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);

	// Push (decrement) the Stack Pointer
	instr.opCode = OP_PSH;
	status = executeStackManipulation(instr); 
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
	
	// The SP should have moved down by 1 (410 -> 409)
	ASSERT_EQ(stackTop - 1, CPU.SP);
	
	// The data should remain at the OLD SP location (410)
	// We read directly from memory to verify persistence
	readMemory(stackTop, &writtenValue);
	ASSERT_EQ(dataToPush, writtenValue);
}

// Verify pop operation from the stack
UTEST(CPU_Stack, PopOperation) {
	setupCleanCPU();
	Instruction_t instr;
	InstructionStatus_t status;
	
	address stackTop = 410;
	address currentSP = 409; // Simulating we are one step deep
	word dataInStack = 888;
	
	// Manually put data where the stack was before
	writeMemory(stackTop, dataInStack);

	CPU.RB = 0;
	CPU.RL = 500;
	CPU.SP = currentSP;
	CPU.AC = 0;

	// Pop (increment) the Stack Pointer back to data location
	instr.opCode = OP_POP;
	status = executeStackManipulation(instr);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
	ASSERT_EQ(stackTop, CPU.SP);

	// Load the data from SP into AC
	instr.opCode = OP_LOADSP;
	status = executeDataMovement(instr);
	ASSERT_EQ((unsigned)INSTR_EXEC_SUCCESS, status);
	ASSERT_EQ(dataInStack, CPU.AC);
}

// Verify stack overflow protection (colliding with code segment)
UTEST(CPU_Stack, StackOverflowProtection) {
	setupCleanCPU();
	Instruction_t instr;
	
	address codeEnd = 310;
	
	CPU.RB = 300;
	CPU.RX = codeEnd;
	CPU.SP = codeEnd;
	CPU.AC = 123;

	// We try to PUSH. Logic: New SP would be (310 - 1) = 309.
	// 309 < RX (310), so it enters protected code area.
	instr.opCode = OP_PSH;

	InstructionStatus_t status = executeStackManipulation(instr);
	// Should fail due to overflow/protection violation
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, status);
	// SP should not have changed
	ASSERT_EQ(codeEnd, CPU.SP);
}

// Verify stack underflow protection (popping from an empty stack)
UTEST(CPU_Stack, StackUnderflowProtection) {
	setupCleanCPU();
	Instruction_t instr;
	
	address stackLimit = 500; // RL
	
	CPU.RL = stackLimit;
	CPU.SP = stackLimit; // Stack is empty (at top)
	
	// We try to POP. Logic: New SP would be (500 + 1) = 501.
	// 501 > RL (500), so it exceeds stack segment.
	instr.opCode = OP_POP;

	InstructionStatus_t status = executeStackManipulation(instr);
	// Should fail due to underflow
	ASSERT_EQ((unsigned)INSTR_EXEC_FAIL, status);
	// SP should not have changed
	ASSERT_EQ(stackLimit, CPU.SP);
}
