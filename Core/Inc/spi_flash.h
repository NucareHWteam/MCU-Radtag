#ifndef INC_SPI_FLASH_H_
#define INC_SPI_FLASH_H_

/*
 * spi_flash.h
 *
 *  Created on: May 28, 2025
 *      Author: korea
 */
#include "main.h"
#define LOG_PAGE_SIZE    256            /* 256 B */

//Comment this define if not use SPI DMA Transfer.
#define SPI_FLASH_DMA_ENABLE

//Comment this define if not use DMA check read/write.
//#define  SPI_DMA_TEST


#define MX25V16066M2I02_READ_STATUS_CMD 0x05
#define MX25V16066M2I02_WRITE_ENABLE_CMD 0x06
#define MX25V16066M2I02_SECTOR_ERASE_CMD 0x20
#define MX25V16066M2I02_BLOCK_64K_ERASE_CMD 0xD8
#define MX25V16066M2I02_PAGE_PROGRAM_CMD 0x02
#define MX25V16066M2I02_READ_DATA_CMD 0x03
#define MX25V16066M2I02_CHIP_ERASE_CMD 0x60


#define FLASH_CS_LOW()   HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET)
#define FLASH_CS_HIGH()  HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET)


extern SPI_HandleTypeDef hspi3;

void SPI_FLASH_Init(void);

void SPI_FLASH_WriteEnable(void);

HAL_StatusTypeDef SPI_FLASH_EraseSector(uint32_t addr);

HAL_StatusTypeDef SPI_FLASH_EraseBlock64k(uint32_t addr);

HAL_StatusTypeDef SPI_FLASH_PageProgram(uint32_t addr, uint8_t *buf, uint32_t len);

HAL_StatusTypeDef SPI_FLASH_ReadData(uint32_t addr, uint8_t *buf, uint32_t len);

HAL_StatusTypeDef SPI_FLASH_ReadStatus(uint8_t *status);

void SPI_FLASH_EraseChip(void);

/** @brief 부팅 시 Flash 스캔 후 이어쓰기 오프셋 복원 */
void meas_data_log_init(void);

 /** @brief Flash에 측정 데이터를 순차 기록 */
 void meas_data_log_write(const uint8_t *data, size_t len);


#endif /* INC_SPI_FLASH_H_ */
