
#include "spi_flash.h"
#include "main.h"

#ifndef SPI_FLASH_DMA_ENABLE /* NORMAL SPI*/
HAL_StatusTypeDef SPI_FLASH_ReadStatus(uint8_t *status)
{
	HAL_StatusTypeDef ret;
    uint8_t cmd = MX25V16066M2I02_READ_STATUS_CMD;
    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY);
    ret = HAL_SPI_Receive (&hspi3, status, 1, HAL_MAX_DELAY);
    FLASH_CS_HIGH();
    return ret;
}

static HAL_StatusTypeDef SPI_FLASH_WaitForWriteCompletion(void)
{
    uint8_t status = 0;
    HAL_StatusTypeDef ret;
    do
    {
        ret = SPI_FLASH_ReadStatus(&status);
        if (ret != HAL_OK)
        {
            return ret;
        }
    } while ((status & 0x01) == 0x01);

    return HAL_OK;
}

void SPI_FLASH_Init(void)
{
    FLASH_CS_HIGH();
}


void SPI_FLASH_WriteEnable(void)
{
    uint8_t cmd = MX25V16066M2I02_WRITE_ENABLE_CMD;
    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY);
    FLASH_CS_HIGH();
}

 /**
   * @brief  Erases a single 4K-byte sector of the flash memory.
   * @param  addr: Any address within the 4K-byte sector to be erased.
   * @retval HAL_StatusTypeDef: HAL status of the operation.
   */
HAL_StatusTypeDef SPI_FLASH_EraseSector(uint32_t addr)
{
    /* Prepare the command buffer: command + 3 address bytes. */
    uint8_t cmd[4] = { MX25V16066M2I02_SECTOR_ERASE_CMD,
                    (uint8_t)(addr >> 16),
                    (uint8_t)(addr >>  8),
                    (uint8_t)(addr >>  0) };


    SPI_FLASH_WriteEnable();

    FLASH_CS_LOW();

    HAL_SPI_Transmit(&hspi3, cmd, 4, HAL_MAX_DELAY);

    FLASH_CS_HIGH();

    return SPI_FLASH_WaitForWriteCompletion();
}

HAL_StatusTypeDef SPI_FLASH_EraseBlock64k(uint32_t addr)
{
    uint8_t cmd[4] = { MX25V16066M2I02_BLOCK_64K_ERASE_CMD,
                        (uint8_t)(addr >> 16),
                        (uint8_t)(addr >>  8),
                        (uint8_t)(addr >>  0) };
    SPI_FLASH_WriteEnable();
    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi3, cmd, 4, HAL_MAX_DELAY);
    FLASH_CS_HIGH();
    return SPI_FLASH_WaitForWriteCompletion();
}

HAL_StatusTypeDef SPI_FLASH_PageProgram(uint32_t addr, uint8_t *buf, uint32_t len)
{
HAL_StatusTypeDef ret =0;
    uint8_t cmd[4] = { MX25V16066M2I02_PAGE_PROGRAM_CMD,
                    (uint8_t)(addr >> 16),
                    (uint8_t)(addr >>  8),
                    (uint8_t)(addr >>  0) };
    SPI_FLASH_WriteEnable();
    FLASH_CS_LOW();
    ret = HAL_SPI_Transmit(&hspi3, cmd, 4, HAL_MAX_DELAY);
    if(ret == HAL_OK)
    {
        ret = HAL_SPI_Transmit(&hspi3, buf, len, HAL_MAX_DELAY);
    }
    FLASH_CS_HIGH();
    if (ret != HAL_OK) return ret;

    return SPI_FLASH_WaitForWriteCompletion();
}

HAL_StatusTypeDef SPI_FLASH_ReadData(uint32_t addr, uint8_t *buf, uint32_t len)
{
HAL_StatusTypeDef ret = 0;
    uint8_t cmd[4] = { MX25V16066M2I02_READ_DATA_CMD,
                        (uint8_t)(addr >> 16),
                        (uint8_t)(addr >>  8),
                        (uint8_t)(addr >>  0) };
    FLASH_CS_LOW();
    ret = HAL_SPI_Transmit(&hspi3, cmd, 4, HAL_MAX_DELAY);
    if(ret == HAL_OK)
    {
        ret = HAL_SPI_Receive (&hspi3, buf, len, HAL_MAX_DELAY);
    }
    FLASH_CS_HIGH();
    return ret;
}

void SPI_FLASH_EraseChip(void)
{
    uint8_t cmd = MX25V16066M2I02_CHIP_ERASE_CMD; // Chip Erase command (0xC7 or 0x60 are common)

    SPI_FLASH_WriteEnable();
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

    SPI_FLASH_WaitForWriteCompletion();
}
#else
/* DMA SPI FLASH */
// This flag signals that a DMA transfer has completed.
static volatile uint8_t spi_dma_done = 0;
/**
  * @brief  Reads the flash status register. Uses blocking SPI for speed.
  */
 HAL_StatusTypeDef SPI_FLASH_ReadStatus(uint8_t *status)
{
    HAL_StatusTypeDef ret;
    uint8_t cmd = MX25V16066M2I02_READ_STATUS_CMD;
    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY);
    ret = HAL_SPI_Receive(&hspi3, status, 1, HAL_MAX_DELAY);
    FLASH_CS_HIGH();
    return ret;
}

/**
  * @brief  Waits for the current write or erase operation to complete.
  */
static HAL_StatusTypeDef SPI_FLASH_WaitForWriteCompletion(void)
{
    uint8_t status = 0;
    HAL_StatusTypeDef ret;
    do
    {
        // This function is now much faster as SPI_FLASH_ReadStatus is blocking
        ret = SPI_FLASH_ReadStatus(&status);
        if (ret != HAL_OK) {
            return ret;
        }
        // Could add a small delay here in an RTOS environment
    } while ((status & 0x01) == 0x01); // Wait for WIP (Write-In-Progress) bit to clear

    return HAL_OK;
}

void SPI_FLASH_Init(void)
{
    FLASH_CS_HIGH();
}

/**
  * @brief  Sends the Write Enable (WREN) command. Uses blocking SPI for speed.
  */
void SPI_FLASH_WriteEnable(void)
{
    uint8_t cmd = MX25V16066M2I02_WRITE_ENABLE_CMD;
    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY);
    FLASH_CS_HIGH();
}

/**
  * @brief  Erases a 4KB sector.
  */
HAL_StatusTypeDef SPI_FLASH_EraseSector(uint32_t addr)
{
    uint8_t cmd[4] = { MX25V16066M2I02_SECTOR_ERASE_CMD, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
    SPI_FLASH_WriteEnable();
    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi3, cmd, 4, HAL_MAX_DELAY); // Use blocking for small command
    FLASH_CS_HIGH();
    return SPI_FLASH_WaitForWriteCompletion();
}

/**
  * @brief  Erases a 64KB block.
  */
HAL_StatusTypeDef SPI_FLASH_EraseBlock64k(uint32_t addr)
{
    uint8_t cmd[4] = { MX25V16066M2I02_BLOCK_64K_ERASE_CMD, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
    SPI_FLASH_WriteEnable();
    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi3, cmd, 4, HAL_MAX_DELAY); // Use blocking for small command
    FLASH_CS_HIGH();
    return SPI_FLASH_WaitForWriteCompletion();
}

/**
  * @brief  Erases the entire chip.
  */
void SPI_FLASH_EraseChip(void)
{
    uint8_t cmd = MX25V16066M2I02_CHIP_ERASE_CMD;
    SPI_FLASH_WriteEnable();
    FLASH_CS_LOW();
    HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY); // Use blocking for small command
    FLASH_CS_HIGH();
    SPI_FLASH_WaitForWriteCompletion();
}


/**
  * @brief  Writes a page of data to the flash.
  * Uses blocking SPI for the command and DMA for the data payload.
  */
HAL_StatusTypeDef SPI_FLASH_PageProgram(uint32_t addr, uint8_t *buf, uint32_t len)
{
    HAL_StatusTypeDef ret;
    uint8_t cmd[4] = { MX25V16066M2I02_PAGE_PROGRAM_CMD, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };

    SPI_FLASH_WriteEnable();

    FLASH_CS_LOW();

    // 1. Transmit command using blocking SPI (it's small and fast)
    if (HAL_SPI_Transmit(&hspi3, cmd, 4, HAL_MAX_DELAY) != HAL_OK)
    {
        FLASH_CS_HIGH();
        return HAL_ERROR;
    }

    // 2. Transmit data using DMA for high performance
    spi_dma_done = 0;
    ret = HAL_SPI_Transmit_DMA(&hspi3, buf, len);
    if (ret != HAL_OK)
    {
        FLASH_CS_HIGH();
        return ret;
    }

    // 3. Wait for DMA transfer to finish
    while (spi_dma_done == 0) {}

    FLASH_CS_HIGH();

    // 4. Wait for the flash chip's internal write process to complete
    return SPI_FLASH_WaitForWriteCompletion();
}

/**
  * @brief  Reads data from the flash.
  * Uses blocking SPI for the command and DMA for the data payload.
  */
HAL_StatusTypeDef SPI_FLASH_ReadData(uint32_t addr, uint8_t *buf, uint32_t len)
{
    HAL_StatusTypeDef ret;
    uint8_t cmd[4] = { MX25V16066M2I02_READ_DATA_CMD, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };

    FLASH_CS_LOW();

    // 1. Transmit command using blocking SPI
    if (HAL_SPI_Transmit(&hspi3, cmd, 4, HAL_MAX_DELAY) != HAL_OK)
    {
        FLASH_CS_HIGH();
        return HAL_ERROR;
    }

    // 2. Receive data using DMA
    spi_dma_done = 0;
    ret = HAL_SPI_Receive_DMA(&hspi3, buf, len);
     if (ret != HAL_OK)
    {
        FLASH_CS_HIGH();
        return ret;
    }

    // 3. Wait for DMA transfer to complete
    while (spi_dma_done == 0) {}

    FLASH_CS_HIGH();

    return ret;
}

/* --- HAL DMA Callback Functions --- */

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI3)
  {
    spi_dma_done = 1;
  }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI3)
  {
    spi_dma_done = 1;
  }
}

/**
  * @brief  SPI error callback.
  * @param  hspi: SPI handle
  * @retval None
  */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    // Handle or log the error
    Error_Handler();
}
#ifdef SPI_DMA_TEST

#define TEST_SECTOR_ADDRESS   0x10000
#define TEST_DATA_SIZE        256   // Test with one full page of data.

static void Print_Test_Buffer(const char *title, uint8_t *buffer, uint32_t size)
{
    printf("--- %s (%lu bytes) ---\r\n", title, size);
    for (uint32_t i = 0; i < size; i++)
    {
        if (i > 0 && i % 16 == 0) printf("\r\n");
        if (i % 16 == 0) printf("  ");
        printf("%02X ", buffer[i]);
    }
    printf("\r\n---------------------------\r\n");
}

/**
  * @brief  Runs a comprehensive test on the DMA-based SPI flash driver.
  * @param  None
  * @retval None
  */
void Test_SPI_DMA_Flash(void)
{
    HAL_StatusTypeDef status;
    // Use word-aligned buffers for better DMA performance
    uint8_t write_buffer[TEST_DATA_SIZE] __attribute__((aligned(4)));
    uint8_t read_buffer[TEST_DATA_SIZE] __attribute__((aligned(4)));
    int i;

    printf("\r\n--- Starting SPI Flash DMA Driver Test ---\r\n");

    printf("1. Erasing 4KB sector at address 0x%08X...\r\n", (unsigned int)TEST_SECTOR_ADDRESS);
    status = SPI_FLASH_EraseSector(TEST_SECTOR_ADDRESS);
    if (status != HAL_OK)
    {
        printf("   ERASE FAILED! HAL Status: %d\r\n", status);
        return;
    }
    printf("   Erase successful.\r\n");

    printf("2. Writing %d bytes of test data via DMA...\r\n", TEST_DATA_SIZE);
    for (i = 0; i < TEST_DATA_SIZE; i++)
    {
        write_buffer[i] = (uint8_t)(i % 256); // Fill buffer with a predictable pattern: 0, 1, 2...
    }

    status = SPI_FLASH_PageProgram(TEST_SECTOR_ADDRESS, write_buffer, TEST_DATA_SIZE);
    if (status != HAL_OK)
    {
        printf("   WRITE FAILED! HAL Status: %d\r\n", status);
        return;
    }
    printf("   Write successful.\r\n");

    printf("3. Reading back %d bytes via DMA...\r\n", TEST_DATA_SIZE);
    memset(read_buffer, 0, sizeof(read_buffer)); // Clear read buffer first
    status = SPI_FLASH_ReadData(TEST_SECTOR_ADDRESS, read_buffer, TEST_DATA_SIZE);
    if (status != HAL_OK)
    {
        printf("   READ FAILED! HAL Status: %d\r\n", status);
        return;
    }
    printf("   Read successful.\r\n");

    printf("4. Verifying data integrity...\r\n");
    if (memcmp(write_buffer, read_buffer, TEST_DATA_SIZE) == 0)
    {
        printf("   SUCCESS! Data read back matches data written. DMA driver is working.\r\n");
    }
    else
    {
        printf("   !!! FAILURE! Data mismatch detected !!!\r\n");
        Print_Test_Buffer("Data Sent (Expected)", write_buffer, 32);
        Print_Test_Buffer("Data Received (Got)", read_buffer, 32);
    }

    printf("--- SPI Flash DMA Driver Test Finished ---\r\n");
}

#endif

#endif