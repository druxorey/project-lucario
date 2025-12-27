#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "../lib/utest.h"
#include "../inc/logger.h"

bool isDebugMode = false;

UTEST_MAIN();

// Auxiliary function for threading test
void* threadLogAction(void* arg) {
	int value = *((int*)arg);
	for (int i = 0; i < 100; i++) {
		char message[64];
		snprintf(message, sizeof(message), "Message %d from thread %d", i, value);
		loggerLog(LOG_INFO, message);
	}
	free(arg);
	return NULL;
}

// Verify that the log file is created upon initialization
UTEST(Logger, LogFileCreation) {
	loggerInit();
	loggerClose();

	FILE *f = fopen("logs.txt", "r");
	ASSERT_TRUE(f != NULL);
	if (f) fclose(f);
}

// Verify that log entries are correctly written to the log file
UTEST(Logger, LogFileUpdate) {
	loggerInit();
	loggerLog(LOG_ERROR, "Writing test log entry");
	loggerClose();
	
	FILE *f = fopen("logs.txt", "r");
	ASSERT_TRUE(f != NULL);
	
	char buffer[100];
	bool found = false;
	while (fgets(buffer, 100, f)) {
		if (strstr(buffer, "] [ERROR]: Writing test log entry")) {
			found = true;
			break;
		}
	}
	fclose(f);
	ASSERT_TRUE(found);
}

// Verify that interrupt logs are formatted correctly
UTEST(Logger, InterruptLogFormat) {
	// This one is hard to fully automate without redirecting stdout,
	// but we can check that the function doesn't crash and writes to disk.
	loggerInit();
	loggerLogInterrupt(IC_OVERFLOW);
	loggerClose();
	
	FILE *f = fopen("logs.txt", "r");
	ASSERT_TRUE(f != NULL);

	char buffer[256];
	bool found = false;
	while (fgets(buffer, (int)sizeof(buffer), f)) {
		if (strstr(buffer, "] [WARN]: Arithmetic overflow interrupt") != NULL) {
			found = true;
			break;
		}
	}
	fclose(f);
	ASSERT_TRUE(found);
}

// Verify that debug messages are filtered based on debug mode
UTEST(Logger, DebugFiltering) {
	loggerInit();

	isDebugMode = true;
	
	loggerLog(LOG_DEBUG, "Secret Debug Message 1");
	loggerLog(LOG_INFO, "Message 2");
	loggerLog(LOG_DEBUG, "Secret Debug Message 3");
	
	FILE *f = fopen("logs.txt", "r");
	ASSERT_TRUE(f != NULL);
	
	char buffer[256];
	int count = 0;
	while (fgets(buffer, sizeof(buffer), f)) {
		if (strstr(buffer, "Secret Debug Message")) {
			count++;
		}
	}
	fclose(f);

	// Only the second message should be logged
	ASSERT_EQ(2, count);
}

// Verify thread-safety of the logger
UTEST(Logger, ThreadSafety) {
	loggerInit();

	pthread_t t1, t2;
	int *arg1 = malloc(sizeof(int));
	int *arg2 = malloc(sizeof(int));
	*arg1 = 1;
	*arg2 = 2;
	pthread_create(&t1, NULL, threadLogAction, arg1);
	pthread_create(&t2, NULL, threadLogAction, arg2);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	loggerClose();
 
	FILE *f = fopen("logs.txt", "r");
	int lines = 0;
	char c;
	while(!feof(f)) {
		c = fgetc(f);
		if(c == '\n') lines++;
	}
	fclose(f);
	
	// Note: It can vary by 1 depending on whether the last line has \n
	// But if 50 lines are missing, your logger is useless for the project.
	ASSERT_TRUE(lines >= 200);
}

// TODO: Verify compliance with the specified log format for CPU cycles
//UTEST(Logger, DebugFormatCompliance);
