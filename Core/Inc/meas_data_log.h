#ifndef MEAS_DATA_LOG_H
#define MEAS_DATA_LOG_H

#include <stdint.h>
#include <stddef.h>

/* 로그 전용 매크로 (HAL 드라이버와 충돌 없음) */
#define LOG_BASE_ADDR    (1UL * 1024 * 1024)        // Flash 내부 오프셋
#define LOG_MAX_SIZE     (1UL * 1024 * 1024)// 1 MB 로그 영역
#define LOG_SECTOR_SIZE  4096               // 4 KB 단위로 Erase
#define LOG_PAGE_SIZE    256                // 256 B 단위로 Program

#pragma pack(push,1)
typedef struct {
	uint16_t index;         // 2B: 로그 순번 (0~65535)
    uint8_t  year;         //  1B: 2000년 기준 오프셋 (예: 25→2025)
    uint8_t  month;        //  1B: 1~12
    uint8_t  day;          //  1B: 1~31
    uint8_t  hour;         //  1B: 0~23
    uint8_t  minute;       //  1B: 0~59
    uint8_t  second;       //  1B: 0~59

    uint16_t   count;        //  1B: 측정 Count (0~65535)
    int16_t   temperature;  //  1B: 온도, -80~50℃ (정수)
    uint32_t dose;         //  4B: 선량, 단위 0.01 mSv/h (예: 1250→12.50 mSv/h)
    uint8_t  mark;
    uint8_t  rad_measure_mark; // Rad 저장할때만 0x01

    uint16_t checksum;     //  2B: CRC16 (혹은 simple sum)
    uint8_t  reserved[1];   // 3B: 확장용
} log_entry_t;
#pragma pack(pop)

#define ENTRY_SIZE  sizeof(log_entry_t)  // → 16

/**
 * @brief  log_entry_t 하나를 플래시에 이어쓰기
 */
void meas_data_log_write_entry(const log_entry_t *e);

/**
 * @brief  플래시에 기록된 n번째 엔트리를 읽어옴
 */
void meas_data_log_read_entry(uint32_t index, log_entry_t *e);

/**
 * @brief  Flash에 측정 데이터를 순차 기록
 */
void meas_data_log_write(const uint8_t *data, size_t len);

void meas_data_log_read_last(void);


/**
 * @brief  부팅 시 Flash를 스캔해서 log_write_offset 복원
 */
void meas_data_log_init(void);

void meas_data_log_fast_init(uint16_t index_num);
/** @brief 플래시에 기록된 모든 로그 데이터를 UART3로 헥사 덤프 */
void dump_all_flash_data(void);

/** @brief 저장된 모든 엔트리를 UART3로 덤프 */
void dump_log_entries(void);

/**
 * @brief  Flash로부터 측정 데이터를 읽음
 */
void meas_data_log_read(uint32_t offset, uint8_t *buf, size_t len);
void meas_data_log_erase(void);
void meas_data_log_erase_if_not_empty(void);

void dump_flash_status(void);
void test_flash_write_read(void);

#endif /* MEAS_DATA_LOG_H */
