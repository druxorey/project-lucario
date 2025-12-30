#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#include "../inc/logger.h"

const char* LOG_FILE_NAME = "logs.txt";
FILE* logFile = NULL;
static pthread_mutex_t LOG_LOCK = PTHREAD_MUTEX_INITIALIZER;

static void getCurrentTimeString(char* buffer, size_t size) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	
	if (t == NULL || strftime(buffer, size, "%y-%m-%d %H:%M:%S", t) == 0) {
		strncpy(buffer, "UNKNOWN_TIME", size);
	}
}


void loggerInit(void) {
	pthread_mutex_lock(&LOG_LOCK);
	logFile = fopen(LOG_FILE_NAME, "a");
	pthread_mutex_unlock(&LOG_LOCK);
}


void loggerClose(void) {
	pthread_mutex_lock(&LOG_LOCK);
	if (logFile != NULL) {
		fclose(logFile);
		logFile = NULL;
	}
	pthread_mutex_unlock(&LOG_LOCK);
}


void loggerLog(LogLevel_t level, const char* message) {
	pthread_mutex_lock(&LOG_LOCK);
	logFile = fopen(LOG_FILE_NAME, "a");
	if (logFile != NULL) {
		char timeBuffer[32];
		getCurrentTimeString(timeBuffer, sizeof(timeBuffer));
		char* prefix = ":";

		switch (level) {
			case LOG_INFO:
				prefix = ":";
				break;
			case LOG_WARNING:
				prefix = ": [WARN]";
				break;
			case LOG_ERROR:
				prefix = ": [ERROR]";
				break;
		}

		fprintf(logFile, "[%s]%s %s\n", timeBuffer, prefix, message);
		fflush(logFile);
		fclose(logFile);
		logFile = NULL;
	}
	pthread_mutex_unlock(&LOG_LOCK);
}


void loggerLogInterrupt(InterruptCode_t code) {
	const char *message = "Unknown interrupt code";
	char timeBuffer[32];
	getCurrentTimeString(timeBuffer, sizeof(timeBuffer));

	switch (code) {
		case IC_INVALID_SYSCALL: message = "Invalid system call interrupt"; break;
		case IC_INVALID_INT_CODE: message = "Invalid interrupt code"; break;
		case IC_SYSCALL: message = "System call interrupt"; break;
		case IC_TIMER: message = "Timer interrupt"; break;
		case IC_IO_DONE: message = "I/O completion interrupt"; break;
		case IC_INVALID_INSTR: message = "Invalid instruction interrupt"; break;
		case IC_INVALID_ADDR: message = "Invalid memory address interrupt"; break;
		case IC_UNDERFLOW: message = "Arithmetic underflow interrupt"; break;
		case IC_OVERFLOW: message = "Arithmetic overflow interrupt"; break;
		default: break;
	}

	loggerLog(LOG_WARNING, message);
	printf("[%s] \x1b[33m[WARN]\x1b[0m: %s\n", timeBuffer, message);
	fflush(logFile);
}
