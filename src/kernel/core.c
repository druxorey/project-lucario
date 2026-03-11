#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#include <string.h>
#include "../../inc/logger.h"
#include "../../inc/hardware/disk.h"
#include "../../inc/hardware/memory.h"
#include "../../inc/hardware/cpu.h"
#include "../../inc/kernel/core.h"
#include "../../inc/kernel/mmu.h"
#include "../../inc/kernel/vfs.h"
#include "../../inc/kernel/scheduler.h"

PCB_t PROCESS_TABLE[MAX_PROCESSES];
int currentActiveProcess = -1;
bool osYield = false;

static bool osRunning = false;
static int nextPid = 1;
static pthread_t cpuThread;

void* cpuThreadWorker(void* arg) {
	(void)arg;
	loggerLogKernel(LOG_INFO, "CPU Background Thread started");

	schedulerTick();

	while (osRunning) {
		if (currentActiveProcess != -1) {
			bool keepRunning = cpuStep();
			
			if (!keepRunning) {
				char logBuffer[LOG_BUFFER_SIZE];
				snprintf(logBuffer, LOG_BUFFER_SIZE, "Process PID [%d] terminated. Cleaning resources.", PROCESS_TABLE[currentActiveProcess].pid);
				loggerLogKernel(LOG_INFO, logBuffer);
				freeMemory(PROCESS_TABLE[currentActiveProcess].startBlock, PROCESS_TABLE[currentActiveProcess].blockCount);
				PROCESS_TABLE[currentActiveProcess].state = FINISHED;
				osYield = false;
				schedulerTick();
			} else if (osYield) {
				osYield = false;
				schedulerTick();
			}
			
			usleep(250000);
		} else {
			usleep(100000);
			schedulerTick();
		}
	}

	loggerLogKernel(LOG_INFO, "CPU Background Thread stopped");
	return NULL;
}


OSStatus_t osStart(void) {
	osRunning = true;
	if (pthread_create(&cpuThread, NULL, cpuThreadWorker, NULL) != 0) {
		loggerLogKernel(LOG_ERROR, "Failed to create CPU thread");
		return OS_ERR_THREAD;
	}
	return OS_SUCCESS;
}


OSStatus_t osStop(void) {
	osRunning = false;
	if (pthread_join(cpuThread, NULL) != 0) {
		loggerLogKernel(LOG_ERROR, "Failed to join CPU thread");
		return OS_ERR_THREAD;
	}
	return OS_SUCCESS;
}


OSStatus_t initOS(void) {
	mmuInit();

	for (int i = 0; i < MAX_PROCESSES; i++) {
		PROCESS_TABLE[i].state = FINISHED;
		PROCESS_TABLE[i].pid = -1;
	}
	
	currentActiveProcess = -1;
	
	loggerLogKernel(LOG_INFO, "OS initialized: Process Table completely flushed and ready");
	return OS_SUCCESS;
}


int getFreePCBIndex(void) {
	for (int i = 0; i < MAX_PROCESSES; i++) {
		if (PROCESS_TABLE[i].state == FINISHED) return i;
	}
	
	loggerLogKernel(LOG_WARNING, "Process limit reached. No free PCB available");
	return -1;
}


OSStatus_t createProcess(char* progName) {
	char logBuffer[LOG_BUFFER_SIZE];

	int pcbIndex = getFreePCBIndex();
	if (pcbIndex == -1) {
		loggerLogKernel(LOG_ERROR, "Failed to create process: Max processes limit reached");
		return OS_ERR_MAX_PROCESSES;
	}

	PROCESS_TABLE[pcbIndex].state = NEW;

	snprintf(logBuffer, LOG_BUFFER_SIZE, "PCB reserved at index %d", pcbIndex);
	loggerLogKernel(LOG_INFO, logBuffer);

	if (!vfsFileExists(progName)) {
		if (vfsLoadToDisk(progName) != VFS_SUCCESS) {
			PROCESS_TABLE[pcbIndex].state = FINISHED;
			snprintf(logBuffer, LOG_BUFFER_SIZE, "Failed to create process: Could not load '%s' to Virtual Disk", progName);
			loggerLogKernel(LOG_ERROR, logBuffer);
			return OS_ERR_DISK;
		}
	}

	snprintf(logBuffer, LOG_BUFFER_SIZE, "Program '%s' verified in VFS", progName);
	loggerLogKernel(LOG_INFO, logBuffer);

	FileMeta_t meta;
	if (vfsGetMetadata(progName, &meta) != VFS_SUCCESS) {
		PROCESS_TABLE[pcbIndex].state = FINISHED;
		return OS_ERR_DISK;
	}

	int requiredBlocks = calculateRequiredBlocks(meta.wordCount);
	int startBlock = allocateMemory(requiredBlocks);
	
	if (startBlock == -1) {
		PROCESS_TABLE[pcbIndex].state = FINISHED;
		snprintf(logBuffer, LOG_BUFFER_SIZE, "Failed to create process '%s': Insufficient contiguous RAM", progName);
		loggerLogKernel(LOG_ERROR, logBuffer);
		return OS_ERR_MEMORY;
	}

	snprintf(logBuffer, LOG_BUFFER_SIZE, "Allocated %d blocks starting at block %d", requiredBlocks, startBlock);
	loggerLogKernel(LOG_INFO, logBuffer);

	int RB = GET_BASE_REGISTER(startBlock);
	uint8_t track = meta.startTrack;
	uint8_t cylinder = meta.startCylinder;
	uint8_t sector = meta.startSector;

	for (int i = 0; i < meta.wordCount; i++) {
		Sector_t actualSector;
		readSector(track, cylinder, sector, &actualSector);
		dmaWriteMemory(RB + i, actualSector.data);
		sector++;
		if (sector >= DISK_SECTORS) {
			sector = 0;
			cylinder++;
			if (cylinder >= DISK_CYLINDERS) {
				cylinder = 0;
				track++;
			}
		}
	}

	snprintf(logBuffer, LOG_BUFFER_SIZE, "Loaded %d words into RAM (Physical Base: %d)", meta.wordCount, RB);
	loggerLogKernel(LOG_INFO, logBuffer);

	PROCESS_TABLE[pcbIndex].pid = nextPid++;
	strncpy(PROCESS_TABLE[pcbIndex].programName, meta.programName, 255);
	PROCESS_TABLE[pcbIndex].programName[255] = '\0';
	PROCESS_TABLE[pcbIndex].startBlock = startBlock;
	PROCESS_TABLE[pcbIndex].blockCount = requiredBlocks;
	PROCESS_TABLE[pcbIndex].sleepTics = 0;

	CPU_t* ctx = &PROCESS_TABLE[pcbIndex].context;
	*ctx = (CPU_t){0};

	ctx->RB = RB;
	ctx->RL = GET_LIMIT_REGISTER(RB, requiredBlocks);
	ctx->RX = meta.wordCount;
	ctx->SP = (requiredBlocks * PARTITION_SIZE) - 1;
	ctx->PSW.pc = meta.startPC - 1;
	ctx->PSW.mode = MODE_USER;
	ctx->PSW.interruptEnable = ITR_ENABLED;
	ctx->timerLimit = 2;

	snprintf(logBuffer, LOG_BUFFER_SIZE, "Context initialized (PC: %d, Mode: USER)", ctx->PSW.pc);
	loggerLogKernel(LOG_INFO, logBuffer);

	PROCESS_TABLE[pcbIndex].state = READY;

	snprintf(logBuffer, LOG_BUFFER_SIZE, "Process created successfully [PID %d] - '%s'", PROCESS_TABLE[pcbIndex].pid, meta.programName);
	loggerLogKernel(LOG_INFO, logBuffer);
	snprintf(logBuffer, LOG_BUFFER_SIZE, "PID %d Info -  Blocks: %d, RB: %d, RL: %d", PROCESS_TABLE[pcbIndex].pid, requiredBlocks, ctx->RB, ctx->RL);
	loggerLogKernel(LOG_INFO, logBuffer);

	return OS_SUCCESS;
}
