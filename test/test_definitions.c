#include <stdbool.h>

#include "../lib/utest.h"
#include "../inc/definitions.h"

UTEST_MAIN();

// Verifies global system constants match the PDF requirements
UTEST(Definitions, SystemConstants) {
	ASSERT_EQ(2000, RAM_SIZE);
	ASSERT_EQ(300, OS_RESERVED_SIZE);
	ASSERT_EQ(9, SECTOR_SIZE);
	ASSERT_EQ(10000000, SIGN_BIT);
}

// Tests the instruction decoding macros (OODVVVVV)
UTEST(Definitions, InstructionMacros) {
	// input: 04100005 | 04 (OpCode) | 1 (Mode) | 00005 (Value)
	word instruction = 4100005;
	ASSERT_EQ(4, GET_INSTRUCTION_OPCODE(instruction));
	ASSERT_EQ(1, GET_INSTRUCTION_MODE(instruction));
	ASSERT_EQ(5, GET_INSTRUCTION_VALUE(instruction));
}

// Tests Sign-Magnitude logic.
UTEST(Definitions, SignMagnitudeMacros) {
	// 0xxxxxxx = Positive, 1xxxxxxx = Negative.
	word positive = 2345678;
	word negative = 12345678;

	ASSERT_FALSE(IS_NEGATIVE(positive));
	ASSERT_TRUE(IS_NEGATIVE(negative));
	
	// Ensure GET_MAG strips the sign bit (10000099 -> 99)
	ASSERT_EQ(2345678, GET_MAGNITUDE(negative));
	ASSERT_EQ(2345678, GET_MAGNITUDE(positive));
}

// Verifies Word validation logic.
UTEST(Definitions, WordValidationIntegrity) {
    word validPos = 9999999;
    word validNeg = 19999999;
    word invalidHuge = 20000000; // Sign digit '2', invalid
    word invalidNeg = -5; // Negative in standard C int, invalid for this arch

    ASSERT_TRUE(IS_VALID_WORD(validPos));
    ASSERT_TRUE(IS_VALID_WORD(validNeg));
    ASSERT_FALSE(IS_VALID_WORD(invalidHuge));
    ASSERT_FALSE(IS_VALID_WORD(invalidNeg));
}

// Verifies Struct hierarchy and size consistency.
UTEST(Definitions, StructureIntegrity) {
	// 1. Verify Sector_t is just a wrapper for 'word' (no extra overhead)
	ASSERT_EQ(sizeof(int), sizeof(Sector_t));

	// 2. Verify CPU nesting (CPU contains PSW)
	CPU_t cpu_test = {0};
	cpu_test.PSW.mode = MODE_KERNEL;
	
	// 3. Ensure we can access PSW fields through CPU
	ASSERT_EQ((unsigned)1, cpu_test.PSW.mode);
}

// Checks critical OpCodes limits to ensure Enum integrity.
UTEST(Definitions, OpCodeBoundaries) {
	ASSERT_EQ(0, OP_SUM);
	ASSERT_EQ(13, OP_SVC);
	ASSERT_EQ(33, OP_SDMAON);
}

// Verifies Disk Geometry dimensions.
UTEST(Definitions, DiskGeometry) {
	ASSERT_EQ(10, DISK_TRACKS);
	ASSERT_EQ(10, DISK_CYLINDERS);
	ASSERT_EQ(100, DISK_SECTORS);
	
	// Verify array sizing logic holds up
	// Total Sectors = 10 * 10 * 100 = 10,000
	int total_sectors = DISK_TRACKS * DISK_CYLINDERS * DISK_SECTORS;
	ASSERT_EQ(10000, total_sectors);
}
