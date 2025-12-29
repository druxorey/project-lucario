#include <stdbool.h>
#include <pthread.h>

#include "../inc/logger.h"
#include "../inc/cpu.h"
#include "../inc/memory.h"

static bool interruptPending = false;
static InterruptCode_t pendingInterruptCode = 0;

// Helper macro to update flags based on AC value (DRY principle)
static void updatePSWFlags(void) {
	if (CPU.AC == 0) CPU.PSW.conditionCode = CC_ZERO;
	else if (IS_NEGATIVE(CPU.AC)) CPU.PSW.conditionCode = CC_NEG;
	else CPU.PSW.conditionCode = CC_POS;
}


void raiseInterrupt(InterruptCode_t code) {
	loggerLogInterrupt(code);
	interruptPending = true;
	pendingInterruptCode = code;
}


int wordToInt(word w) {
	return (IS_NEGATIVE(w))? -((int)GET_MAGNITUDE(w)): (int)GET_MAGNITUDE(w);
}


word intToWord(int intValue, PSW_t* psw) {
	word wordValue = 0;
	
	int magnitude = (intValue >= 0) ? intValue : -intValue;
	bool isNegative = (intValue < 0);

	if (magnitude > MAX_MAGNITUDE) {
		psw->conditionCode = CC_OVERFLOW;
		magnitude = magnitude % (MAX_MAGNITUDE + 1);
		raiseInterrupt(IC_OVERFLOW);
	} else {
		if (intValue == 0) psw->conditionCode = CC_ZERO;
		else if (isNegative) psw->conditionCode = CC_NEG;
		else psw->conditionCode = CC_POS;
	}

	wordValue = magnitude;
	if (isNegative && magnitude != 0) { // Avoid "-0" (10000000) if we prefer to normalize
		wordValue |= SIGN_BIT;
	}

	return wordValue;
}


address calculateEffectiveAddress(Instruction_t instr) {
	if (instr.direction == DIR_INDEXED) {
		return instr.value + wordToInt(CPU.AC);
	}
	return instr.value;
}


InstructionStatus_t fetchOperand(Instruction_t instr, word *outValue) {
	MemoryStatus_t ret = MEM_SUCCESS;

	if (instr.direction == DIR_IMMEDIATE) {
		*outValue = intToWord(instr.value, &CPU.PSW);
	} else if (instr.direction == DIR_DIRECT || instr.direction == DIR_INDEXED) {
		address addr = calculateEffectiveAddress(instr);
		ret = readMemory(addr, outValue);
	} else {
		raiseInterrupt(IC_INVALID_INSTR);
		return INSTR_EXEC_FAIL;
	}

	if (ret != MEM_SUCCESS) {
		raiseInterrupt(IC_INVALID_ADDR);
		return INSTR_EXEC_FAIL;
	}

	return INSTR_EXEC_SUCCESS;
}


InstructionStatus_t executeArithmetic(OpCode_t op, word operandValue) {
	int accumulatorValue = wordToInt(CPU.AC);
	int operandIntValue = wordToInt(operandValue);
	switch (op) {
		case OP_SUM:
			CPU.AC = intToWord(accumulatorValue + operandIntValue, &CPU.PSW);
			break;
		case OP_RES:
			CPU.AC = intToWord(accumulatorValue - operandIntValue, &CPU.PSW);
			break;
		case OP_MULT:
			CPU.AC = intToWord(accumulatorValue * operandIntValue, &CPU.PSW);
			break;
		case OP_DIVI:
			if (operandIntValue == 0) {
				raiseInterrupt(IC_INVALID_INSTR);
				CPU.PSW.conditionCode = CC_OVERFLOW;
				return INSTR_EXEC_FAIL;
			}
			CPU.AC = intToWord(accumulatorValue / operandIntValue, &CPU.PSW);
			break;
		default:
			raiseInterrupt(IC_INVALID_INSTR);
			CPU.PSW.conditionCode = CC_OVERFLOW;
			return INSTR_EXEC_FAIL;
	}
	return INSTR_EXEC_SUCCESS;
}


InstructionStatus_t executeDataMovement(Instruction_t instr) {
	InstructionStatus_t status;
	MemoryStatus_t ret = MEM_SUCCESS;

	switch (instr.opCode) {
		case OP_STR: {
			if (instr.direction == DIR_IMMEDIATE) {
				raiseInterrupt(IC_INVALID_INSTR);
				return INSTR_EXEC_FAIL;
			}
			address effectiveAddr = calculateEffectiveAddress(instr);
			ret = writeMemory(effectiveAddr, CPU.AC);
			break;
		}
		case OP_LOAD: {
			word data;
			status = fetchOperand(instr, &data);
			CPU.AC = data;
			updatePSWFlags();
			break;
		}
		case OP_STRRX: {
			CPU.RX = CPU.AC;
			break;
		}
		case OP_STRRB: {
			CPU.RB = CPU.AC;
			break;
		}
		case OP_STRRL: {
			CPU.RL = CPU.AC;
			break;
		}
		case OP_STRSP: {
			CPU.SP = CPU.AC;
			break;
		}
		case OP_LOADRX: {
			CPU.AC = CPU.RX;
			updatePSWFlags();
			break;
		}
		case OP_LOADRB: {
			CPU.AC = CPU.RB;
			updatePSWFlags();
			break;
		}
		case OP_LOADRL: {
			CPU.AC = CPU.RL;
			updatePSWFlags();
			break;
		}
		case OP_LOADSP: {
			CPU.AC = CPU.SP;
			updatePSWFlags();
			break;
		}
		default:
			raiseInterrupt(IC_INVALID_INSTR);
			break;
	}

	if (ret != MEM_SUCCESS || status == INSTR_EXEC_FAIL) {
		raiseInterrupt(IC_INVALID_ADDR);
		return INSTR_EXEC_FAIL;
	}

	return INSTR_EXEC_SUCCESS;
}


InstructionStatus_t executeBranching(Instruction_t instruction) {
	if (instruction.opCode == OP_J) {
		CPU.PSW.pc = calculateEffectiveAddress(instruction) - 1;
		return INSTR_EXEC_SUCCESS;
	}

	word stackValue = 0;
	MemoryStatus_t ret = readMemory(CPU.SP, &stackValue);

	if (ret != MEM_SUCCESS) {
		raiseInterrupt(IC_INVALID_ADDR);
		return INSTR_EXEC_FAIL;
	}

	int acInt = wordToInt(CPU.AC);
	int stackInt = wordToInt(stackValue);
	bool shouldJump = false;

	switch (instruction.opCode) {
		case OP_JMPE:
			shouldJump = (acInt == stackInt);
			break;
		case OP_JMPNE:
			shouldJump = (acInt != stackInt);
			break;
		case OP_JMPLT:
			shouldJump = (acInt < stackInt);
			break;
		case OP_JMPLGT:
			shouldJump = (acInt > stackInt);
			break;
		default:
			raiseInterrupt(IC_INVALID_INSTR);
			return INSTR_EXEC_SUCCESS;
	}

	if (shouldJump) {
		CPU.PSW.pc = calculateEffectiveAddress(instruction) - 1;
	}

	return INSTR_EXEC_SUCCESS;
}


InstructionStatus_t executeComparison(word operandValue) {
	int acVal = wordToInt(CPU.AC);
	int opVal = wordToInt(operandValue);
	int result = acVal - opVal;
	int magnitude = (result >= 0) ? result : -result;

	if (magnitude > MAX_MAGNITUDE) {
		CPU.PSW.conditionCode = CC_OVERFLOW;
	} else if (result == 0) {
		CPU.PSW.conditionCode = CC_ZERO;
	} else if (result < 0) {
		CPU.PSW.conditionCode = CC_NEG;
	} else {
		CPU.PSW.conditionCode = CC_POS;
	}

	return INSTR_EXEC_SUCCESS;
}
  

CPUStatus_t fetch(void) {
    CPU.MAR = CPU.PSW.pc;
    if (readMemory(CPU.MAR, &CPU.MDR) != MEM_SUCCESS) {
        loggerLogInterrupt(IC_INVALID_ADDR);
        return CPU_STOP;
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
            return CPU_STOP;
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
