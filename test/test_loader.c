#include "../lib/utest.h"
#include "../inc/loader.h"

UTEST_MAIN();

// Auxiliary function to create a test input file
void createTestInputFile(void) {
    FILE* f = fopen("../test_program.txt", "r");
    if (f != NULL) {
        fclose(f);
    } else {
        f = fopen("../test_program.txt", "w");
        if (f != NULL) {
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

UTEST(Loader, FileCreation) {
    createTestInputFile();
    FILE* f = fopen("../test_program.txt", "r");
    ASSERT_TRUE(f != NULL);
    if (f) fclose(f);
}

UTEST(Loader, ParseStartLine) {
    createTestInputFile();
    int* start = parseStart("../test_program.txt");
    ASSERT_EQ(*start, 1);
    free(start);
}

UTEST(Loader, ParseWordCount) {
    createTestInputFile();
    int* wordCount = parseWordCount("../test_program.txt");
    ASSERT_EQ(*wordCount, 7);
    free(wordCount);
}

UTEST(Loader, ParseProgramName) {
    createTestInputFile();
    char* programName = parseProgramName("../test_program.txt");
    ASSERT_STREQ(programName, "5mas5");
    free(programName);
    programName = NULL;
}