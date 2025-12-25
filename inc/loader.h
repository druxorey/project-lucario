#ifndef LOADER_H
#define LOADER_H

#include "../inc/definitions.h"

/**
 * @brief Obtains program's start line, quantity of words and name from input file.
 * 
 * @param filePath The path to the program's input file
 */
void parseInputFile(char* filePath);

/**
 * @brief Obtains program's start line from input file.
 * 
 * @param filePath The path to the program's input file
 */
int* parseStart(char* filePath);

/**
 * @brief Obtains program's quantity of words from input file.
 * 
 * @param filePath The path to the program's input file
 */
int* parseWordCount(char* filePath);

/**
 * @brief Obtains program's name from input file.
 * 
 * @param filePath The path to the program's input file
 */
char* parseProgramName(char* filePath);

#endif // LOADER_H