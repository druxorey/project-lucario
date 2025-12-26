#ifndef MOCK_MEMORY_H
#define MOCK_MEMORY_H

#include "../inc/definitions.h"

//Mock function for writing in memory
int writeMemory(address addr, word value);

//Mock function for reading from memory
word readProgramWord(FILE* filePtr);

#endif // LOADER_H
