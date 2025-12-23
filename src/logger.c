#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#include "../inc/logger.h"

const char* LOG_FILE_NAME = "logs.txt";
FILE* logFile = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

char* getCurrentTimeString(void) {
    /* 19 chars + '\0' for "YYYY-MM-DD HH:MM:SS" */
    static char buffer[18];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t == NULL) {
        buffer[0] = '\0';
        return buffer;
    }
    if (strftime(buffer, sizeof(buffer), "%y-%m-%d %H:%M:%S", t) == 0) {
        buffer[0] = '\0';
    }
    return buffer;
}


void loggerInit(void) {
    pthread_mutex_lock(&log_mutex);
    logFile = fopen(LOG_FILE_NAME, "w");
    pthread_mutex_unlock(&log_mutex);
}


void loggerClose(void) {
    pthread_mutex_lock(&log_mutex);
    if (logFile != NULL) {
        fclose(logFile);
        logFile = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
}


void loggerLog(LogLevel_t level, const char* message) {
    pthread_mutex_lock(&log_mutex);
    logFile = fopen(LOG_FILE_NAME, "a");
    if (logFile != NULL) {
		char* actualTime = getCurrentTimeString();
		char* prefix = "";

        switch (level) {
            case LOG_INFO:
				prefix = ":";
                break;
            case LOG_WARNING:
				prefix = " [WARN]:";
                break;
            case LOG_ERROR:
				prefix = " [ERROR]:";
                break;
            case LOG_DEBUG:
				prefix = " [DEBUG]:";
                break;
        }

        fprintf(logFile, "[%s]%s %s\n", actualTime, prefix, message);
        fflush(logFile);
        fclose(logFile);
        logFile = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
}

void loggerLogInterrupt(InterruptCode_t code) {
	const char *message = "Unknown interrupt code";
	const char *actualTime = getCurrentTimeString();

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
	printf("[%s] %s[WARN]%s: %s\n", actualTime, COLOR_WARNING, COLOR_RESET, message);
	fflush(logFile);
}
