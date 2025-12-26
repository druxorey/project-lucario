#include "../lib/utest.h"
#include "../inc/loader.h"

UTEST_MAIN();

// Auxiliary function to create a test input file
void createTestInputFile(void) {
    FILE* f = fopen("../test_program.txt", "r");
    if (f) {
        fclose(f);
    } else {
        f = fopen("../test_program.txt", "w");
        if (f) {
            fprintf(f, "_start 1\n");
            fprintf(f, ".NumeroPalabras 7\n");
            fprintf(f, ".NombreProg 5mas5\n");
            fprintf(f, "04100005\n");
            fprintf(f, "00100005\n");
            fprintf(f, "25000000\n");
            fprintf(f, "04100007\n");
            fprintf(f, "13000000\n");
            fprintf(f, "04100007\n");
            fprintf(f, "13000000");
            fclose(f);
        }
    }
}

// Auxilary function to create a test input file with more words than memory can hold
void createLargeTestInputFile(void) {
    FILE* f = fopen("../large_test_program.txt", "r");
    if (f) {
        fclose(f);
    } else { 
        f = fopen("../large_test_program.txt", "w");
        fprintf(f, "_start 1\n");
        fprintf(f, ".NumeroPalabras %d\n", RAM_SIZE + 1);
        fprintf(f, ".NombreProg LargeProg\n");
        for (int i = 1; i < RAM_SIZE + 1; i++) {
            fprintf(f, "04100005\n");
        }
        fclose(f);
    }
}

// Verify that the test input file is created correctly
UTEST(Loader, FileCreation) {
    createTestInputFile();
    FILE* f = fopen("../test_program.txt", "r");
    ASSERT_TRUE(f != NULL);
    if (f) fclose(f);
}

// Verify that the large test input file is created correctly
UTEST(Loader, LargeFileCreation) {
    createLargeTestInputFile();
    FILE* f = fopen("../large_test_program.txt", "r");
    ASSERT_TRUE(f != NULL);
    if (f) fclose(f);
}

// Verify that readProgramWord reads the correct word from the file
UTEST(Loader, ReadProgramWord) {
    createTestInputFile();
    FILE* f = fopen("../test_program.txt", "r");
    ASSERT_TRUE(f != NULL);
    if (f) {
        fscanf(f, "%*s %*d");
        fscanf(f, "%*s %*d");
        fscanf(f, "%*s %*s");
        word w = readProgramWord(f);
        ASSERT_EQ(w, 4100005);
        fclose(f);
    }
}

// Verify that loadProgram loads the program correctly
UTEST(Loader, LoadProgram) {
    createTestInputFile();
    ProgramInfo_t programInfo = loadProgram("../test_program.txt");
    ASSERT_EQ(programInfo._start, 1);
    ASSERT_EQ(programInfo.wordCount, 7);
    ASSERT_STREQ(programInfo.programName, "5mas5");
    ASSERT_EQ(programInfo.status, LOAD_SUCCESS);
}

// Verify that loadProgram handles file not found error
UTEST(Loader, LoadProgramFileError) {
    ProgramInfo_t programInfo = loadProgram("../non_existent_file.txt");
    ASSERT_EQ(programInfo._start, 0);
    ASSERT_EQ(programInfo.wordCount, 0);
    ASSERT_STREQ(programInfo.programName, "");
    ASSERT_EQ(programInfo.status, LOAD_FILE_ERROR);
}

// Verify that loadProgram handles memory error when program is too large
UTEST(Loader, LoadProgramMemoryError) {
    createLargeTestInputFile();
    ProgramInfo_t programInfo = loadProgram("../large_test_program.txt");
    ASSERT_EQ(programInfo._start, 0);
    ASSERT_EQ(programInfo.wordCount, 0);
    ASSERT_STREQ(programInfo.programName, "");
    ASSERT_EQ(programInfo.status, LOAD_MEMORY_ERROR);
}