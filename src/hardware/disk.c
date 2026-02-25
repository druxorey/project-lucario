#include <stdbool.h>

#include "../../inc/definitions.h"
#include "../../inc/logger.h"
#include "../../inc/hardware/disk.h"

Sector_t DISK[DISK_TRACKS][DISK_CYLINDERS][DISK_SECTORS];

DiskStatus_t readSector(uint8_t track, uint8_t cylinder, uint8_t sector, Sector_t* buffer){
	if (track >= DISK_TRACKS || cylinder >= DISK_CYLINDERS || sector >= DISK_SECTORS) {
		loggerLogHardware(LOG_ERROR, "Disk Read Error: Sector out of bounds");
		*buffer = (Sector_t){0}; // Clear buffer on error
		return DISK_ERR_OUT_OF_BOUNDS;
	}
	*buffer = DISK[track][cylinder][sector];
	loggerLogHardware(LOG_INFO, "Disk Read: Sector read successfully");
	return DISK_SUCCESS;
}


DiskStatus_t writeSector(uint8_t track, uint8_t cylinder, uint8_t sector, Sector_t data) {
	if (track >= DISK_TRACKS || cylinder >= DISK_CYLINDERS || sector >= DISK_SECTORS) {
		loggerLogHardware(LOG_ERROR, "Disk Write Error: Sector out of bounds");
		return DISK_ERR_OUT_OF_BOUNDS;
	}
	DISK[track][cylinder][sector] = data;
	loggerLogHardware(LOG_INFO, "Disk Write: Sector written successfully");
	return DISK_SUCCESS;
}
