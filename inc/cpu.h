/**
 * @file cpu.h
 * @brief Central Processing Unit (CPU) emulation and instruction cycle management.
 *
 * This file declares the core functions for the CPU simulation, including the
 * instruction cycle (Fetch-Decode-Execute), ALU operations, and internal
 * data format conversions (Sign-Magnitude <-> Two's Complement).
 *
 * @version 1.0
 */

#ifndef CPU_H
#define CPU_H

#include "../inc/definitions.h"
#include "memory.h"

/**
 * @brief CPU Status Codes.
 * Used to indicate the result of internal CPU operations to the control loop.
 */
typedef enum {
	CPU_OK = 0,          /**< CPU operation successful */
    CPU_STOP = 1,        /**< CPU has reached a stop condition */
} CPUStatus_t;

typedef enum {
	INSTR_EXEC_SUCCESS = 0,
	INSTR_EXEC_FAIL = 1
} InstructionStatus_t;

/**
 * @brief Triggers a hardware interrupt or exception.
 *
 * Marks an interrupt as pending and logs the event. The CPU will handle
 * the context switch at the end of the current instruction cycle.
 *
 * @param code The specific interrupt code (e.g., IC_OVERFLOW, IC_TIMER).
 */
void raiseInterrupt(InterruptCode_t code);

/**
 * @brief Converts a Virtual Machine Word (Sign-Magnitude) to a C Integer.
 *
 * The architecture uses 8-digit decimal Sign-Magnitude (1xxxxxxx = Negative).
 * C uses Two's Complement. This helper performs the necessary translation
 * for arithmetic operations.
 *
 * @param w The 8-digit word from memory or register.
 * @return int The standard C integer representation.
 */
int wordToInt(word w);

/**
 * @brief Converts a C Integer to a Virtual Machine Word (Sign-Magnitude).
 *
 * Translates the result of a C operation back to the architecture's format.
 * This function is also responsible for:
 * 1. Setting PSW flags (Zero, Negative, Positive).
 * 2. Detecting Overflow (Magnitudes > 7 digits).
 *
 * @param intValue The standard C integer to convert.
 * @param[out] psw Pointer to the PSW to update status flags.
 * @return word The 8-digit Sign-Magnitude representation (truncated on overflow).
 */
word intToWord(int intValue, PSW_t *psw);

/**
 * @brief Retrieves the effective value of an instruction operand.
 *
 * Resolves the operand of the decoded instruction according to its addressing mode
 *
 * @param instr Decoded instruction from which the operand value will be obtained.
 * @return word Effective operand value ready to be used by the ALU or control unit.
 */
InstructionStatus_t fetchOperand(Instruction_t instr, word *outValue);

/**
 * @brief Calculates the target memory address for instructions.
 * Used for STR (Store) and Jump instructions.
 * @param instr The decoded instruction.
 * @return address The physical/logical address (Value + Index if needed).
 */
address calculateEffectiveAddress(Instruction_t instr);

/**
 * @brief Performs an Arithmetic Logic Unit (ALU) operation.
 *
 * Handles logic for SUM, RES, MULT, and DIVI. It updates the Accumulator (AC)
 * and the Condition Codes in the PSW.
 *
 * @param op The arithmetic operation code (OP_SUM, OP_RES, etc.).
 * @param operandValue The resolved integer value of the operand (not the address).
 */
InstructionStatus_t executeArithmetic(Instruction_t instruction);

/**
 * @brief Handles Data Movement instructions (LOAD, STR, MOV registers).
 * Covers OpCodes: 04-07, 19-26.
 */
InstructionStatus_t executeDataMovement(Instruction_t instr);

/**
 * @brief Handles Flow Control instructions (Jumps).
 * Covers OpCodes: 09-12, 27.
 */
InstructionStatus_t executeBranching(Instruction_t instr);

/**
 * @brief Handles Comparison instruction (COMP).
 * Performs a subtraction (AC - Operand) to update flags, but discards result.
 * Covers OpCode: 08.
 */
InstructionStatus_t executeComparison(Instruction_t instruction);

/**
 * @brief Executes the Fetch phase of the instruction cycle.
 *
 * Moves the PC value to MAR, reads from Memory into MDR, and updates the IR.
 * This function also increments the PC.
 *
 * @return CPUStatus_t Result of the memory access (usually CPU_OK).
 */
CPUStatus_t fetch(void);

/**
 * @brief Executes the Decode phase of the instruction cycle.
 *
 * Parses the raw 8-digit value in the Instruction Register (IR) to extract
 * the OpCode, Addressing Mode, and Operand Value.
 *
 * @return Instruction_t A structured representation of the decoded instruction.
 */
Instruction_t decode(void);

/**
 * @brief Executes the Execute phase of the instruction cycle.
 *
 * Dispatches the decoded instruction to the appropriate handler (ALU,
 * Data Movement, Control Flow, etc.) based on the OpCode.
 *
 * @param instruction The decoded instruction structure to execute.
 * @return CPUStatus_t The result of the execution (e.g., CPU_HALT if EXIT).
 */
CPUStatus_t execute(Instruction_t instruction);

/**
 * @brief Main execution loop (Normal Mode).
 *
 * Runs the Fetch-Decode-Execute cycle continuously until a HALT condition
 * or an unrecoverable error occurs.
 *
 * @return int Exit code (0 for success).
 */
int cpuRun(void);

/**
 * @brief Executes a single instruction step (Debug Mode).
 *
 * Performs exactly one F-D-E cycle and returns control to the caller.
 *
 * @return true If the CPU can continue executing.
 * @return false If the CPU has halted.
 */
bool cpuStep(void);

/**
 * @brief Hard Resets the CPU.
 *
 * Clears all registers (AC, PC, IR, etc.) to zero and sets default
 * startup flags (Kernel Mode, Interrupts Disabled).
 */
void cpuReset(void);

#endif // CPU_H
