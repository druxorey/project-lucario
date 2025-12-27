#ifndef CPU_H
#define CPU_H

#include "../inc/definitions.h"

/**
 * @brief CPU Status Codes.
 * Used to indicate the result of CPU operations.
 */
typedef enum {
	CPU_OK = 0,          /**< CPU operation successful */
    CPU_HALT = 1,        /**< CPU has reached a halt instruction */
} CPUStatus_t;

/**
 * @brief Fetches the next instruction from memory into the IR.
 */
CPUStatus_t fetch(void);

/**
 * @brief Decodes the instruction in the IR into its components.
 */
Instruction_t decode(void);

/**
 * @brief Executes the given instruction.
 *
 * @param instruction The instruction to execute
 */
CPUStatus_t execute(Instruction_t instruction);

/**
 * @brief Runs the CPU until a halt condition is met.
 */
int cpuRun(void);

/**
 * @brief Executes a single instruction.
 */
bool cpuStep(void);

/**
 * @brief Resets the CPU to its initial state.
 */
void cpuReset(void);

#endif // CPU_H
