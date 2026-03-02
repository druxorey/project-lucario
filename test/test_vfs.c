#include <stdbool.h>
#include <stdio.h>

#include "../lib/utest.h"
#include "../inc/hardware/memory.h"
#include "../inc/kernel/vfs.h"
#include "../inc/hardware/disk.h"

CPU_t CPU;
word RAM[RAM_SIZE];
Sector_t DISK[DISK_TRACKS][DISK_CYLINDERS][DISK_SECTORS];

UTEST_MAIN();

// Mock function for reading from disk
DiskStatus_t writeSector(uint8_t track, uint8_t cylinder, uint8_t sector, Sector_t data) {
	DISK[track][cylinder][sector] = data;
	return DISK_SUCCESS;
}

// Mock function for writing in memory
MemoryStatus_t writeMemory(address addr, word value) {
	if (addr >= OS_RESERVED_SIZE && addr < RAM_SIZE) {
		RAM[addr] = value;
		return 0;
	}
	return 1;
}

// Auxiliary functions to create test input files
void createTestInputFile(void) {
	FILE* f = fopen("test_program.txt", "w");
	if (f) {
		fprintf(f, "_start 1\n");
		fprintf(f, ".NumeroPalabras 7\n");
		fprintf(f, ".NombreProg 5mas5\n");
		fprintf(f, "04100005   // Comentario\n");
		fprintf(f, "00100005\n");
		fprintf(f, "25000000\n");
		fprintf(f, "04100007\n");
		fprintf(f, "13000000\n");
		fprintf(f, "04100007\n");
		fprintf(f, "13000000\n");
		fclose(f);
	}
}

void createLargeTestInputFile(void) {
	FILE* f = fopen("large_test_program.txt", "w");
	if (f) {
		fprintf(f, "_start 1\n");
		fprintf(f, ".NumeroPalabras %d\n", RAM_SIZE + 1);
		fprintf(f, ".NombreProg LargeProg\n");
		for (int i = 1; i < RAM_SIZE + 1; i++) {
			fprintf(f, "04100005\n");
		}
		fclose(f);
	}
}

UTEST(Loader, FileCreation) {
	createTestInputFile();
	FILE* f = fopen("test_program.txt", "r");
	ASSERT_TRUE(f != NULL);
	if (f) fclose(f);
}

UTEST(Loader, LargeFileCreation) {
	createLargeTestInputFile();
	FILE* f = fopen("large_test_program.txt", "r");
	ASSERT_TRUE(f != NULL);
	if (f) fclose(f);
}

UTEST(Loader, ReadProgramWord) {
	createTestInputFile();
	FILE* f = fopen("test_program.txt", "r");
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

UTEST(Loader, LoadProgram) {
	createTestInputFile();
	ProgramInfo_t programInfo = loadProgram("test_program.txt");
	ASSERT_EQ(programInfo._start, 1);
	ASSERT_EQ(programInfo.wordCount, 7);
	ASSERT_STREQ(programInfo.programName, "5mas5");
	ASSERT_EQ(programInfo.status, (unsigned)LOAD_SUCCESS);
}

UTEST(Loader, LoadProgramFileError) {
	ProgramInfo_t programInfo = loadProgram("non_existent_file.txt");
	ASSERT_EQ(programInfo._start, 0);
	ASSERT_EQ(programInfo.wordCount, 0);
	ASSERT_STREQ(programInfo.programName, "");
	ASSERT_EQ(programInfo.status, (unsigned)LOAD_FILE_ERROR);
}

UTEST(Loader, LoadProgramMemoryError) {
	createLargeTestInputFile();
	ProgramInfo_t programInfo = loadProgram("large_test_program.txt");
	ASSERT_EQ(programInfo._start, 1);
	ASSERT_EQ(programInfo.wordCount, 2001);
	ASSERT_STREQ(programInfo.programName, "LargeProg");
	ASSERT_EQ(programInfo.status, (unsigned)LOAD_FILE_ERROR);
}

UTEST(VFS, FileExists_WhenEmpty) {
	vfsClearCatalog();
	ASSERT_FALSE(vfsFileExists("ghost.txt"));
}

UTEST(VFS, RegisterAndExists) {
	vfsClearCatalog();
	ASSERT_EQ(vfsRegisterFile("programa1.txt", 0, 1, 5, 20, 1), (unsigned)VFS_SUCCESS);
	ASSERT_TRUE(vfsFileExists("programa1.txt"));
	ASSERT_FALSE(vfsFileExists("programa2.txt"));
}

UTEST(VFS, GetMetadata_Success) {
	vfsClearCatalog();
	vfsRegisterFile("calc.txt", 2, 4, 10, 85, 1);
	
	FileMeta_t meta;
	ASSERT_EQ(vfsGetMetadata("calc.txt", &meta), (unsigned)VFS_SUCCESS);
	
	ASSERT_STREQ(meta.fileName, "calc.txt");
	ASSERT_EQ(meta.startTrack, 2);
	ASSERT_EQ(meta.startCylinder, 4);
	ASSERT_EQ(meta.startSector, 10);
	ASSERT_EQ(meta.wordCount, 85);
}

UTEST(VFS, GetMetadata_NotFound) {
	vfsClearCatalog();
	FileMeta_t meta;
	ASSERT_EQ(vfsGetMetadata("missing.txt", &meta), (unsigned)VFS_ERR_NOT_FOUND);
}

UTEST(VFS, Catalog_DiskFull) {
	vfsClearCatalog();
	for (int i = 0; i < MAX_PROCESSES; i++) {
		char tempName[32];
		sprintf(tempName, "file%d.txt", i);
		ASSERT_EQ(vfsRegisterFile(tempName, 0, 0, 0, 10, 1), (unsigned)VFS_SUCCESS);
	}
	ASSERT_EQ(vfsRegisterFile("overflow.txt", 0, 0, 0, 10, 1), (unsigned)VFS_ERR_DISK_FULL);
}

UTEST(VFS, LoadToDisk_Success) {
	vfsClearCatalog();
	createTestInputFile();
	
	VFSStatus_t status = vfsLoadToDisk("test_program.txt");
	ASSERT_EQ(status, (unsigned)VFS_SUCCESS);
	
	FileMeta_t meta;
	ASSERT_EQ(vfsGetMetadata("test_program.txt", &meta), (unsigned)VFS_SUCCESS);
	ASSERT_EQ(meta.startTrack, 0);
	ASSERT_EQ(meta.startCylinder, 0);
	ASSERT_EQ(meta.startSector, 0);
	ASSERT_EQ(meta.wordCount, 7);
	ASSERT_EQ(meta.startPC, 1);
	
	ASSERT_EQ(DISK[0][0][0].data, 4100005);
	ASSERT_EQ(DISK[0][0][1].data, 100005);
}

UTEST(VFS, LoadToDisk_NotFound) {
	vfsClearCatalog();
	VFSStatus_t status = vfsLoadToDisk("archivo_falso.txt");
	ASSERT_EQ(status, (unsigned)VFS_ERR_NOT_FOUND);
}

UTEST(VFS, DiskGeometryWrapAround) {
	vfsClearCatalog();
	
	FILE* f = fopen("geom_test.txt", "w");
	fprintf(f, "_start 1\n");
	fprintf(f, ".NumeroPalabras 105\n");
	fprintf(f, ".NombreProg GeomProg\n");
	for (int i = 0; i < 105; i++) fprintf(f, "99999999\n");
	fclose(f);
	
	ASSERT_EQ(vfsLoadToDisk("geom_test.txt"), (unsigned)VFS_SUCCESS);
	
	createTestInputFile();
	ASSERT_EQ(vfsLoadToDisk("test_program.txt"), (unsigned)VFS_SUCCESS);
	
	FileMeta_t meta2;
	ASSERT_EQ(vfsGetMetadata("test_program.txt", &meta2), (unsigned)VFS_SUCCESS);
	
	ASSERT_EQ(meta2.startTrack, 0);
	ASSERT_EQ(meta2.startCylinder, 1);
	ASSERT_EQ(meta2.startSector, 5);
}
