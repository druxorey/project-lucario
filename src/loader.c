#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/loader.h"

FILE* programFile = NULL;
static pthread_mutex_t program_mutex = PTHREAD_MUTEX_INITIALIZER;

int* parseStart(char* filePath) {
    pthread_mutex_lock(&program_mutex);

    programFile = fopen(filePath, "r");
    int* _start = (int*) malloc(sizeof(int));
    *_start = -1;
    if (programFile != NULL) {
        fscanf(programFile, "%*s %d", _start);
    }
    fclose(programFile);
    
    pthread_mutex_unlock(&program_mutex);
    return _start;
}

int* parseWordCount(char* filePath) {
    pthread_mutex_lock(&program_mutex);
    
    programFile = fopen(filePath, "r");
    int* wordCount = (int*) malloc(sizeof(int));
    *wordCount = -1;
    if (programFile != NULL) {
        fscanf(programFile, "%*s %*d");
        fscanf(programFile, "%*s %d", wordCount);
    }
    fclose(programFile);

    pthread_mutex_unlock(&program_mutex);
    return wordCount;
}

char* parseProgramName(char* filePath) {
    pthread_mutex_lock(&program_mutex);
    
    programFile = fopen(filePath, "r");
    char* programName = (char*) malloc(100 * sizeof(char));
    programName[0] = '\0';
    if (programFile != NULL) {
        fscanf(programFile, "%*s %*d");
        fscanf(programFile, "%*s %*d");
        fscanf(programFile, "%*s %s", programName);
    }
    fclose(programFile);
    
    pthread_mutex_unlock(&program_mutex);
    return programName;
}