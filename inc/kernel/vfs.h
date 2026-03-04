/**
 * @file vfs.h
 * @brief Interfaces for loading programs into the Virtual Hardware memory and Disk Catalog.
 *
 * Declares data structures and functions to manage the Virtual File System (VFS),
 * read program metadata, and store them in the Virtual Hardware.
 *
 * @version 1.3
 */

#ifndef VFS_H
#define VFS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "../../inc/definitions.h"

/**
 * @brief VFS Status Codes.
 * Indicates the result of virtual file system operations.
 */
typedef enum {
	VFS_SUCCESS       = 0, /**< Operation completed successfully */
	VFS_ERR_NOT_FOUND = 1, /**< File not found in the catalog */
	VFS_ERR_DISK_FULL = 2  /**< Maximum capacity of the catalog reached */
} VFSStatus_t;

/**
 * @brief Loader Status Codes. (Legacy/Transition)
 */
typedef enum {
	LOAD_SUCCESS      = 0,
	LOAD_FILE_ERROR   = 1,
	LOAD_MEMORY_ERROR = 2
} LoadStatus_t;

/**
 * @brief Program information obtained from input file. (Legacy/Transition)
 */
typedef struct {
	int _start;
	int wordCount;
	char programName[256];
	LoadStatus_t status;
} ProgramInfo_t;

/**
 * @brief File Metadata for the Disk Catalog.
 */
typedef struct {
	char fileName[256];     /**< Name of the program file (e.g., "calc.txt") */
	uint8_t startTrack;     /**< Track where the program starts in the virtual disk */
	uint8_t startCylinder;  /**< Cylinder where the program starts */
	uint8_t startSector;    /**< Sector where the program starts */
	int wordCount;          /**< Total number of words the program occupies */
	int startPC;            /**< Starting Program Counter (PC) value for execution */
} FileMeta_t;

/**
 * @brief Retrieves the current number of files registered in the catalog.
 */
int vfsGetCatalogCount(void);

/**
 * @brief Retrieves the metadata of a specific catalog entry by its index.
 */
VFSStatus_t vfsGetCatalogEntry(int index, FileMeta_t* outMeta);

/**
 * @brief Checks if a file exists in the VFS catalog.
 * @param fileName Name of the file to search for.
 * @return true if the file is registered, false otherwise.
 */
bool vfsFileExists(const char* fileName);

/**
 * @brief Retrieves the metadata of a file from the catalog.
 * @param fileName Name of the file to search for.
 * @param outMeta Pointer to a FileMeta_t structure to store the result.
 * @return VFSStatus_t VFS_SUCCESS if found, VFS_ERR_NOT_FOUND otherwise.
 */
VFSStatus_t vfsGetMetadata(const char* fileName, FileMeta_t* outMeta);

/**
 * @brief Registers a new file in the VFS catalog.
 * @return VFSStatus_t VFS_SUCCESS or VFS_ERR_DISK_FULL.
 */
VFSStatus_t vfsRegisterFile(const char* fileName, uint8_t track, uint8_t cyl, uint8_t sec, int words, int startPC);

/**
 * @brief Clears the VFS catalog (Useful for tests and system restarts).
 */
void vfsClearCatalog(void);

/**
 * @brief Reads a program from the host OS and injects it into the Virtual Disk.
 * @param filePath Path to the .txt program.
 * @return VFSStatus_t Success or specific error.
 */
VFSStatus_t vfsLoadToDisk(const char* filePath);

// --- Legacy / Transition Functions ---
word readProgramWord(FILE* file);
ProgramInfo_t loadProgram(char* filePath);

#endif // VFS_H
