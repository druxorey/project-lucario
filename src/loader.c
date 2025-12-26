#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/loader.h"

CPU_t CPU;
word RAM[RAM_SIZE];
static pthread_mutex_t program_mutex = PTHREAD_MUTEX_INITIALIZER;

//Mock function for writing in memory
int writeMemory(address addr, word value) {
    if (addr >= OS_RESERVED_SIZE && addr < RAM_SIZE) {
        RAM[addr] = value;
        return 0;
    }
    return 1;
}

word readProgramWord(FILE* filePtr) {
    word w = 0;
    fscanf(filePtr, "%d", &w);
    return w;
}

ProgramInfo_t loadProgram(char* filePath) {
    ProgramInfo_t programInfo = {0};
    CPU.PSW.mode = MODE_USER;
    FILE* programFile = fopen(filePath, "r");
    
    if (programFile) {
        fscanf(programFile, "%*s %d", &programInfo._start);
        fscanf(programFile, "%*s %d", &programInfo.wordCount);
        fscanf(programFile, "%*s %s", programInfo.programName);

        CPU.PSW.mode = MODE_KERNEL;
        
        for (int i = 0 ; i < programInfo.wordCount; i++) {
            pthread_mutex_lock(&program_mutex);
            if (writeMemory(OS_RESERVED_SIZE + i, readProgramWord(programFile)) != 0) {
                ProgramInfo_t programInfoError = {0};
                programInfoError.status = LOAD_MEMORY_ERROR;
                return programInfoError;
            }
            pthread_mutex_unlock(&program_mutex);
        }

        CPU.RB = OS_RESERVED_SIZE;
        CPU.RL = OS_RESERVED_SIZE + programInfo.wordCount;
        CPU.PSW.pc = OS_RESERVED_SIZE + programInfo._start - 1;

        programInfo.status = LOAD_SUCCESS;

        fclose(programFile);
    } else {
        ProgramInfo_t programInfoError = {0};
        programInfoError.status = LOAD_FILE_ERROR;
        return programInfoError;
    }
    return programInfo;
}