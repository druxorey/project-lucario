/**
 * @file loader.h
 * @brief Interfaces for loading programs into the Virtual Hardware memory.
 *
 * Declares data structures and functions to read program metadata and words
 * from an input file and store them in the Virtual Hardware memory.
 *
 * @version 1.0
 */

#ifndef LOADER_H
#define LOADER_H

#include <stdio.h>
#include "../../inc/definitions.h"

/**
 * @brief Loader Status Codes.
 * Used to indicate the result of the loading process.
 */
typedef enum {
	LOAD_SUCCESS = 0,     /**< Program loaded successfully */
	LOAD_FILE_ERROR = 1,  /**< Error opening the file */
	LOAD_MEMORY_ERROR = 2 /**< Error writing to memory */
} LoadStatus_t;

/**
 * @brief Program information obtained from input file.
 */
typedef struct {
	int _start;             /**< Program start line */
	int wordCount;          /**< Number of words in the program */
	char programName[256];  /**< Name of the program */
	LoadStatus_t status;    /**< Loading status code */
} ProgramInfo_t;

/**
 * @brief Obtains program's word at a specific position in the input file.
 *
 * @param file The file pointer to read from
 */
word readProgramWord(FILE* file);

/**
 * @brief Obtains program's start line, quantity of words and name from input file.
 *
 * @param filePath The path to the program's input file
 */
ProgramInfo_t loadProgram(char* filePath);

#endif // LOADER_H
