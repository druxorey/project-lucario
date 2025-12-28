#include "../lib/utest.h"
#include "../inc/cpu.h"
#include "../inc/memory.h"

CPU_t CPU;
word RAM[RAM_SIZE];
bool isDebugMode = false;

// Mock function for reading from memory
MemoryStatus_t readMemory(address addr, word* outData) {
    if (addr >= OS_RESERVED_SIZE && addr < RAM_SIZE) {
        *outData = RAM[addr];
        return MEM_SUCCESS;
    }
    return MEM_ERR_OUT_OF_BOUNDS;
}

// Mock function for writing in memory
MemoryStatus_t writeMemory(address addr, word value) {
    if (addr >= OS_RESERVED_SIZE && addr < RAM_SIZE) {
        RAM[addr] = value;
        return MEM_SUCCESS;
    }
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
    CPU.IR = 4100005;
    Instruction_t inst;
    inst = decode();
    ASSERT_EQ(inst.opCode, (unsigned)OP_LOAD);
    ASSERT_EQ(inst.direction, (unsigned)DIR_IMMEDIATE);
    ASSERT_EQ(inst.value, 5);
}

// Because execute is currently missing implementations, only the default case can be tested
UTEST(CPU, ExecuteStageDefault) {
    CPU.IR = 34100005; // Invalid OpCode (0-33 are valid)
    Instruction_t inst;
    inst = decode();
    ASSERT_EQ(inst.opCode, (unsigned)34);
    ASSERT_EQ(inst.direction, (unsigned)DIR_IMMEDIATE);
    ASSERT_EQ(inst.value, 5);
    CPUStatus_t status = execute(inst);
    ASSERT_EQ(status, CPU_HALT);
}