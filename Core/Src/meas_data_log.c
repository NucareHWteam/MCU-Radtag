#include "main.h"            // huart3, HAL_MAX_DELAY
#include "meas_data_log.h"   // ENTRY_SIZE, LOG_••• 매크로·프로토타입
#include "spi_flash.h"       // SPI_FLASH_••• 함수 프로토타입

// printf, va_list 사용

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
// UART3 핸들러 extern 선언 (main.h 에도 선언되어 있어야 합니다)
extern UART_HandleTypeDef huart3;

static uint32_t log_write_offset;



/**
 * @brief  플래시에 기록된 모든 로그 데이터를
 *         빈(0xFF) 구간 전까지 UART3로 덤프
 */
//void dump_all_flash_data(void)
//{
//    const size_t CHUNK = 64;
//    uint8_t buf[CHUNK];
//    uint32_t offset = 0;
//    const char hex[] = "0123456789ABCDEF";
//
//    while (offset < LOG_MAX_SIZE)
//    {
//        size_t to_read = (LOG_MAX_SIZE - offset > CHUNK) ? CHUNK : (LOG_MAX_SIZE - offset);
//        meas_data_log_read(offset, buf, to_read);
//
//        // 온전히 0xFF면 더 이상 기록된 데이터가 없으므로 종료
//        bool all_ff = true;
//        for (size_t i = 0; i < to_read; i++)
//        {
//            if (buf[i] != 0xFF)
//            {
//                all_ff = false;
//                break;
//            }
//        }
//        if (all_ff) break;
//
//        // inline 헥사 덤프
//        for (size_t i = 0; i < to_read; i++)
//        {
//            char hi = hex[buf[i] >> 4];
//            char lo = hex[buf[i] & 0x0F];
//            HAL_UART_Transmit(&huart3, (uint8_t*)&hi, 1, HAL_MAX_DELAY);
//            HAL_UART_Transmit(&huart3, (uint8_t*)&lo, 1, HAL_MAX_DELAY);
//            const char sp = ' ';
//            HAL_UART_Transmit(&huart3, (uint8_t*)&sp, 1, HAL_MAX_DELAY);
//        }
//        const char crlf[] = "\r\n";
//        HAL_UART_Transmit(&huart3, (uint8_t*)crlf, 2, HAL_MAX_DELAY);
//
//        offset += to_read;
//    }
//}
/**
 * @brief 부팅 시 Flash를 페이지 단위로 스캔해
 *        첫 0xFF 바이트 위치를 log_write_offset에 저장
 */
//void meas_data_log_init(void)
//{
//    uint8_t buf[ENTRY_SIZE];
//    uint32_t offset = 0;
//
//    // 0, ENTRY_SIZE, 2*ENTRY_SIZE ... 간격으로 검사
//    while (offset < LOG_MAX_SIZE)
//    {
//        meas_data_log_read(offset, buf, ENTRY_SIZE);
//        bool all_ff = true;
//        for (uint32_t i = 0; i < ENTRY_SIZE; i++)
//            if (buf[i] != 0xFF) { all_ff = false; break; }
//        if (all_ff)  {
//        	break;
//        }
//        offset += ENTRY_SIZE;
//        }
//            // 스캔이 멈춘 지점이 바로 다음에 쓸 위치
//            log_write_offset = offset;
//}

void meas_data_log_fast_init(uint16_t index_num)
{
    log_write_offset = index_num * ENTRY_SIZE;
    if (log_write_offset > LOG_MAX_SIZE)  // 오버런 방지
        log_write_offset = LOG_MAX_SIZE;
}
//void meas_data_log_write(const uint8_t *data, size_t len)
//{
//
//    uint32_t addr      = LOG_BASE_ADDR + log_write_offset;
//    size_t   remaining = len;
//    const uint8_t *p   = data;
//
//    /* 페이지 단위 Program */
//    while (remaining) {
//        uint32_t off = addr % LOG_PAGE_SIZE;
//        uint32_t chunk = LOG_PAGE_SIZE - off;
//        if (chunk > remaining) chunk = remaining;
//
//        SPI_FLASH_PageProgram(addr, (uint8_t*)p, chunk);
//
//        addr             += chunk;
//        p                += chunk;
//        remaining        -= chunk;
//        log_write_offset += chunk;
//    }
//}


void meas_data_log_write(const uint8_t *data, size_t len)
{


    uint32_t addr = LOG_BASE_ADDR + log_write_offset;
    size_t remaining = len;
    const uint8_t *p = data;

    while (remaining) {
        uint32_t off = addr % LOG_PAGE_SIZE;
        uint32_t chunk = LOG_PAGE_SIZE - off;
        if (chunk > remaining) chunk = remaining;

        SPI_FLASH_PageProgram(addr, (uint8_t*)p, chunk);

        addr += chunk;
        p += chunk;
        remaining -= chunk;
        log_write_offset += chunk;

//        printf("[DEBUG] Chunk written: chunk=%lu, next_offset=%lu\r\n", (unsigned long)chunk, (unsigned long)log_write_offset);
    }

    if (log_write_offset >= LOG_MAX_SIZE) {
//        printf("[WARN] log_write_offset overflow! Reset to 0\r\n");
        log_write_offset = 0;
    }
}


void meas_data_log_read(uint32_t offset, uint8_t *buf, size_t len)
{
    if (offset + len > LOG_MAX_SIZE) return;
    SPI_FLASH_ReadData(LOG_BASE_ADDR + offset, buf, len);
}


static void uart3_printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (len > 0) {
        HAL_UART_Transmit(&huart3, (uint8_t*)buf,
                          (len < sizeof(buf) ? len : sizeof(buf)),
                          HAL_MAX_DELAY);
    }
}

/* 상태 덤프와 테스트 함수 */
void dump_flash_status(void)
{
    uint8_t sr = 0;
   SPI_FLASH_ReadStatus(&sr);
    uart3_printf("FLASH_SR=0x%02X  BUSY=%u  WEL=%u\r\n",
                 sr, (sr&0x01)?1:0, (sr&0x02)?1:0);
}

//void test_flash_write_read(void)
//{
//    const uint8_t pat[4] = {0xAA,0x55,0xF0,0x0F};
//    uint8_t buf[4];
//
//    uart3_printf("== FLASH TEST ==\r\n");
//    dump_flash_status();
//
//    SPI_FLASH_EraseSector(0x0);
//    dump_flash_status();
//
//    SPI_FLASH_WriteEnable();
//    dump_flash_status();
//
//    SPI_FLASH_PageProgram(0x0, (uint8_t*)pat, sizeof(pat));
//    while (SPI_FLASH_ReadStatus() & 0x01) { }
//    dump_flash_status();
//
//    SPI_FLASH_ReadData(0x0, buf, sizeof(buf));
//    uart3_printf("Read:");
//    for (int i = 0; i < 4; i++)  uart3_printf(" %02X", buf[i]);
//    uart3_printf("\r\n");
//}

//void meas_data_log_erase(void) // 메모리 모두 Print
//{
//    uint32_t sectors = LOG_MAX_SIZE / LOG_SECTOR_SIZE;
//    uint8_t buf[LOG_SECTOR_SIZE];
//
//    printf("[ERASE] Start flash erase, sectors=%lu\r\n", (unsigned long)sectors);
//
//    for (uint32_t s = 0; s < sectors; s++)
//    {
//        uint32_t addr = LOG_BASE_ADDR + s * LOG_SECTOR_SIZE;
//        bool erased_ok = false;
//
//        for (int attempt = 0; attempt < 3 && !erased_ok; attempt++)  // 최대 3회 시도
//        {
//            SPI_FLASH_EraseSector(addr);
//            SPI_FLASH_ReadData(addr, buf, LOG_SECTOR_SIZE);
//
//            erased_ok = true;
//            for (uint32_t i = 0; i < LOG_SECTOR_SIZE; i++) {
//                if (buf[i] != 0xFF) {
//                    erased_ok = false;
////                    printf("[ERASE][FAIL] Sector %lu (0x%08lX) attempt=%d offset=%lu data=0x%02X\r\n",
////                           (unsigned long)s, (unsigned long)addr, attempt+1,
////                           (unsigned long)i, buf[i]);
//                    break;
//                }
//            }
//        }
//
//        if (erased_ok) {
//            printf("[ERASE][OK] Sector %lu (0x%08lX)\r\n", (unsigned long)s, (unsigned long)addr);
//            for (uint32_t i = 0; i < LOG_SECTOR_SIZE; i += 16) {
//                printf("0x%08lX : ", (unsigned long)(addr + i));
//                for (uint32_t j = 0; j < 16; j++) {
//                    printf("%02X ", buf[i + j]);
//                }
//                printf("\r\n");
//            }
//        } else {
//            printf("[ERASE][ERROR] Sector %lu (0x%08lX) erase failed after retries!\r\n",
//                   (unsigned long)s, (unsigned long)addr);
//        }
//    }
//
//    log_write_offset = 0;
//    printf("[ERASE] All sectors processed. log_write_offset reset.\r\n");
//}
void meas_data_log_erase(void) // 3번 Check
{
    uint32_t sectors = LOG_MAX_SIZE / LOG_SECTOR_SIZE;
    uint8_t buf[LOG_SECTOR_SIZE];

    for (uint32_t s = 0; s < sectors; s++)
    {
        uint32_t addr = LOG_BASE_ADDR + s * LOG_SECTOR_SIZE;
        bool erased_ok = false;

        for (int attempt = 0; attempt < 3 && !erased_ok; attempt++)  // 최대 3회 시도
        {
            SPI_FLASH_EraseSector(addr);
            SPI_FLASH_ReadData(addr, buf, LOG_SECTOR_SIZE);

            erased_ok = true;
            for (uint32_t i = 0; i < LOG_SECTOR_SIZE; i++) {
                if (buf[i] != 0xFF) {
                    erased_ok = false;
                    break;
                }
            }
        }
        // 출력 부분 제거됨 (OK/FAIL 로그 없음)
    }

    log_write_offset = 0;
}


//void meas_data_log_erase(void) // Erase 한번
//{
//    uint32_t sectors = LOG_MAX_SIZE / LOG_SECTOR_SIZE;
//    for (uint32_t s = 0; s < sectors; s++)
//    {
//        SPI_FLASH_EraseSector(LOG_BASE_ADDR + s * LOG_SECTOR_SIZE);
//    }
//    log_write_offset = 0;
//}
#include <string.h>
#include <stdbool.h>

void meas_data_log_erase_if_not_empty(void)
{
    uint32_t sectors = LOG_MAX_SIZE / LOG_SECTOR_SIZE;
    uint8_t buf[LOG_SECTOR_SIZE];

    for (uint32_t s = 0; s < sectors; s++)
    {
        uint32_t addr = LOG_BASE_ADDR + s * LOG_SECTOR_SIZE;
        SPI_FLASH_ReadData(addr, buf, LOG_SECTOR_SIZE);

        // 섹터 전체가 0xFF이면 erase 불필요
        bool not_empty = false;
        for (uint32_t i = 0; i < LOG_SECTOR_SIZE; i++) {
            if (buf[i] != 0xFF) {
                not_empty = true;
                break;
            }
        }
        if (not_empty) {
            SPI_FLASH_EraseSector(addr);
        }
    }
    log_write_offset = 0;
}


/**
 * @brief  log_entry_t 하나를 플래시에 이어쓰기
 */
void meas_data_log_write_entry(const log_entry_t *e)
{
    // 체크섬 계산 예시 (간단 sum)
    uint16_t sum = 0;
    const uint8_t *p = (const uint8_t*)e;
    for (size_t i = 0; i < ENTRY_SIZE - sizeof(e->checksum); i++) {
        sum += p[i];
    }
    // 구조체 복사 후 checksum 필드에 기록
    log_entry_t tmp = *e;
    tmp.checksum    = sum;

    // ENTRY_SIZE 바이트를 그대로 append
    meas_data_log_write((uint8_t*)&tmp, ENTRY_SIZE);
}

/**
 * @brief  플래시에 기록된 n번째 엔트리를 읽어옴
 */
void meas_data_log_read_entry(uint32_t index, log_entry_t *e)
{
    uint32_t offset = index * ENTRY_SIZE;
    if (offset + ENTRY_SIZE > LOG_MAX_SIZE) {
        // 범위 벗어나면 초기화
        memset(e, 0xFF, ENTRY_SIZE);
        return;
    }
    meas_data_log_read(offset, (uint8_t*)e, ENTRY_SIZE);
}


void meas_data_log_read_last(void)
{
    if (log_write_offset < ENTRY_SIZE) return;
    uint32_t read_ofs = log_write_offset - ENTRY_SIZE;
    log_entry_t e;
    meas_data_log_read(read_ofs, (uint8_t*)&e, ENTRY_SIZE);

    // UART 출력 (print_log_entry와 동일)
    char txt[128];
    int year = 2000 + e.year;
    float temp_c = e.temperature / 10.0f;
    int len = snprintf(txt, sizeof(txt),
        "Idx:%3u Date:%04d-%02u-%02u %02u:%02u:%02u "
        "Cnt:%3u T:%.1fCC Dose:%.2fmSv/h Mark:0x%02X RadFlag:%d\r\n",
        e.index, year, e.month, e.day, e.hour, e.minute, e.second,
        e.count, temp_c, e.dose/100.0f, e.mark, e.rad_measure_mark
    );
    HAL_UART_Transmit(&huart3, (uint8_t*)txt, len, HAL_MAX_DELAY);
}


//static void print_log_entry(const log_entry_t *e)
//{
//    char buf[128];
//    int year = 2000 + e->year;
//    float temp_c = e->temperature / 10.0f;
//    int len = snprintf(buf, sizeof(buf),
//        "Idx:%3u Date:%04d-%02u-%02u %02u:%02u:%02u  "
//        "Cnt:%3u  T:%.1fCC  Dose:%.2fmSv/h  Mark:0x%02X\r\n",
//        e->index,
//        year, e->month, e->day,
//        e->hour, e->minute, e->second,
//        e->count,
//        temp_c,
//        e->dose / 100.0f,
//        e->mark,
//		e->rad_measure_mark       // ← 인자 추가
//    );
//    HAL_UART_Transmit(&huart3, (uint8_t*)buf, len, HAL_MAX_DELAY);
//}
//
///**
// * @brief  저장된 모든 로그 엔트리를 읽어 UART로 출력
// */
//void dump_log_entries(void)
//{
//    log_entry_t e;
//    uint32_t max_idx = LOG_MAX_SIZE / ENTRY_SIZE;
//    for (uint32_t i = 0; i < max_idx; i++)
//    {
//        meas_data_log_read_entry(i, &e);
//        // 비어 있으면(인덱스==0xFF) 끝
//        if (e.year== 0xFF) break;
//        print_log_entry(&e);
//    }
//}
