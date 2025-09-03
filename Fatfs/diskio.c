/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */
#include "spi_flash.h"
#include "usb_media_common.h"


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	// DSTATUS stat;
	// int result;

	// switch (pdrv) {
	// case DEV_RAM :
	// 	result = RAM_disk_status();

	// 	// translate the reslut code here

	// 	return stat;

	// case DEV_MMC :
	// 	result = MMC_disk_status();

	// 	// translate the reslut code here

	// 	return stat;

	// case DEV_USB :
	// 	result = USB_disk_status();

	// 	// translate the reslut code here

	// 	return stat;
	// }
	// return STA_NOINIT;
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	// DSTATUS stat;
	// int result;

	// switch (pdrv) {
	// case DEV_RAM :
	// 	result = RAM_disk_initialize();

	// 	// translate the reslut code here

	// 	return stat;

	// case DEV_MMC :
	// 	result = MMC_disk_initialize();

	// 	// translate the reslut code here

	// 	return stat;

	// case DEV_USB :
	// 	result = USB_disk_initialize();

	// 	// translate the reslut code here

	// 	return stat;
	// }
	// return STA_NOINIT;
	if(pdrv)
		return STA_NOINIT;
	else {
		SPI_FLASH_Init();
		return  0;
	}
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	UNUSED(pdrv);
	int i;

	for(i=0;i<count;i++)
	{
		if (SPI_FLASH_ReadData(sector * MEDIA_LOGICAL_SECTOR_SIZE, buff, MEDIA_LOGICAL_SECTOR_SIZE) != HAL_OK) {
			return RES_ERROR;
		}
		sector ++;
		buff += MEDIA_LOGICAL_SECTOR_SIZE;
	}
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	int i=0;
	int j =0;
	UNUSED(pdrv);

    // Use a variable for the address to make it clearer
    uint32_t addr;
    // Use a temporary pointer for the buffer
    const BYTE* current_buff_pos;

	for(i = 0; i < count; i++)
	{
        // Calculate the starting byte address for the current sector
        addr = sector * MEDIA_LOGICAL_SECTOR_SIZE;
        current_buff_pos = buff;

		SPI_FLASH_EraseSector(addr);

        // This loop now correctly writes the sector, one page at a time
		for (j = 0; j < (MEDIA_LOGICAL_SECTOR_SIZE / NOR_FLASH_PAGE_SIZE); j++)
		{
            // Call page program with the updated address and buffer
			if(SPI_FLASH_PageProgram(addr, (uint8_t*)current_buff_pos, NOR_FLASH_PAGE_SIZE) != HAL_OK) {
				return RES_ERROR;
			}

            // Move to the next page address and next part of the buffer
            addr += NOR_FLASH_PAGE_SIZE;
            current_buff_pos += NOR_FLASH_PAGE_SIZE;
		}

		sector++;
		buff += MEDIA_LOGICAL_SECTOR_SIZE;
	}

	return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  DRESULT res = RES_ERROR;

  switch (cmd)
  {
  /* Make sure that no pending write process */
  case CTRL_SYNC :
    res = RES_OK;
    break;

  /* Get number of sectors on the disk (DWORD) */
  case GET_SECTOR_COUNT :
    *(DWORD*)buff = NOR_FLASH_TOTAL_SIZE / MEDIA_LOGICAL_SECTOR_SIZE;
    res = RES_OK;
    break;

  /* Get R/W sector size (WORD) */
  case GET_SECTOR_SIZE :
    *(WORD*)buff = MEDIA_LOGICAL_SECTOR_SIZE;
    res = RES_OK;
    break;

  /* Get erase block size in unit of sector (DWORD) */
  case GET_BLOCK_SIZE :
    *(DWORD*)buff = (NOR_FLASH_BLOCK_64K_SIZE/MEDIA_LOGICAL_SECTOR_SIZE);
    res = RES_OK;
    break;

  default:
    res = RES_PARERR;
  }

  return res;
}

//FIXME: use RTC realtime here
DWORD get_fattime(void)
{
    return ((DWORD)(2025 - 1980) << 25) /* Year = 2025 */
         | ((DWORD)7 << 21)             /* Month = July */
         | ((DWORD)23 << 16)            /* Day = 23 */
         | ((DWORD)17 << 11)            /* Hour = 17 */
         | ((DWORD)36 << 5)             /* Minute = 36 */
         | ((DWORD)0 >> 1);             /* Second = 0 */
}
