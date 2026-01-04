/**
 * @file disk.h
 * @brief Disk simulation interface.
 *
 * This file declares the functions for reading from and writing to
 * a simulated disk.
 *
 * @version 2.0
 */

#ifndef DISK_H
#define DISK_H
#include "../inc/definitions.h"

/**
 * @brief Status codes for disk operations.
 * Replaces generic integers for better type safety and readability.
 */
typedef enum {
    DISK_SUCCESS           = 0, /**< Operation completed successfully. */
    DISK_ERR_OUT_OF_BOUNDS = 1  /**< Disk Error: Track/Cylinder/Sector out of bounds. */
} DiskStatus_t;

/**
 * @brief Reads a sector from the disk into the provided buffer.
 *
 * @param track The track number to read from.
 * @param cylinder The cylinder number to read from.
 * @param sector The sector number to read from.
 * @param buffer Pointer to a Sector_t structure where the data will be stored.
 */
DiskStatus_t readSector(int track, int cylinder, int sector, Sector_t* buffer);


/**
 * @brief Writes data to a specific sector on the disk.
 *
 * @param track The track number to write to.
 * @param cylinder The cylinder number to write to.
 * @param sector The sector number to write to.
 * @param data The Sector_t structure containing the data to write.
 */
DiskStatus_t writeSector(int track, int cylinder, int sector, Sector_t data);

#endif // DISK_H