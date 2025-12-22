#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "../lib/utest.h"
#include "../inc/logger.h"

UTEST_MAIN();

// Auxiliary function for threading test
void* thread_log_action(void* arg) {
	for (int i = 0; i < 100; i++) {
		char message[64];
		snprintf(message, sizeof(message), "Message from thread %d", i);
		loggerLog(LOG_INFO, message);
	}
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
	loggerLog(LOG_INFO, "Writing test log entry");
	loggerClose();
	
	FILE *f = fopen("logs.txt", "r");
	ASSERT_TRUE(f != NULL);
	
	char buffer[100];
	bool found = false;
	while (fgets(buffer, 100, f)) {
		if (strstr(buffer, "Writing test log entry")) {
			found = true;
			break;
		}
	}
	fclose(f);
	ASSERT_TRUE(found);
}

// Verify that interrupt logs are formatted correctly
UTEST(Logger, SimultaneousWriteToStdoutAndFile) {
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
		if (strstr(buffer, "Arithmetic overflow interrupt") != NULL) {
			found = true;
			break;
		}
	}
	fclose(f);
	ASSERT_TRUE(found);
}

// Verify thread-safety of the logger
UTEST(Logger, ThreadSafety) {
	loggerInit();

	pthread_t t1, t2;
	pthread_create(&t1, NULL, thread_log_action, NULL);
	pthread_create(&t2, NULL, thread_log_action, NULL);

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

// Verify compliance with the specified log format for CPU cycles
UTEST(Logger, DebugFormatCompliance) {
	loggerInit();
	loggerLogCpuCycle(100, "00100005 (SUM 5)", 15);
	loggerClose();
	
	FILE *f = fopen("logs.txt", "r");
	char buffer[200];
	fgets(buffer, 200, f);
	fclose(f);
	
	ASSERT_TRUE(strstr(buffer, "PC:100"));
	ASSERT_TRUE(strstr(buffer, "INST:00100005"));
	ASSERT_TRUE(strstr(buffer, "RES:15"));
}
