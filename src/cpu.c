#include <stdio.h>
#include <string.h>

#include "../inc/cpu.h"
#include "../inc/memory.h"
#include "../inc/logger.h"

CPUStatus_t fetch(void) {
    CPU.MAR = CPU.PSW.pc;
    if (readMemory(CPU.MAR, &CPU.MDR) != MEM_SUCCESS) {
        return CPU_HALT;    
    }
    CPU.IR = CPU.MDR;
    CPU.PSW.pc += 1;
    return CPU_OK;
}

Instruction_t decode(void) {
    Instruction_t inst;
    inst.opCode = GET_INSTRUCTION_OPCODE(CPU.IR);
    inst.direction = GET_INSTRUCTION_MODE(CPU.IR);
    inst.value = GET_INSTRUCTION_VALUE(CPU.IR);
    return inst;
}

CPUStatus_t execute(Instruction_t instruction) {
    switch (instruction.opCode) {
        case OP_SUM:
            // Implementation
            return CPU_OK;
        case OP_RES:
            // Implementation
            return CPU_OK;
        case OP_MULT:
            // Implementation
            return CPU_OK;
        case OP_DIVI:
            // Implementation
            return CPU_OK;
        case OP_LOAD:
            // Implementation
            return CPU_OK;
        case OP_STR:
            // Implementation
            return CPU_OK;
        case OP_LOADRX:
            // Implementation
            return CPU_OK;
        case OP_STRRX:
            // Implementation
            return CPU_OK;
        case OP_COMP:
            // Implementation
            return CPU_OK;
        case OP_JMPE:
            // Implementation
            return CPU_OK;
        case OP_JMPNE:
            // Implementation
            return CPU_OK;
        case OP_JMPLT:
            // Implementation
            return CPU_OK;
        case OP_JMPLGT:
            // Implementation
            return CPU_OK;
        case OP_SVC:
            // Implementation
            return CPU_OK;
        case OP_RETRN:
            // Implementation
            return CPU_OK;
        case OP_HAB:
            // Implementation
            return CPU_OK;
        case OP_DHAB:
            // Implementation
            return CPU_OK;
        case OP_TTI:
            // Implementation
            return CPU_OK;
        case OP_CHMOD:
            // Implementation
            return CPU_OK;
        case OP_LOADRB:
            // Implementation
            return CPU_OK;
        case OP_STRRB:
            // Implementation
            return CPU_OK;
        case OP_LOADRL:
            // Implementation
            return CPU_OK;
        case OP_STRRL:
            // Implementation
            return CPU_OK;
        case OP_LOADSP:
            // Implementation
            return CPU_OK;
        case OP_STRSP:
            // Implementation
            return CPU_OK;
        case OP_PSH:
            // Implementation
            return CPU_OK;
        case OP_POP:
            // Implementation
            return CPU_OK;
        case OP_J:
            // Implementation
            return CPU_OK;
        case OP_SDMAP:
            // Implementation
            return CPU_OK;
        case OP_SDMAC:
            // Implementation
            return CPU_OK;
        case OP_SDMAS:
            // Implementation
            return CPU_OK;
        case OP_SDMAIO:
            // Implementation
            return CPU_OK;
        case OP_SDMAM:
            // Implementation
            return CPU_OK;
        case OP_SDMAON:
            // Implementation
            return CPU_OK;
        default:
            loggerLogInterrupt(IC_INVALID_INSTR);
            return CPU_HALT;
    }
}

bool cpuStep(void) {
    if (fetch()) {
        return false;
    }
    Instruction_t inst = decode();
    if (execute(inst)) {
        return false;
    }
    return true;
}

int cpuRun(void) {
    while (true) {
        if (!cpuStep()) {
            break;
        }
    }
    return 0;
}

void cpuReset(void) {
    CPU = (CPU_t){0};
}
