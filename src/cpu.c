#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#include "../inc/logger.h"
#include "../inc/cpu.h"
#include "../inc/memory.h"

static uint16_t interruptBitmap = 0;
static word *interruptWord = NULL;

static void updatePSWFlags(void) {
	if (CPU.AC == 0) CPU.PSW.conditionCode = CC_ZERO;
	else if (IS_NEGATIVE(CPU.AC)) CPU.PSW.conditionCode = CC_NEG;
	else CPU.PSW.conditionCode = CC_POS;
}

static CPUStatus_t checkStatus(InstructionStatus_t status) {
	if (status == INSTR_EXEC_FAIL) return CPU_STOP;
	return CPU_OK;
}

static void internalPush(word value) {
	CPU.SP -= 1;
	
	MemoryStatus_t status = writeMemory(CPU.SP, value);
	
	if (status != MEM_SUCCESS) {
		#ifdef DEBUG
		printf("\x1b[31m[CRITICAL]: Failed to push context (SP=%d). Error: %d\x1b[0m\n", CPU.SP, status);
		#endif
	}
}

static word internalPop(void) {
	word value = 0;

	MemoryStatus_t status = readMemory(CPU.SP, &value);
	
	if (status != MEM_SUCCESS) {
		#ifdef DEBUG
		printf("\x1b[31m[CRITICAL]: Failed to pop context (SP=%d). Error: %d\x1b[0m\n", CPU.SP, status);
		#endif
	}

	CPU.SP += 1;
	return value;
}


static void saveContext(void) {
	internalPush(CPU.RX);
	internalPush(CPU.RL);
	internalPush(CPU.RB);
	internalPush((word)CPU.PSW.mode);
	internalPush((word)CPU.PSW.conditionCode);
	internalPush(CPU.PSW.pc);
	internalPush(CPU.AC);
}

static void restoreContext(InterruptCode_t codeHandled) {
	word savedAC = internalPop();
	
	if (codeHandled != IC_OVERFLOW && codeHandled != IC_UNDERFLOW) {
		CPU.AC = savedAC;
	}

	CPU.PSW.pc             = internalPop();
	CPU.PSW.conditionCode  = (ConditionCode_t)internalPop();
	CPU.PSW.mode           = (OpMode_t)internalPop();
	CPU.RB                 = internalPop();
	CPU.RL                 = internalPop();
	CPU.RX                 = internalPop();
}

void raiseInterrupt(InterruptCode_t code) {
	loggerLogInterrupt(code);
	interruptBitmap |= (1 << code);
}


void raiseInterruptRelated(InterruptCode_t code, word* relatedWord) {
	loggerLogInterrupt(code);
	interruptBitmap |= (1 << code); 
	interruptWord = relatedWord;
}


bool checkInterrupts(void) {
	if (interruptBitmap == 0 || CPU.PSW.interruptEnable == ITR_DISABLED) return true;

	InterruptCode_t codeToHandle = -1;
	bool status = false;

	// Instruction ordered by priority
	if (interruptBitmap & (1 << IC_INVALID_INSTR))         codeToHandle = IC_INVALID_INSTR;
	else if (interruptBitmap & (1 << IC_INVALID_ADDR))     codeToHandle = IC_INVALID_ADDR;
	else if (interruptBitmap & (1 << IC_OVERFLOW))         codeToHandle = IC_OVERFLOW;
	else if (interruptBitmap & (1 << IC_UNDERFLOW))        codeToHandle = IC_UNDERFLOW;
	else if (interruptBitmap & (1 << IC_TIMER))            codeToHandle = IC_TIMER;
	else if (interruptBitmap & (1 << IC_IO_DONE))          codeToHandle = IC_IO_DONE;
	else if (interruptBitmap & (1 << IC_SYSCALL))          codeToHandle = IC_SYSCALL;
	else if (interruptBitmap & (1 << IC_INVALID_SYSCALL))  codeToHandle = IC_INVALID_SYSCALL;
	else                                                   codeToHandle = IC_INVALID_INT_CODE;

	CPU.PSW.interruptEnable = ITR_DISABLED;
	if (codeToHandle != -1) {
		saveContext();
		status = handleInterrupt(codeToHandle);
		interruptBitmap &= ~(1 << codeToHandle);
		if (status == true) {
			restoreContext(codeToHandle);
		}
	}
	CPU.PSW.interruptEnable = ITR_ENABLED;

	return status;
}


bool handleInterrupt(InterruptCode_t code) {
	switch (code) {
		case IC_INVALID_INSTR:
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: Invalid Instruction. Aborting execution.\x1b[0m\n");
			#endif
			return false;
		case IC_INVALID_ADDR:
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: Invalid Address. Aborting execution.\x1b[0m\n");
			#endif
			return false;
		case IC_OVERFLOW:
			int intValue = wordToInt(*interruptWord);
			*interruptWord = intToWord(intValue % (MAX_MAGNITUDE + 1), &CPU.PSW);
			interruptWord = NULL;
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: Overflow detected. Adjusting value - Previous: %d, New: %d\x1b[0m\n", intValue, wordToInt(*interruptWord));
			#endif
			return true;
		case IC_UNDERFLOW:
			*interruptWord = 0;
			interruptWord = NULL;
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: Underflow detected. Adjusting value to 0\x1b[0m\n");
			#endif
			return true;
		case IC_TIMER: // Simulate program pause
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: Timer interrupt triggered. Pausing program.\x1b[0m\n");
			#endif
			return true;
		case IC_IO_DONE: // Simulate program resume after I/O completion
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: I/O operation completed. Resuming program.\x1b[0m\n");
			#endif
			return true;
		case IC_SYSCALL:
			int syscallCode = wordToInt(CPU.AC);
			if (syscallCode == 0) {
				printf("SYSTEM CALL [0]: Program requested termination (EXIT).\n");
				return false;
			} else {
				// Any other code is considered a service request that does NOT stop the CPU
				printf("SYSTEM CALL [%d]: Service handled (Simulation).\n", syscallCode);
				// In the future an interrupt will be generated here, but the CPU does NOT stop,
				// it simply continues to the next instruction after being serviced.
				return true;
			}
		case IC_INVALID_SYSCALL:
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: Invalid System Call. Continuing execution.\x1b[0m\n");
			#endif
			return true;
		case IC_INVALID_INT_CODE:
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: Invalid Interrupt Code. Continuing execution.\x1b[0m\n");
			#endif
			return true;
	}
	return false;
}


int wordToInt(word wordValue) {
	return (IS_NEGATIVE(wordValue))? -((int)GET_MAGNITUDE(wordValue)): (int)GET_MAGNITUDE(wordValue);
}


word intToWord(int intValue, PSW_t* psw) {
	word wordValue = 0;
	
	int magnitude = (intValue >= 0) ? intValue : -intValue;
	bool isNegative = (intValue < 0);

	if (magnitude > MAX_MAGNITUDE) {
		psw->conditionCode = CC_OVERFLOW;
		magnitude = magnitude % (MAX_MAGNITUDE + 1);
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


address calculateEffectiveAddress(Instruction_t instruction) {
	if (instruction.direction == ADDR_MODE_INDEXED) {
		return instruction.value + wordToInt(CPU.AC);
	}
	return instruction.value;
}


InstructionStatus_t fetchOperand(Instruction_t instruction, word *outValue) {
	MemoryStatus_t ret = MEM_SUCCESS;

	if (instruction.direction == ADDR_MODE_IMMEDIATE) {
		*outValue = intToWord(instruction.value, &CPU.PSW);
	} else if (instruction.direction == ADDR_MODE_DIRECT || instruction.direction == ADDR_MODE_INDEXED) {
		address addr = calculateEffectiveAddress(instruction);
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


InstructionStatus_t executeArithmetic(Instruction_t instruction) {
	OpCode_t op = instruction.opCode;
	word operandValue;
	if (fetchOperand(instruction, &operandValue)) return INSTR_EXEC_FAIL;
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
			return INSTR_EXEC_FAIL;
	}
	if (CPU.PSW.conditionCode == CC_OVERFLOW) {
		raiseInterruptRelated(IC_OVERFLOW, &CPU.AC);
	}
	return INSTR_EXEC_SUCCESS;
}


InstructionStatus_t executeInterruptsChange(Instruction_t instruction) {
	if (instruction.opCode == OP_HAB) {
		CPU.PSW.interruptEnable = ITR_ENABLED;
	} else if (instruction.opCode == OP_DHAB) {
		CPU.PSW.interruptEnable = ITR_DISABLED;
	} else {
		raiseInterrupt(IC_INVALID_INSTR);
		return INSTR_EXEC_FAIL;
	}
	return INSTR_EXEC_SUCCESS;
}


InstructionStatus_t executeDataMovement(Instruction_t instruction) {
	InstructionStatus_t status = INSTR_EXEC_SUCCESS;
	MemoryStatus_t ret = MEM_SUCCESS;

	switch (instruction.opCode) {
		case OP_STR: {
			if (instruction.direction == ADDR_MODE_IMMEDIATE) {
				raiseInterrupt(IC_INVALID_INSTR);
				return INSTR_EXEC_FAIL;
			}
			address effectiveAddr = calculateEffectiveAddress(instruction);
			ret = writeMemory(effectiveAddr, CPU.AC);
			break;
		}
		case OP_LOAD: {
			word data;
			status = fetchOperand(instruction, &data);
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
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: STRSP: Writing AC=%08d to SP-RB=%d\x1b[0m\n", CPU.AC, CPU.SP - CPU.RB);
			#endif
			ret = writeMemory(CPU.SP, CPU.AC);
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
			#ifdef DEBUG
			printf("\x1b[36m[DEBUG]: LOADSP: Reading from SP-RB=%d to AC\x1b[0m\n", CPU.SP - CPU.RB);
			#endif
			readMemory(CPU.SP, &CPU.AC);
			updatePSWFlags();
			break;
		}
		default:
			raiseInterrupt(IC_INVALID_INSTR);
			break;
	}

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Data Movement instruction executed: OpCode=%d\x1b[0m\n", instruction.opCode);
	printf("\x1b[36m[DEBUG]: Return status: [%d], Status: [%d]\x1b[0m\n", ret, status);
	#endif

	if (ret != MEM_SUCCESS || status == INSTR_EXEC_FAIL) {
		raiseInterrupt(IC_INVALID_ADDR);
		return INSTR_EXEC_FAIL;
	}

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Data Movement executed. AC=%08d\x1b[0m\n", CPU.AC);
	#endif

	return INSTR_EXEC_SUCCESS;
}


InstructionStatus_t executeBranching(Instruction_t instruction) {
	if (instruction.opCode == OP_J) {
		CPU.PSW.pc = calculateEffectiveAddress(instruction);
		return INSTR_EXEC_SUCCESS;
	}

	word stackValue = 0;
	MemoryStatus_t ret = readMemory(CPU.SP, &stackValue);

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Branching instruction executed: stackValue=%d\x1b[0m\n", stackValue);
	#endif

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
		CPU.PSW.pc = calculateEffectiveAddress(instruction);
		#ifdef DEBUG
		printf("\x1b[36m[DEBUG]: Branch taken to address %03d\x1b[0m\n", CPU.PSW.pc);
		#endif
	}

	return INSTR_EXEC_SUCCESS;
}


InstructionStatus_t executeComparison(Instruction_t instruction) {
	word operandValue;
	if (fetchOperand(instruction, &operandValue)) return INSTR_EXEC_FAIL;
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


InstructionStatus_t executeDMAInstruction(Instruction_t instruction) {
	word data;
	InstructionStatus_t status = fetchOperand(instruction, &data);
	int intData = wordToInt(data);

	switch (instruction.opCode) {
		case OP_SDMAP: {
			if(intData < 0 || intData >= DISK_TRACKS) {
				raiseInterrupt(IC_INVALID_INSTR);
				return INSTR_EXEC_FAIL;
			}
			DMA.track = intData;
			break;
		}
		case OP_SDMAC: {
			if(intData < 0 || intData >= DISK_CYLINDERS) {
				raiseInterrupt(IC_INVALID_INSTR);
				return INSTR_EXEC_FAIL;
			}
			DMA.cylinder = intData;
			break;
		}
		case OP_SDMAS: {
			if(intData < 0 || intData >= DISK_SECTORS) {
				raiseInterrupt(IC_INVALID_INSTR);
				return INSTR_EXEC_FAIL;
			}
			DMA.sector = intData;
			break;
		}
		case OP_SDMAIO: {
			if(intData != 0 && intData != 1) {
				raiseInterrupt(IC_INVALID_INSTR);
				return INSTR_EXEC_FAIL;
			}
			DMA.ioDirection = intData;
			break;
		}
		case OP_SDMAM: {
            int physicalAddr;

            if (CPU.PSW.mode == MODE_KERNEL) {
                physicalAddr = intData;
            } else {
                physicalAddr = CPU.RB + intData;
                if (physicalAddr > CPU.RL) {
                    raiseInterrupt(IC_INVALID_ADDR);
                    return INSTR_EXEC_FAIL;
                }
            }

            if(physicalAddr < 0 || physicalAddr >= RAM_SIZE) {
                raiseInterrupt(IC_INVALID_INSTR);
                return INSTR_EXEC_FAIL;
            }

            DMA.memAddr = physicalAddr;
            break;
        }
		case OP_SDMAON: {
			pthread_mutex_lock(&BUS_LOCK);
			DMA.pending = true;
			pthread_cond_signal(&DMA_COND);
			pthread_mutex_unlock(&BUS_LOCK);
			while (DMA.pending) {
				usleep(1000); // Simulates blocked program state until DMA completes
			}
			break;
		}
		default:
			raiseInterrupt(IC_INVALID_INSTR);
			return INSTR_EXEC_FAIL;
	}

	if (status == INSTR_EXEC_FAIL) {
		raiseInterrupt(IC_INVALID_ADDR);
		return INSTR_EXEC_FAIL;
	}

	return INSTR_EXEC_SUCCESS;
}

  
InstructionStatus_t executeStackManipulation(Instruction_t instruction) {

	if (instruction.opCode == OP_PSH) {
		if (CPU.SP - 1 < CPU.RX) {
			raiseInterrupt(IC_INVALID_ADDR);
			return INSTR_EXEC_FAIL;
		}
		CPU.SP -= 1;
	} else if (instruction.opCode == OP_POP) {
		if (CPU.SP >= CPU.RL) {
			raiseInterrupt(IC_INVALID_ADDR);
			return INSTR_EXEC_FAIL;
		}
		CPU.SP += 1;
		updatePSWFlags();
	} else {
		raiseInterrupt(IC_INVALID_INSTR);
		return INSTR_EXEC_FAIL;
	}

	return  INSTR_EXEC_SUCCESS;
}


InstructionStatus_t executeSystemCall(void) {
	raiseInterrupt(IC_SYSCALL);
	return INSTR_EXEC_SUCCESS;
}


InstructionStatus_t executeReturn(void) {
	word returnAddress;
	MemoryStatus_t ret = readMemory(CPU.SP, &returnAddress);
	if (ret != MEM_SUCCESS) {
		raiseInterrupt(IC_INVALID_ADDR);
		return INSTR_EXEC_FAIL;
	}
	CPU.PSW.pc = wordToInt(returnAddress);
	CPU.SP += 1;
	return INSTR_EXEC_SUCCESS;
}


CPUStatus_t fetch(void) {
	CPU.MAR = CPU.PSW.pc;

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Fetching instruction from address %03d\x1b[0m\n", CPU.MAR);
	#endif

	if (readMemory(CPU.MAR, &CPU.MDR) != MEM_SUCCESS) {
		loggerLogInterrupt(IC_INVALID_ADDR);
		return CPU_STOP;
	}
	CPU.IR = CPU.MDR;
	CPU.PSW.pc += 1;

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Fetched instruction %08d from address %03d\x1b[0m\n", CPU.IR, CPU.MAR);
	printf("\x1b[36m[DEBUG]: Updated PC to %03d\x1b[0m\n", CPU.PSW.pc);
	#endif

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
	InstructionStatus_t status = INSTR_EXEC_SUCCESS;
	switch (instruction.opCode) {
		case OP_SUM:
		case OP_RES:
		case OP_MULT:
		case OP_DIVI:
			status = executeArithmetic(instruction);
			return checkStatus(status);
		case OP_LOAD:
		case OP_STR:
		case OP_LOADRX:
		case OP_STRRX:
			status = executeDataMovement(instruction);
			return checkStatus(status);
		case OP_COMP:
			status = executeComparison(instruction);
			return checkStatus(status);
		case OP_JMPE:
		case OP_JMPNE:
		case OP_JMPLT:
		case OP_JMPLGT:
			status = executeBranching(instruction);
			return checkStatus(status);
		case OP_SVC:
			status = executeSystemCall();
			return checkStatus(status);
		case OP_RETRN:
			status = executeReturn();
			return checkStatus(status);
		case OP_HAB:
		case OP_DHAB:
			status = executeInterruptsChange(instruction);
			return checkStatus(status);
		case OP_TTI:
			// Implementation
			return checkStatus(status);
		case OP_CHMOD:
			if (instruction.value == 0) {
				CPU.PSW.mode = MODE_USER;
			} else if (instruction.value == 1) {
				CPU.PSW.mode = MODE_KERNEL;
			} else {
				raiseInterrupt(IC_INVALID_INSTR);
				return CPU_STOP;
			}
			return CPU_OK;
		case OP_LOADRB:
		case OP_STRRB:
		case OP_LOADRL:
		case OP_STRRL:
		case OP_LOADSP:
		case OP_STRSP:
			status = executeDataMovement(instruction);
			return checkStatus(status);
		case OP_PSH:
		case OP_POP:
			status = executeStackManipulation(instruction);
			return checkStatus(status);
		case OP_J:
			status = executeBranching(instruction);
			return checkStatus(status);
		case OP_SDMAP:
		case OP_SDMAC:
		case OP_SDMAS:
		case OP_SDMAIO:
		case OP_SDMAM:
		case OP_SDMAON:
			status = executeDMAInstruction(instruction);
			return checkStatus(status);
		default:
			raiseInterrupt(IC_INVALID_INSTR);
			return CPU_STOP;
	}
}


bool cpuStep(void) {
	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Starting CPU step at PC:%03d\x1b[0m\n", CPU.PSW.pc);
	#endif

	if (fetch()) return false;

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Fetched instruction %08d into IR\x1b[0m\n", CPU.IR);
	#endif
	
	Instruction_t inst = decode();

	#ifdef DEBUG
	printf("\x1b[36m[DEBUG]: Decoded instruction - Opcode: %02d, Mode: %01d, Value: %04d\x1b[0m\n", inst.opCode, inst.direction, inst.value);
	#endif

	if (execute(inst)) {
		#ifdef DEBUG
		printf("\x1b[36m[DEBUG]: Fatal error ocurred during execution stage. Executing interruption handler\x1b[0m\n");
		#endif
	} else {
		#ifdef DEBUG
		printf("\x1b[36m[DEBUG]: Completed CPU step. PC is now at %03d\x1b[0m\n", CPU.PSW.pc);
		#endif
	}
  
	CPU.cyclesCounter++;
	if ((CPU.cyclesCounter >= CPU.timerLimit) && (CPU.timerLimit > 0)) {
		CPU.cyclesCounter = 0;
		raiseInterrupt(IC_TIMER);
		// Call the interrupt handler
	}

	return checkInterrupts();
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
	interruptBitmap = 0;
	interruptWord = NULL;
}
