#include "../lib/utest.h"
#include "../inc/console.h"
#include "../inc/loader.h"
#include "../inc/logger.h"

static char spyLastFileName[CONSOLE_BUFFER_SIZE];
static int spyCpuRunCallCount = 0;
static int spyCpuStepCallCount = 0;

// Mock for Cpu Initialization
CPU_t CPU;

// Mock for Loader
ProgramInfo_t loadProgram(char* fileName) {
	ProgramInfo_t programInfo;
	if (strcmp(fileName, "program.txt") == 0) {
		strncpy(spyLastFileName, fileName, 99);
		programInfo.status = LOAD_SUCCESS;
		return programInfo;
	}
	strncpy(spyLastFileName, "", 99);
	programInfo.status = LOAD_FILE_ERROR;
    return programInfo;
}

// Mock for CPU Run
int cpuRun(void) {
    spyCpuRunCallCount++;
	return 0;
}

// Mock for CPU Step
bool cpuStep(void) {
    spyCpuStepCallCount++;
    return false;
}

// Mock for CPU Reset
void cpuReset(void) {
	return;
}

int wordToInt(word w) {
	return (IS_NEGATIVE(w))? -((int)GET_MAGNITUDE(w)): (int)GET_MAGNITUDE(w);
}

CommandStatus_t consoleProcessCommand(char* input) {
	char command[CONSOLE_BUFFER_SIZE];
	char argument[CONSOLE_BUFFER_SIZE];

	CommandStatus_t parseStatus = parseInput(input, command, argument);
	if (parseStatus == CMD_EMPTY) return CMD_EMPTY;

	if (strcmp(command, "EXIT") == 0) return CMD_SUCCESS;
	if (strcmp(command, "LOAD") == 0) return handleLoadCommand(argument);
	if (strcmp(command, "RUN") == 0) return handleRunCommand();
	if (strcmp(command, "DEBUG") == 0) return handleDebugCommand();

	return CMD_UNKNOWN;
}

UTEST_MAIN();

// Verify that the consoleProcessCommand function returns correct status codes for LOAD command
UTEST(Console, LoadCommandReturnsCorrectCodes) {
	loggerInit();
    memset(spyLastFileName, 0, CONSOLE_BUFFER_SIZE);

    char input[CONSOLE_BUFFER_SIZE];
    CommandStatus_t output;

    strcpy(input, "LOAD program.txt\n");
    output = consoleProcessCommand(input);
    ASSERT_EQ(output, (unsigned)CMD_SUCCESS);

    strcpy(input, "LOAD  program.txt\n");
    output = consoleProcessCommand(input);
    ASSERT_EQ(output, (unsigned)CMD_SUCCESS);

    strcpy(input, "LOAD incorrect.txt\n");
    output = consoleProcessCommand(input);
    ASSERT_EQ(output, (unsigned)CMD_LOAD_ERROR);

    strcpy(input, "LOAD    \n");
    output = consoleProcessCommand(input);
    ASSERT_EQ(output, (unsigned)CMD_MISSING_ARGS);
	loggerClose();
}

// Verify that the consoleProcessCommand function returns correct status codes for RUN command
UTEST(Console, RunCommandReturnsCorrectCode) {
	loggerInit();
    char input[CONSOLE_BUFFER_SIZE];

    strcpy(input, "RUN\n");
    CommandStatus_t output = consoleProcessCommand(input);
    ASSERT_EQ(output, (unsigned)CMD_SUCCESS);
	loggerClose();
}

// Verify that the consoleProcessCommand function returns correct status codes for EXIT command
UTEST(Console, ExitCommandReturnsCorrectCode) {
	loggerInit();
    char input[CONSOLE_BUFFER_SIZE];

    strcpy(input, "EXIT\n");
    CommandStatus_t output = consoleProcessCommand(input);
    ASSERT_EQ(output, (unsigned)CMD_SUCCESS);
	loggerClose();
}

// Verify that the consoleProcessCommand function returns correct status codes for empty input
UTEST(Console, EmptyInputReturnsCorrectCode) {
	loggerInit();
    char input[CONSOLE_BUFFER_SIZE];

    strcpy(input, "\n");
    CommandStatus_t output = consoleProcessCommand(input);
    ASSERT_EQ(output, (unsigned)CMD_EMPTY);
	loggerClose();
}

// Verify that the consoleProcessCommand function returns correct status codes for unknown command
UTEST(Console, UnknownCommandReturnsCorrectCode) {
	loggerInit();
    char input[CONSOLE_BUFFER_SIZE];

    strcpy(input, "INCORRECT_COMMAND\n");
    CommandStatus_t output = consoleProcessCommand(input);
    ASSERT_EQ(output, (unsigned)CMD_UNKNOWN);
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
