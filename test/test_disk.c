#include <stdbool.h>

#include "../lib/utest.h"
#include "../inc/disk.h"

Sector_t testData = {0};
Sector_t buffer = {0};
int track, cylinder, sector;

UTEST_MAIN();

// Verify that reading/writing works on the last valid sector (bounds check).
UTEST(Disk, ReadWriteMaxBounds) {
    track = DISK_TRACKS - 1;
    cylinder = DISK_CYLINDERS - 1;
    sector = DISK_SECTORS - 1;
    testData.data = 987654321;

    writeSector(track, cylinder, sector, testData);
    readSector(track, cylinder, sector, &buffer);

    EXPECT_EQ(testData.data, buffer.data);
}

// Verify that writing and reading a sector works correctly.
UTEST(Disk, ReadWriteSector) {
    track = 2;
    cylinder = 3;
    sector = 4;
    testData.data = 123456789;

    writeSector(track, cylinder, sector, testData);
    readSector(track, cylinder, sector, &buffer);

    EXPECT_EQ(testData.data, buffer.data);
}

// Verify that reading from an out-of-bounds sector is handled gracefully (bounds check).
UTEST(Disk, ReadOutOfBoundsSector) {
    track = -1;
    cylinder = 3;
    sector = 4;

    readSector(track, cylinder, sector, &buffer); // Invalid track

    EXPECT_EQ(buffer.data, 0); // Buffer should be cleared on error
    EXPECT_EQ(readSector(track, cylinder, sector, &buffer), DISK_ERR_OUT_OF_BOUNDS); // Check error code
}

// Verify that writing to an out-of-bounds sector is handled gracefully.
UTEST(Disk, WriteOutOfBoundsSector) {
    track = DISK_TRACKS;
    cylinder = 3;
    sector = 4;
    testData.data = 555555555;

    writeSector(track, cylinder, sector, testData); // Invalid track

    EXPECT_EQ(writeSector(track, cylinder, sector, testData), DISK_ERR_OUT_OF_BOUNDS); // Check error code
}

