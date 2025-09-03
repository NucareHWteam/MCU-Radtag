/*
 * usb_media_common.h
 *
 *  Created on: Jul 10, 2025
 *      Author: dongo
 */

#ifndef INC_USB_MEDIA_COMMON_H_
#define INC_USB_MEDIA_COMMON_H_

/**************************************************************************/
// MEDIA OPTIONS
/**************************************************************************/
// Enable the LevelX NOR driver for FileX
// if you are using a different driver, comment this out.
// If use levelx should be decline the logical sector size below 1024 bytes
// #define FX_ENABLE_LEVELX_NOR_DRIVER

/**************************************************************************/
// NOR FLASH MX25V16066 INFORMATION
/**************************************************************************/
#define NOR_FLASH_SECTOR_SIZE                   4096    // Smallest erasable unit is 4KB
#define NOR_FLASH_PAGE_SIZE                     256     // Smallest programmable unit is 256B
//FIXME: 2MB for test, should be change to 1MB.
#define NOR_FLASH_TOTAL_SIZE                    (1 * 1024 * 1024) // 1MB
#define NOR_FLASH_BLOCK_64K_SIZE               (64*1024)   // Block size is 64KB
#define NOR_FLASH_BLOCK_32K_SIZE               (32*1024)   // Block size is 32KB
#define NOR_FLASH_BASE_ADDRESS                  0x00000000 // Base address of the NOR flash in memory map
#define NOR_FLASH_NUMBER_OF_BLOCKS_64K         (NOR_FLASH_TOTAL_SIZE / NOR_FLASH_BLOCK_64K_SIZE) // Number of 64KB blocks


/**************************************************************************/
// MEDIA INFORMATION
/**************************************************************************/
// Incase of using LevelX NOR driver, the logical sector size must match with the file system sector size
// Shoud check the LX_NOR_SECTOR_SIZE defined in the driver header file
#define MEDIA_LOGICAL_SECTOR_SIZE               (1024*4)

#define MEDIA_LOGICAL_CLUSTER_SIZE              (16*1024)
#define MEDIA_SIZE                              (NOR_FLASH_TOTAL_SIZE) // 2MB
#define MEDIA_TOTAL_LOGICAL_SECTORS             (MEDIA_SIZE / MEDIA_LOGICAL_SECTOR_SIZE)
#define MEDIA_LOGICAL_SECTOR_PER_CLUSTER        (MEDIA_LOGICAL_CLUSTER_SIZE / MEDIA_LOGICAL_SECTOR_SIZE)
#define MEDIA_NAME                              "RAD TAG"
#define MEDIA_NUMBER_OF_FATS                    2
#define MEDIA_DIRECTORY_ENTRIES                 512
#define MEDIA_HIDDEN_SECTORS                    0
#define MEDIA_HEADS                             64
#define MEDIA_SECTORS_PER_TRACK                 32


#endif /* INC_USB_MEDIA_COMMON_H_ */
