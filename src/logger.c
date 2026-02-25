#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#include "../inc/logger.h"

const char* HARDWARE_LOG_FILE_NAME = "logs_hardware.txt";
const char* KERNEL_LOG_FILE_NAME = "logs_kernel.txt";

FILE* hardwareLogFile = NULL;
FILE* kernelLogFile = NULL;

static pthread_mutex_t LOG_LOCK = PTHREAD_MUTEX_INITIALIZER;

static void getCurrentTimeString(char* buffer, size_t size) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	
	if (t == NULL || strftime(buffer, size, "%y-%m-%d %H:%M:%S", t) == 0) {
		strncpy(buffer, "UNKNOWN_TIME", size);
	}
}


static void saveInLogFile(LogLevel_t level, const char* message, LogType_t logType) {
	FILE* targetFile = (logType == KERNEL_LOG) ? kernelLogFile : hardwareLogFile;

	if (targetFile == NULL) return;

	pthread_mutex_lock(&LOG_LOCK);
	char timeBuffer[32];
	getCurrentTimeString(timeBuffer, sizeof(timeBuffer));
	const char* prefix = ":";

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

	fprintf(targetFile, "[%s]%s %s\n", timeBuffer, prefix, message);
	fflush(targetFile);
	pthread_mutex_unlock(&LOG_LOCK);
}


void loggerInit(void) {
	pthread_mutex_lock(&LOG_LOCK);
	hardwareLogFile = fopen(HARDWARE_LOG_FILE_NAME, "a");
	kernelLogFile = fopen(KERNEL_LOG_FILE_NAME, "a");
	pthread_mutex_unlock(&LOG_LOCK);
}


void loggerClose(void) {
	pthread_mutex_lock(&LOG_LOCK);
	if (hardwareLogFile != NULL) {
		fclose(hardwareLogFile);
		hardwareLogFile = NULL;
	}
	if (kernelLogFile != NULL) {
		fclose(kernelLogFile);
		kernelLogFile = NULL;
	}
	pthread_mutex_unlock(&LOG_LOCK);
}


void loggerLogHardware(LogLevel_t level, const char* message) {
	saveInLogFile(level, message, HARDWARE_LOG);
}


void loggerLogKernel(LogLevel_t level, const char* message) {
	saveInLogFile(level, message, KERNEL_LOG);
}


void loggerLogInterrupt(InterruptCode_t code) {
	const char *message = "Unknown interrupt code";
	bool isError = false;

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

	switch (code) {
		case IC_INVALID_SYSCALL:
		case IC_INVALID_INT_CODE:
		case IC_INVALID_INSTR:
		case IC_INVALID_ADDR: isError = true;
		default: break;
	}

	if (isError) {
		loggerLogHardware(LOG_ERROR, message);
		printf("\x1b[31m[ERROR]\x1b[0m: %s\n", message);
	} else {
		loggerLogHardware(LOG_WARNING, message);
		printf("\x1b[33m[WARN]\x1b[0m: %s\n", message);
	}
	fflush(hardwareLogFile);
}
