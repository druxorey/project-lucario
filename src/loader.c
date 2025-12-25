#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/loader.h"

static pthread_mutex_t program_mutex = PTHREAD_MUTEX_INITIALIZER;

int* parseStart(char* filePath) {
    FILE* programFile = fopen(filePath, "r");
    int* _start = (int*) malloc(sizeof(int));
    *_start = -1;

    if (programFile) {
        fscanf(programFile, "%*s %d", _start);
        fclose(programFile);
    }

    return _start;
}

int* parseWordCount(char* filePath) {
    FILE* programFile = fopen(filePath, "r");
    int* wordCount = (int*) malloc(sizeof(int));
    *wordCount = -1;
    
    if (programFile) {
        fscanf(programFile, "%*s %*d");
        fscanf(programFile, "%*s %d", wordCount);
        fclose(programFile);
    }

    return wordCount;
}

char* parseProgramName(char* filePath) {
    FILE* programFile = fopen(filePath, "r");
    char* programName = (char*) malloc(100 * sizeof(char));
    programName[0] = '\0';
    
    if (programFile) {
        fscanf(programFile, "%*s %*d");
        fscanf(programFile, "%*s %*d");
        fscanf(programFile, "%*s %s", programName);
        fclose(programFile);
    }
    
    return programName;
}

// Must secure file pointer using mutex before calling this function to avoid race conditions
// Line numbers are 1-based
void positionInLine(FILE* filePtr, int lineNumber) {
    fseek(filePtr, 0, SEEK_SET);
    char buffer[256];
    for (int i = 1; i < lineNumber + 3; i++) {
        if (fgets(buffer, sizeof(buffer), filePtr) == NULL) {
            return;
        }
    }
}

word readProgramWord(FILE* filePtr) {
    word w = 0;
    fscanf(filePtr, "%d", &w);
    return w;
}