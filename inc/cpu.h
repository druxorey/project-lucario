/**
 * @file cpu.h
 * @brief Central Processing Unit (CPU) emulation and instruction cycle management.
 *
 * This file declares the core functions for the CPU simulation, including the
 * instruction cycle (Fetch-Decode-Execute), ALU operations, and internal
 * data format conversions (Sign-Magnitude <-> Two's Complement).
 *
 * @version 1.3
 */

#ifndef CPU_H
#define CPU_H

#include "../inc/definitions.h"

/**
 * @brief CPU Status Codes.
 *
 * Indicates the result of internal CPU operations to the main control loop,
 * signaling whether to continue execution or stop.
 */
typedef enum {
	CPU_OK   = 0,             /**< CPU operation successful, continue execution. */
	CPU_STOP = 1              /**< CPU has reached a stop condition or halt instruction. */
} CPUStatus_t;

/**
 * @brief Instruction Execution Status.
 *
 * Defines the outcome of individual instruction execution phases or 
 * specific instruction handlers.
 */
typedef enum {
	INSTR_EXEC_SUCCESS = 0,   /**< Instruction executed successfully. */
	INSTR_EXEC_FAIL    = 1    /**< Instruction execution failed due to error. */
} InstructionStatus_t;

/**
 * @brief Triggers a hardware interrupt or exception.
 *
 * Marks an interrupt as pending in the internal CPU state. The actual context 
 * switch and vector jump will be handled by the control unit at the end of 
 * the current instruction cycle.
 *
 * @param code The specific interrupt code to be raised.
 */
void raiseInterrupt(InterruptCode_t code);

/**
 * @brief Triggers a hardware interrupt with related data.
 *
 * Marks an interrupt as pending in the internal CPU state, associating it 
 * with a specific word of data. The actual context switch and vector jump 
 * will be handled by the control unit at the end of the current instruction cycle.
 *
 * @param code The specific interrupt code to be raised.
 * @param relatedWord Pointer to the word associated with the interrupt.
 */
void raiseInterruptRelated(InterruptCode_t code, word* relatedWord);

/**
 * @brief Checks and handles pending interrupts.
 * 
 * If interrupts are enabled and any are pending, this function processes them
 * according to their priority, updating the CPU state as necessary.
 * 
 * @return bool True if execution can continue, False if a fatal interrupt occurred.
 */
bool checkInterrupts(void);

/**
 * @brief Handles a specific interrupt.
 *
 * Executes the necessary context switch and state updates for the given 
 * interrupt code.
 *
 * @param code The interrupt code to handle.
 * @return bool True if execution can continue after handling, False otherwise.
 */
bool handleInterrupt(InterruptCode_t code);

/**
 * @brief Converts a Virtual Machine Word (Sign-Magnitude) to a C Integer.
 *
 * Translates the architecture's 8-digit decimal Sign-Magnitude format 
 * (where the most significant digit indicates sign) into a standard 
 * two's complement C integer for arithmetic processing.
 *
 * @param wordValue The 8-digit word to convert.
 * @return int The equivalent standard C integer.
 */
int wordToInt(word wordValue);

/**
 * @brief Converts a C Integer to a Virtual Machine Word (Sign-Magnitude).
 *
 * Translates a C integer back into the architecture's 8-digit Sign-Magnitude 
 * format. This function also updates the Processor Status Word (PSW) flags 
 * (Zero, Negative, Positive) and detects magnitude overflow.
 *
 * @param intValue The C integer to convert.
 * @param psw Pointer to the PSW structure to update status flags.
 * @return word The 8-digit Sign-Magnitude representation.
 */
word intToWord(int intValue, PSW_t *psw);

/**
 * @brief Retrieves the effective value of an instruction operand.
 *
 * Resolves the operand value by evaluating the addressing mode specified 
 * in the instruction (Immediate, Direct, Indirect, or Indexed).
 *
 * @param instruction The decoded instruction containing the operand and mode.
 * @param outValue Pointer where the resolved word value will be stored.
 * @return InstructionStatus_t Success if the operand was fetched, failure otherwise.
 */
InstructionStatus_t fetchOperand(Instruction_t instruction, word *outValue);

/**
 * @brief Calculates the target memory address for an instruction.
 *
 * Computes the final memory address for operations that require a destination 
 * or jump target, applying index register offsets if necessary.
 *
 * @param instruction The decoded instruction.
 * @return address The calculated physical or logical memory address.
 */
address calculateEffectiveAddress(Instruction_t instruction);

/**
 * @brief Performs an Arithmetic Logic Unit (ALU) operation.
 *
 * Executes arithmetic instructions (SUM, RES, MULT, DIVI). It updates the 
 * Accumulator (AC) and the condition codes in the PSW based on the result.
 *
 * @param instruction The decoded arithmetic instruction to execute.
 * @return InstructionStatus_t Result of the execution (Success/Failure).
 */
InstructionStatus_t executeArithmetic(Instruction_t instruction);

/**
 * @brief Handles Interrupt Enable/Disable instructions (HAB/DHAB).
 *
 * Updates the CPU's interrupt enable state based on the instruction.
 *
 * @param instruction The decoded interrupt control instruction.
 * @return InstructionStatus_t Result of the execution (Success/Failure).
 */
InstructionStatus_t executeInterruptsChange(Instruction_t instruction);

/**
 * @brief Handles Data Movement instructions.
 *
 * Manages instructions related to moving data between registers and memory, 
 * such as LOAD, STR, and MOV.
 *
 * @param instruction The decoded data movement instruction.
 * @return InstructionStatus_t Result of the execution (Success/Failure).
 */
InstructionStatus_t executeDataMovement(Instruction_t instruction);

/**
 * @brief Handles Flow Control instructions.
 *
 * Executes branching instructions (Jumps) by updating the Program Counter (PC) 
 * based on the instruction type and current PSW flags.
 *
 * @param instruction The decoded branching instruction.
 * @return InstructionStatus_t Result of the execution (Success/Failure).
 */
InstructionStatus_t executeBranching(Instruction_t instruction);

/**
 * @brief Handles the Comparison instruction (COMP).
 *
 * Performs a subtraction (AC - Operand) to update the PSW condition flags 
 * without modifying the value stored in the Accumulator.
 *
 * @param instruction The decoded comparison instruction.
 * @return InstructionStatus_t Result of the execution (Success/Failure).
 */
InstructionStatus_t executeComparison(Instruction_t instr);

/**
 * @brief Handles DMA-related instructions (SDMAP, SDMAC, SDMAON, etc.).
 * Updates DMA controller state and initiates transfers as needed.
 * Covers OpCodes: 28-33.
 */
InstructionStatus_t executeDMAInstruction(Instruction_t instr);

/**
 * @brief Handles Stack Manipulation instructions.
 *
 * Executes instructions that interact with the system stack, such as PUSH 
 * and POP, updating the Stack Pointer (SP) and memory accordingly.
 *
 * @param instruction The decoded stack instruction.
 * @return InstructionStatus_t Result of the execution (Success/Failure).
 */
InstructionStatus_t executeStackManipulation(Instruction_t instruction);

/**
 * @brief Handles System Call instructions (SVC).
 *
 * Triggers an interrupt to request operating system services.
 *
 * @return InstructionStatus_t Result of the execution (Success/Failure).
 */
InstructionStatus_t executeSystemCall(void);

/**
 * @brief Handles Return from System Call instructions (RETRN).
 *
 * Restores the CPU state after a system call, returning control to the 
 * instruction following the SVC.
 *
 * @return InstructionStatus_t Result of the execution (Success/Failure).
 */
InstructionStatus_t executeReturn(void);

/**
 * @brief Executes the Fetch phase of the instruction cycle.
 *
 * Transfers the address from the PC to the MAR, reads the instruction from 
 * memory into the MDR, updates the IR, and increments the PC.
 *
 * @return CPUStatus_t Result of the fetch operation (e.g., CPU_OK).
 */
CPUStatus_t fetch(void);

/**
 * @brief Executes the Decode phase of the instruction cycle.
 *
 * Extracts the OpCode, Addressing Mode, and Operand from the raw value 
 * currently held in the Instruction Register (IR).
 *
 * @return Instruction_t A structured representation of the decoded instruction.
 */
Instruction_t decode(void);

/**
 * @brief Executes the Execute phase of the instruction cycle.
 *
 * Dispatches the decoded instruction to the specific execution handler 
 * (ALU, Data Movement, etc.) based on its OpCode.
 *
 * @param instruction The decoded instruction to be executed.
 * @return CPUStatus_t The status of the CPU after execution (e.g., CPU_STOP).
 */
CPUStatus_t execute(Instruction_t instruction);

/**
 * @brief Main execution loop.
 *
 * Continuously runs the Fetch-Decode-Execute cycle until a halt condition 
 * is met or a fatal error occurs.
 *
 * @return int Exit code (0 for normal termination).
 */
int cpuRun(void);

/**
 * @brief Executes a single instruction cycle (Debug Mode).
 *
 * Performs one complete Fetch-Decode-Execute cycle and returns control.
 *
 * @return true If the CPU can continue to the next step.
 * @return false If the CPU has reached a halt state.
 */
bool cpuStep(void);

/**
 * @brief Performs a hard reset of the CPU.
 *
 * Resets all internal registers (AC, PC, IR, SP, etc.) to their initial 
 * states and restores default PSW flags.
 */
void cpuReset(void);

#endif // CPU_H
