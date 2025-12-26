/**
 * @file definitions.h
 * @brief Global definitions, constants, and structures for the Virtual Hardware.
 *
 * Contains all shared data structures between the CPU, Memory, DMA,
 * and other subsystems, based on the 8-digit decimal architecture.
 *
 * @version 1.1
 */

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/** @brief Main memory size in words. */
#define RAM_SIZE 2000
/** @brief Memory space reserved for the Operating System. */
#define OS_RESERVED_SIZE 300
/** @brief Virtual Hard Disk Geometry. */
#define DISK_TRACKS 10
#define DISK_CYLINDERS 10
#define DISK_SECTORS 100
/** @brief Logical sector size. It represents 9 logical characters. */
#define SECTOR_SIZE 9
/** @brief Maximum allowed magnitude (7 digits). */
#define MAX_MAGNITUDE 9999999
#define MIN_MAGNITUDE 0

/** @brief Represents an 8-decimal digit machine word. (SMMMMMMM S=Sign, M=Magnitude). */
typedef int word;
/** @brief Represents a memory address (index 0-1999). */
typedef int address;

/**
 * @brief PSW Condition Codes.
 * Indicates the result of the most recent arithmetic operation.
 */
typedef enum {
	CC_ZERO     = 0,    /**< Result = 0 */
	CC_NEG      = 1,    /**< Result < 0 */
	CC_POS      = 2,    /**< Result > 0 */
	CC_OVERFLOW = 3     /**< Magnitude overflow */
} ConditionCode_t;

/**
 * @brief Processor Operation Modes.
 * Defines the current privilege level of the CPU.
 */
typedef enum {
	MODE_USER   = 0,    /**< User Mode (Restricted) */
	MODE_KERNEL = 1     /**< Kernel Mode (Full Access) */
} OpMode_t;

/**
 * @brief Interrupt Enable State in PSW.
 * Indicates whether interrupts are currently enabled or disabled.
 */
typedef enum {
	ITR_DISABLED = 0,   /**< Interrupts disabled */
	ITR_ENABLED  = 1,   /**< Interrupts enabled */
} InterruptionState_t;

/**
 * @brief Instruction Addressing Modes.
 * Defines how the operand of an instruction is interpreted.
 */
typedef enum {
	DIR_DIRECT    = 0,  /**< Last 5 digits are a memory address */
	DIR_IMMEDIATE = 1,  /**< Last 5 digits are the data value */
	DIR_INDEXED   = 2,  /**< Address calculated (Value + Index) */
} DirectionMode_t;

/**
 * @brief Instruction Set Architecture (ISA).
 * Defines all supported operation codes (OpCodes).
 */
typedef enum {
	OP_SUM      = 0,    /**< Add data to AC */
	OP_RES      = 1,    /**< Subtract data from AC */
	OP_MULT     = 2,    /**< Multiply data with AC */
	OP_DIVI     = 3,    /**< Divide AC by data */
	OP_LOAD     = 4,    /**< Load memory content into AC */
	OP_STR      = 5,    /**< Store AC into memory */
	OP_LOADRX   = 6,    /**< Load RX into AC */
	OP_STRRX    = 7,    /**< Store AC into RX */
	OP_COMP     = 8,    /**< Compare data with AC */
	OP_JMPE     = 9,    /**< Jump if AC == M[SP] */
	OP_JMPNE    = 10,   /**< Jump if AC != M[SP] */
	OP_JMPLT    = 11,   /**< Jump if AC < M[SP] */
	OP_JMPLGT   = 12,   /**< Jump if AC > M[SP] */
	OP_SVC      = 13,   /**< System Call (Supervisor Call) */
	OP_RETRN    = 14,   /**< Return from subroutine */
	OP_HAB      = 15,   /**< Enable interrupts */
	OP_DHAB     = 16,   /**< Disable interrupts */
	OP_TTI      = 17,   /**< Set timer interval */
	OP_CHMOD    = 18,   /**< Change operation mode */
	OP_LOADRB   = 19,   /**< Load RB into AC */
	OP_STRRB    = 20,   /**< Store AC into RB */
	OP_LOADRL   = 21,   /**< Load RL into AC */
	OP_STRRL    = 22,   /**< Store AC into RL */
	OP_LOADSP   = 23,   /**< Load SP into AC */
	OP_STRSP    = 24,   /**< Store AC into SP */
	OP_PSH      = 25,   /**< Push to Stack */
	OP_POP      = 26,   /**< Pop from Stack */
	OP_J        = 27,   /**< Unconditional Jump */
	OP_SDMAP    = 28,   /**< DMA: Set Track */
	OP_SDMAC    = 29,   /**< DMA: Set Cylinder */
	OP_SDMAS    = 30,   /**< DMA: Set Sector */
	OP_SDMAIO   = 31,   /**< DMA: Set I/O Mode (Read/Write) */
	OP_SDMAM    = 32,   /**< DMA: Set Memory Address */
	OP_SDMAON   = 33    /**< DMA: Start Transfer */
} OpCode_t;

/**
 * @brief Interrupt Vector.
 * Defines indices for exception handling.
 */
typedef enum {
	IC_INVALID_SYSCALL   = 0,
	IC_INVALID_INT_CODE  = 1,
	IC_SYSCALL           = 2,
	IC_TIMER             = 3,
	IC_IO_DONE           = 4,
	IC_INVALID_INSTR     = 5,
	IC_INVALID_ADDR      = 6,
	IC_UNDERFLOW         = 7,
	IC_OVERFLOW          = 8
} InterruptCode_t;

/** @brief Program Status Word (PSW). */
typedef struct {
	ConditionCode_t conditionCode;      /**< CC: Arithmetic result status */
	OpMode_t mode;                      /**< Kernel/User Mode */
	InterruptionState_t interruptEnable;/**< Interrupt state */
	address pc;                         /**< Program Counter (part of PSW) */
} PSW_t;

/** @brief Processor Registers (CPU Context). */
typedef struct {
	word AC;        /**< Accumulator */
	word MAR;       /**< Memory Address Register */
	word MDR;       /**< Memory Data Register */
	word IR;        /**< Instruction Register */
	word RB;        /**< Base Register (Protection) */
	word RL;        /**< Limit Register (Protection) */
	word RX;        /**< Index/Auxiliary Register */
	word SP;        /**< Stack Pointer */
	PSW_t PSW;      /**< Program Status Word */
} CPU_t;

/** @brief Helper structure for instruction decoding. */
typedef struct {
	OpCode_t opCode;            /**< Operation Code (0-33) */
	DirectionMode_t direction;  /**< Addressing Mode */
	int value;                  /**< Operand (Address or Immediate Value) */
} Instruction_t;

/** @brief DMA Controller Definition (Direct Memory Access). */
typedef struct {
	int track;          /**< Target Track */
	int cylinder;       /**< Target Cylinder */
	int sector;         /**< Target Sector */
	int ioDirection;    /**< 0: Read from Disk, 1: Write to Disk */
	address memAddr;    /**< Target physical RAM address */
	int status;         /**< Result: 0=Success, 1=Error */
	bool active;        /**< Status flag: true if transfer is in progress */
} DMA_t;

/**
 * @brief Physical representation of a disk sector.
 * Encapsulates the word to maintain the 3D array structure.
 */
typedef struct {
	word data; /**< Stored data (9 logical chars / 1 integer) */
} Sector_t;

/** @brief Extracts the first 2 digits for OpCode. */
#define GET_INSTRUCTION_OPCODE(w) ((w) / 1000000)
/** @brief Extracts the 3rd digit for Addressing Mode. */
#define GET_INSTRUCTION_MODE(w)   (((w) / 100000) % 10)
/** @brief Extracts the last 5 digits as Value/Address. */
#define GET_INSTRUCTION_VALUE(w)  ((w) % 100000)
/** @brief Bit (digit) representing the negative sign (10000000). */
#define SIGN_BIT 10000000
/** @brief Checks if a word is negative (1xxxxxxx). */
#define IS_NEGATIVE(w)            ((w) >= SIGN_BIT)
/** @brief Gets only the magnitude (the lower 7 digits). */
#define GET_MAGNITUDE(w)          ((w) % SIGN_BIT)
/** @brief Maximum valid word value (19999999). */
#define MAX_WORD_VALUE (SIGN_BIT + MAX_MAGNITUDE)
/** @brief Validates if a word is within the allowed range. */
#define IS_VALID_WORD(w)  ((w) >= 0 && (w) <= MAX_WORD_VALUE)

/** @brief Shared Main Memory (RAM). */
extern word RAM[RAM_SIZE];
/** @brief Global Processor Instance. */
extern CPU_t CPU;
/** @brief Global DMA Instance. */
extern DMA_t DMA;
/** @brief Virtual Hard Disk. */
extern Sector_t DISK[DISK_TRACKS][DISK_CYLINDERS][DISK_SECTORS];
/** @brief Mutex for Memory Bus Arbitration. */
extern pthread_mutex_t BUS_LOCK;
/** @brief Condition variable to synchronize DMA start. */
extern pthread_cond_t DMA_COND;

#endif // DEFINITIONS_H
