#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "../lib/utest.h"
#include "../inc/console.h"
#include "../inc/loader.h"
#include "../inc/cpu.h"
#include "../inc/logger.h"

UTEST_MAIN();

static char spyLastFileName[CONSOLE_BUFFER_SIZE];
static int spyCpuRunCallCount = 0;
static int spyCpuStepCallCount = 0;

CPU_t CPU;
word RAM[RAM_SIZE];
DMA_t DMA;
Sector_t DISK[DISK_TRACKS][DISK_CYLINDERS][DISK_SECTORS];
pthread_mutex_t BUS_LOCK;
pthread_cond_t DMA_COND;

// Mock for Loader
LoaderStatus_t loaderLoadProgram(const char* fileName) {
	if (strcmp(fileName, "program.txt") == 0) {
		strncpy(spyLastFileName, fileName, 99);
		return LOADER_SUCCESS;
	}
	strncpy(spyLastFileName, "", 99);
    return LOADER_ERROR;
}

// Mock for CPU Run (Normal Mode)
int cpuRun(void) {
    spyCpuRunCallCount++;
	return 1;
}

// Mock for CPU Step (Debug Mode)
bool cpuStep(void) {
    spyCpuStepCallCount++;
    return false;
}

// Mock for CPU Reset (Normal Mode)
void cpuReset(void) {
	return;
}

// Verify that the consoleProcessCommand function returns correct status codes for LOAD command
UTEST(Console, LoadCommandReturnsCorrectCodes) {
	loggerInit();
    memset(spyLastFileName, 0, CONSOLE_BUFFER_SIZE);

    char input[CONSOLE_BUFFER_SIZE];
    ConsoleStatus_t output;

    strcpy(input, "LOAD program.txt\n");
    output = consoleProcessCommand(input);
    ASSERT_EQ(output, CMD_SUCCESS);

    strcpy(input, "LOAD  program.txt\n");
    output = consoleProcessCommand(input);
    ASSERT_EQ(output, CMD_SUCCESS);

    strcpy(input, "LOAD incorrect.txt\n");
    output = consoleProcessCommand(input);
    ASSERT_EQ(output, CMD_LOAD_ERROR);

    strcpy(input, "LOAD    \n");
    output = consoleProcessCommand(input);
    ASSERT_EQ(output, CMD_MISSING_ARGS);
	loggerClose();
}

// Verify that the consoleProcessCommand function returns correct status codes for RUN command
UTEST(Console, RunCommandReturnsCorrectCode) {
	loggerInit();
    char input[CONSOLE_BUFFER_SIZE];

    strcpy(input, "RUN\n");
    ConsoleStatus_t output = consoleProcessCommand(input);
    ASSERT_EQ(output, CMD_SUCCESS);
	loggerClose();
}

// Verify that the consoleProcessCommand function returns correct status codes for EXIT command
UTEST(Console, ExitCommandReturnsCorrectCode) {
	loggerInit();
    char input[CONSOLE_BUFFER_SIZE];

    strcpy(input, "EXIT\n");
    ConsoleStatus_t output = consoleProcessCommand(input);
    ASSERT_EQ(output, CMD_EXIT);
	loggerClose();
}

// Verify that the consoleProcessCommand function returns correct status codes for empty input
UTEST(Console, EmptyInputReturnsCorrectCode) {
	loggerInit();
    char input[CONSOLE_BUFFER_SIZE];

    strcpy(input, "\n");
    ConsoleStatus_t output = consoleProcessCommand(input);
    ASSERT_EQ(output, CMD_EMPTY);
	loggerClose();
}

// Verify that the consoleProcessCommand function returns correct status codes for unknown command
UTEST(Console, UnknownCommandReturnsCorrectCode) {
	loggerInit();
    char input[CONSOLE_BUFFER_SIZE];

    strcpy(input, "INCORRECT_COMMAND\n");
    ConsoleStatus_t output = consoleProcessCommand(input);
    ASSERT_EQ(output, CMD_UNKNOWN);
	loggerClose();
}

// Verify that the consoleProcessCommand parses LOAD command and calls loaderLoadProgram with correct filename
UTEST(Console, CommandLoad) {
	loggerInit();
    memset(spyLastFileName, 0, CONSOLE_BUFFER_SIZE);
    char input[] = "LOAD program.txt\n";
    consoleProcessCommand(input);
    ASSERT_STREQ("program.txt", spyLastFileName);
	loggerClose();
}

// Verify that the consoleProcessCommand calls cpuRun when RUN command is processed
UTEST(Console, CommandRunTriggersCpuRun) {
	loggerInit();
    spyCpuRunCallCount = 0;
    char input[] = "RUN\n";
    consoleProcessCommand(input);
    ASSERT_EQ(1, spyCpuRunCallCount);
	loggerClose();
}

// Verify that the consoleProcessCommand calls cpuStep when DEBUG command is processed
UTEST(Console, CommandDebugTriggersCpuStep) {
	loggerInit();
    spyCpuStepCallCount = 0;
    char input[] = "DEBUG\n";

    consoleProcessCommand(input);
    ASSERT_EQ(1, spyCpuStepCallCount);
	loggerClose();
}

/*
// Verify that the consoleStart function initializes the console correctly
UTEST(Console, ConsoleInitialization) {
	loggerInit();
    int result = consoleStart();
	ASSERT_EQ(0, result);
	loggerClose();
}
*/
