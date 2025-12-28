#include <stdbool.h>
#include <pthread.h>

#include "../inc/logger.h"
#include "../inc/cpu.h"
#include "../inc/memory.h"

static bool interruptPending = false;
static InterruptCode_t pendingInterruptCode = 0;

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


word getOperandValue(Instruction_t instruction) {
	word returnValue = 0;
	MemoryStatus_t ret;
	switch (instruction.direction) {
		case DIR_IMMEDIATE: {
			returnValue = intToWord(instruction.value, &CPU.PSW);
			break;
		}
		case DIR_DIRECT: {
			address addr = instruction.value;
			ret = readMemory(addr, &returnValue);
			break;
		}
		case DIR_INDEXED: {
			address addr = instruction.value + wordToInt(CPU.RX);
			ret = readMemory(addr, &returnValue);
			break;
		}
		default:
			raiseInterrupt(IC_INVALID_INSTR);
			break;
	}

	if (ret != MEM_SUCCESS) {
		raiseInterrupt(IC_INVALID_ADDR);
	}

	return returnValue;
}


void executeArithmetic(OpCode_t op, word operandValue) {
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
				raiseInterrupt(IC_INVALID_INSTR); // We have to decide which interrupt to raise here
				CPU.PSW.conditionCode = CC_OVERFLOW; // We have to decide which condition code to set here
				return;
			}
			CPU.AC = intToWord(accumulatorValue / operandIntValue, &CPU.PSW);
			break;
		default:
			raiseInterrupt(IC_INVALID_INSTR);
			CPU.PSW.conditionCode = CC_OVERFLOW; // We have to decide which condition code to set here
			break;
	}
	return;
}
