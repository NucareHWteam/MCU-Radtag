#ifndef SENSOR_LOG_H
#define SENSOR_LOG_H

#include <stdint.h>
#include <stddef.h>
#include "main.h"
#include <string.h>
#include <stdbool.h>

//#define FLASH_PAGE_SIZE         0x800        // 2KB
#define SETTINGS_FLASH_ADDR     0x0803F800   // Flash 마지막 2KB (256KB Flash 기준)
extern uint32_t start_remaining;
// 장비 설정 구조체
//FIXME: device code, probe type, start mode, stop mode?
// Alarm at?
// Start time, stop time?
void AlarmDelay_Gate_BeforeCheck(bool *g_rh1, bool *g_rh2,
                                 bool *g_th1, bool *g_th2,
                                 bool *g_tl1, bool *g_tl2,
                                 bool *skip_all_after_done);
typedef struct __attribute__((packed)) {
	char      device_code[16];
    char      model[16];
    char      serial[16];
    uint8_t   sensor_type;
    char      firmware_ver[8];
    uint16_t  trip_code;
    char      trip_desc[40];
    uint8_t   start_mode; // Delay Start : 0x00   Normal Start : 0x01   Software Start : 0x02   Delay Software Start : 0x03 Reservated Time Start(Software Start) : 0x04
    uint32_t  start_delay;
    uint32_t  start_time_info_for_alarm;
    uint32_t  start_time_info;
    uint8_t   pause_enable;
    uint32_t  rad_interval;
    uint32_t  temp_interval;
    uint8_t   report_format; // 0x00 : PDF & CSV, 0x01 : CSV
    uint16_t  alarm_delay_rh1;
    uint16_t  alarm_delay_rh2;
    uint16_t  alarm_delay_th1;
    uint16_t  alarm_delay_th2;
    uint16_t  alarm_delay_tl1;
    uint16_t  alarm_delay_tl2;
    uint32_t  alarm_delay;
    uint8_t display_temp_unit, display_dose_unit;

//    int16_t   max_temp;
//    int16_t   min_temp;
//    uint16_t  max_rad;
    uint8_t   mode_status; // Stop : 0x00  Start : 0x01  pause : 0x02  Over : 0x03
    uint32_t  clock;

    // 추가 알람 값
    uint32_t alarm_rh1;
    int16_t  alarm_th1;
    int16_t  alarm_tl1;
    uint32_t alarm_rh2;
    int16_t  alarm_th2;
    int16_t  alarm_tl2;

    uint32_t start_target_seconds;  // start_delay 적용된 RTC 기준 초 시각
    uint32_t start_reservation_time;
    uint16_t interval_duration_day;
    uint32_t end_target_seconds;

} DeviceSettings;
extern DeviceSettings current_settings;
// 전체 저장/복원
void Save_DeviceSettings(const DeviceSettings *cfg);
void Load_DeviceSettings(DeviceSettings *cfg);

// 내부 로컬 복사본 관리용
void Load_DefaultSettings(void);
void Save_CurrentSettings(void);

// 항목별 Set 함수
void Set_Model(const char* model);
void Set_Serial(const char* serial);
void Set_SensorType(uint8_t type);
void Set_FirmwareVersion(const char* ver);
void Set_TripCode(uint16_t code);
void Set_TripDesc(const char* desc);
void Set_StartDelay(uint16_t delay);
void Set_TimeInfo(uint32_t info);
void Set_RadInterval(uint16_t interval);
void Set_TempInterval(uint16_t interval);
void Set_AlarmDelay(uint16_t delay);

void Set_MaxTemp(int16_t temp);
void Set_MinTemp(int16_t temp);
void Set_MaxRad(uint16_t rad);
void Set_Status(uint8_t status);
void Set_Clock(uint32_t clk);

// 새로 추가된 알람 관련
void Set_AlarmRH1(uint16_t val);
void Set_AlarmRH2(uint16_t val);
void Set_AlarmTH1(int16_t val);
void Set_AlarmTH2(int16_t val);
void Set_AlarmTL1(int16_t val);
void Set_AlarmTL2(int16_t val);


void Set_ModeStatus(uint8_t mode_status);
void Set_StartTargetTime_AtSeconds();

void Init_DeviceSettings(void);
// 항목별 Get 함수
void Get_Model(char* out, size_t maxlen);
void Get_Serial(char* out, size_t maxlen);
uint8_t Get_SensorType(void);
void Get_FirmwareVersion(char* out, size_t maxlen);
uint16_t Get_TripCode(void);
void Get_TripDesc(char* out, size_t maxlen);
uint16_t Get_StartDelay(void);
uint32_t Get_TimeInfo(void);
uint16_t Get_RadInterval(void);
uint16_t Get_TempInterval(void);
uint16_t Get_AlarmDelay(void);

//int16_t Get_MaxTemp(void);
//int16_t Get_MinTemp(void);
//uint16_t Get_MaxRad(void);
uint8_t Get_Status(void);
uint32_t Get_Clock(void);

// 새로 추가된 알람 관련
uint16_t Get_AlarmRH1(void);
uint16_t Get_AlarmRH2(void);
int16_t Get_AlarmTH1(void);
int16_t Get_AlarmTH2(void);
int16_t Get_AlarmTL1(void);
int16_t Get_AlarmTL2(void);

bool is_leap_u16(uint16_t y2000);
uint8_t dim_u16(uint8_t m, uint16_t y2000);
uint32_t ymd_to_epoch(uint16_t y2000, uint8_t m, uint8_t d,
                      uint8_t hh, uint8_t mm, uint8_t ss);
void epoch_to_ymdhms(uint32_t epoch, uint16_t *y2000, uint8_t *m, uint8_t *d,
                     uint8_t *hh, uint8_t *mm, uint8_t *ss);


#endif // SENSOR_LOG_H
