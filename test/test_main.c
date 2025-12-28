#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "../lib/utest.h"
#include "../inc/definitions.h"
#include "../inc/logger.h"
#include "../inc/memory.h"
#include "../inc/console.h"

// Global mock BUS_LOCK pthread mutex
pthread_mutex_t BUS_LOCK;

// Mock flags to verify function calls
static int loggerInitCalled = 0;
static int memoryInitCalled = 0;
static int loggerCloseCalled = 0;
static int consoleStartCalled = 0;
static int loggerLogCalled = 0;
// Mock stub implementations for the functions called by main
void loggerInit(void) { loggerInitCalled++; }
void memoryInit(void) { memoryInitCalled++; pthread_mutex_init(&BUS_LOCK, NULL); }
void loggerClose(void) { loggerCloseCalled++; }
int consoleStart(void) { consoleStartCalled++; return 0; }
void loggerLog(LogLevel_t level, const char* message) {
	(void)level;
	(void)message;
	loggerLogCalled++;
}

// Rename the function under test to avoid conflict with UTEST_MAIN()'s main
#define main main_under_test
#include "../src/main.c"
#undef main

// Forward declaration to satisfy compilers that error on
// implicit function declarations (GCC newer versions).
int main_under_test(int argc, char* argv[]);

UTEST_MAIN();

// Verify that main returns EXIT_SUCCESS.
UTEST(Main, ReturnsExitSuccess) {
	// Arrange
	loggerInitCalled = memoryInitCalled = loggerCloseCalled = consoleStartCalled = 0;

	// Act
	int rc = main_under_test(0, NULL);

	// Assert
	ASSERT_EQ(EXIT_SUCCESS, rc);
}

// Verify that main calls the init and shutdown sequences.
UTEST(Main, CallsInitAndShutdownSequence) {
	// Arrange
	loggerInitCalled = memoryInitCalled = loggerCloseCalled = consoleStartCalled = 0;

	// Act
	(void)main_under_test(0, NULL);

	// Assert
	ASSERT_EQ(1, loggerInitCalled);
	ASSERT_EQ(1, memoryInitCalled);
	ASSERT_EQ(1, consoleStartCalled);
	ASSERT_EQ(1, loggerCloseCalled);
}






