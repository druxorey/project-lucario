#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "../../inc/logger.h"
#include "../../inc/hardware/memory.h"
#include "../../inc/hardware/disk.h"
#include "../../inc/kernel/vfs.h"

static FileMeta_t diskCatalog[MAX_PROCESSES];
static int catalogCount = 0;

static uint8_t freeTrack = 0;
static uint8_t freeCylinder = 0;
static uint8_t freeSector = 0;

bool vfsFileExists(const char* fileName) {
	for (int i = 0; i < catalogCount; i++) {
		if (strcmp(diskCatalog[i].fileName, fileName) == 0) return true;
	}
	return false;
}


VFSStatus_t vfsGetMetadata(const char* fileName, FileMeta_t* outMeta) {
	for (int i = 0; i < catalogCount; i++) {
		if (strcmp(diskCatalog[i].fileName, fileName) == 0) {
			if (outMeta != NULL) *outMeta = diskCatalog[i];
			return VFS_SUCCESS;
		}
	}
	return VFS_ERR_NOT_FOUND;
}


VFSStatus_t vfsRegisterFile(const char* fileName, uint8_t track, uint8_t cyl, uint8_t sec, int words, int startPC) {
	if (catalogCount >= MAX_PROCESSES) {
		loggerLogKernel(LOG_WARNING, "VFS Error: Disk catalog is full.");
		return VFS_ERR_DISK_FULL;
	}
	
	strncpy(diskCatalog[catalogCount].fileName, fileName, 255);
	diskCatalog[catalogCount].fileName[255] = '\0';
	diskCatalog[catalogCount].startTrack = track;
	diskCatalog[catalogCount].startCylinder = cyl;
	diskCatalog[catalogCount].startSector = sec;
	diskCatalog[catalogCount].wordCount = words;
	diskCatalog[catalogCount].startPC = startPC;
	
	catalogCount++;
	
	char logBuffer[LOG_BUFFER_SIZE];
	snprintf(logBuffer, LOG_BUFFER_SIZE, "VFS: File '%s' registered in catalog at T:%d C:%d S:%d", fileName, track, cyl, sec);
	loggerLogKernel(LOG_INFO, logBuffer);
	
	return VFS_SUCCESS;
}


void vfsClearCatalog(void) {
	catalogCount = 0;
	freeTrack = 0;
	freeCylinder = 0;
	freeSector = 0;
	memset(diskCatalog, 0, sizeof(diskCatalog));
}


VFSStatus_t vfsLoadToDisk(const char* filePath) {
	if (vfsFileExists(filePath)) {
		return VFS_SUCCESS;
	}

	FILE* file = fopen(filePath, "r");
	if (!file) {
		loggerLogKernel(LOG_ERROR, "VFS Error: File not found in host OS.");
		return VFS_ERR_NOT_FOUND;
	}

	int startPC, wordCount;
	char internalProgName[256];
	fscanf(file, "%*s %d", &startPC);
	fscanf(file, "%*s %d", &wordCount);
	fscanf(file, "%*s %s", internalProgName);

	int totalDiskSectors = DISK_TRACKS * DISK_CYLINDERS * DISK_SECTORS;
	int usedSectors = (freeTrack * DISK_CYLINDERS * DISK_SECTORS) + (freeCylinder * DISK_SECTORS) + freeSector;
	if (wordCount > (totalDiskSectors - usedSectors)) {
		loggerLogKernel(LOG_ERROR, "VFS Error: Not enough contiguous space on Virtual Disk.");
		fclose(file);
		return VFS_ERR_DISK_FULL;
	}

	uint8_t startT = freeTrack;
	uint8_t startC = freeCylinder;
	uint8_t startS = freeSector;

	for (int i = 0; i < wordCount; i++) {
		word instruction = readProgramWord(file);
		
		Sector_t sectorData;
		sectorData.data = instruction;
		
		writeSector(freeTrack, freeCylinder, freeSector, sectorData);

		freeSector++;
		if (freeSector >= DISK_SECTORS) {
			freeSector = 0;
			freeCylinder++;
			if (freeCylinder >= DISK_CYLINDERS) {
				freeCylinder = 0;
				freeTrack++;
			}
		}
	}

	fclose(file);
	loggerLogKernel(LOG_INFO, "VFS: Program fully written to Virtual Disk.");

	return vfsRegisterFile(filePath, startT, startC, startS, wordCount, startPC);
}


// --- Legacy Loader Functions ---
word readProgramWord(FILE* filePtr) {
	char line[512];
	word w = 0;

	while (fgets(line, sizeof(line), filePtr) != NULL) {
		char* comment = strstr(line, "//");
		if (comment) {
			*comment = '\0';
		}

		if (sscanf(line, "%d", &w) == 1) {
			return w;
		}
	}
	return 0;
}


ProgramInfo_t loadProgram(char* filePath) {
	ProgramInfo_t programInfo = {0};
	char logBuffer[LOG_BUFFER_SIZE];

	snprintf(logBuffer, LOG_BUFFER_SIZE, "Loader: Attempting to load program from file '%s'.", filePath);
	loggerLogHardware(LOG_INFO, logBuffer);

	FILE* programFile = fopen(filePath, "r");
	
	if (programFile) {
		loggerLogHardware(LOG_INFO, "Loader: File opened successfully. Parsing metadata...");

		fscanf(programFile, "%*s %d", &programInfo._start);
		fscanf(programFile, "%*s %d", &programInfo.wordCount);
		fscanf(programFile, "%*s %s", programInfo.programName);

		CPU.PSW.mode = MODE_KERNEL;

		if (OS_RESERVED_SIZE + MIN_STACK_SIZE + programInfo.wordCount > RAM_SIZE) {
			programInfo.status = LOAD_FILE_ERROR;
			return programInfo;
		}

		int stackMemory = RAM_SIZE - OS_RESERVED_SIZE - programInfo.wordCount;
		if (stackMemory > DEFAULT_STACK_SIZE) {
			stackMemory = DEFAULT_STACK_SIZE;
		}

		for (int i = 0 ; i < programInfo.wordCount; i++) {
			word instruction = readProgramWord(programFile);
			MemoryStatus_t ret = writeMemory(OS_RESERVED_SIZE + i, instruction);
			
			if (ret != MEM_SUCCESS) {
				fclose(programFile);
				programInfo.status = LOAD_MEMORY_ERROR;
				return programInfo;
			}
		}

		CPU.RB = OS_RESERVED_SIZE;
		CPU.RL = OS_RESERVED_SIZE + stackMemory + programInfo.wordCount;
		CPU.RX = programInfo.wordCount;
		CPU.SP = programInfo.wordCount + stackMemory;
		CPU.PSW.pc = programInfo._start - 1;
		CPU.timerLimit = 16;
		CPU.cyclesCounter = 0;
		CPU.PSW.mode = MODE_USER;
		CPU.PSW.interruptEnable = ITR_ENABLED;
		
		programInfo.status = LOAD_SUCCESS;
		fclose(programFile);
	} else {
		programInfo.status = LOAD_FILE_ERROR;
		return programInfo;
	}
	return programInfo;
}
