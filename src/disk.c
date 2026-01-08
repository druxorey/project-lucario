#include <stdbool.h>

#include "../inc/disk.h"
#include "../inc/logger.h"
#include "../inc/definitions.h"

Sector_t DISK[DISK_TRACKS][DISK_CYLINDERS][DISK_SECTORS];

DiskStatus_t readSector(uint8_t track, uint8_t cylinder, uint8_t sector, Sector_t* buffer){
	if (track < 0 || track >= DISK_TRACKS || cylinder < 0 || cylinder >= DISK_CYLINDERS || sector < 0 || sector >= DISK_SECTORS) {
		loggerLog(LOG_ERROR, "Disk Read Error: Sector out of bounds");
		*buffer = (Sector_t){0}; // Clear buffer on error
		return DISK_ERR_OUT_OF_BOUNDS;
	}
	*buffer = DISK[track][cylinder][sector];
	return DISK_SUCCESS;
}


DiskStatus_t writeSector(uint8_t track, uint8_t cylinder, uint8_t sector, Sector_t data) {
	if (track < 0 || track >= DISK_TRACKS || cylinder < 0 || cylinder >= DISK_CYLINDERS || sector < 0 || sector >= DISK_SECTORS) {
		loggerLog(LOG_ERROR, "Disk Write Error: Sector out of bounds");
		return DISK_ERR_OUT_OF_BOUNDS;
	}
	DISK[track][cylinder][sector] = data;
	return DISK_SUCCESS;
}
