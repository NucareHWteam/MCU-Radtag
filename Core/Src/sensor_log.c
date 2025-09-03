#include "sensor_log.h"
#include "stm32u0xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

DeviceSettings current_settings;
extern DeviceConfig device_config;
extern uint8_t Meas_Mode;
extern uint32_t Tick_Save;
extern int8_t   First_Measure;
extern uint32_t Interval_LCD_Count;
extern volatile bool RTC_During_Wakeup;


// ==== Epoch(절대초) 유틸: 2000-01-01 00:00:00 기준 ====
bool is_leap_u16(uint16_t y2000){
    uint16_t y = 2000 + y2000;
    return ((y%4==0) && (y%100!=0)) || (y%400==0);
}
 uint8_t dim_u16(uint8_t m, uint16_t y2000){
    static const uint8_t dim[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    return (m==2) ? (uint8_t)(dim[1] + (is_leap_u16(y2000)?1:0)) : dim[m-1];
}
 uint32_t ymd_to_epoch(uint16_t y2000, uint8_t m, uint8_t d, uint8_t hh, uint8_t mm, uint8_t ss){
    // days since 2000-01-01
    uint32_t days = 0;
    for(uint16_t y=0; y<y2000; ++y) days += (365 + (is_leap_u16(y)?1:0));
    for(uint8_t im=1; im<m; ++im)  days += dim_u16(im, y2000);
    days += (uint32_t)(d - 1);
    return days*86400u + (uint32_t)hh*3600u + (uint32_t)mm*60u + ss;
}
 void epoch_to_ymdhms(uint32_t epoch, uint16_t *y2000, uint8_t *m, uint8_t *d,
                             uint8_t *hh, uint8_t *mm, uint8_t *ss){
    uint32_t days = epoch / 86400u;
    uint32_t sod  = epoch % 86400u;
    *hh = (uint8_t)(sod/3600u); sod%=3600u;
    *mm = (uint8_t)(sod/60u);
    *ss = (uint8_t)(sod%60u);

    uint16_t y = 0;
    while(1){
        uint32_t ydays = 365 + (is_leap_u16(y)?1:0);
        if(days >= ydays){ days -= ydays; ++y; }
        else break;
    }
    *y2000 = y;

    uint8_t mon=1;
    while(1){
        uint8_t mdays = dim_u16(mon, *y2000);
        if(days >= mdays){ days -= mdays; ++mon; }
        else break;
    }
    *m = mon;
    *d = (uint8_t)(days+1);
}


 void Save_DeviceSettings(const DeviceSettings *cfg) {
     __disable_irq();
     HAL_FLASH_Unlock();

     FLASH_EraseInitTypeDef erase = { .TypeErase=FLASH_TYPEERASE_PAGES,
         .Page=(SETTINGS_FLASH_ADDR-FLASH_BASE)/FLASH_PAGE_SIZE, .NbPages=1 };
     uint32_t err=0;
     if (HAL_FLASHEx_Erase(&erase, &err)!=HAL_OK || err!=0xFFFFFFFFu) goto done;

     uint32_t off = 0;
     const uint8_t *p = (const uint8_t*)cfg;
     uint8_t buf[8];
     while (off < sizeof(DeviceSettings)) {
         uint32_t n = (sizeof(DeviceSettings)-off >= 8) ? 8 : (sizeof(DeviceSettings)-off);
         memset(buf, 0xFF, 8);
         memcpy(buf, p+off, n);
         if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                 SETTINGS_FLASH_ADDR+off, *(uint64_t*)buf) != HAL_OK) break;
         off += 8;
     }
 done:
     HAL_FLASH_Lock();
     __enable_irq();
 }

void Load_DeviceSettings(DeviceSettings *cfg) {
    memcpy(cfg, (const void*)SETTINGS_FLASH_ADDR, sizeof(DeviceSettings));
}

// 내부 로컬 캐시 로드/세이브
void Load_DefaultSettings(void) {
    memcpy(&current_settings, (const void*)SETTINGS_FLASH_ADDR, sizeof(DeviceSettings));

    // CSV용 주요 설정값
    strncpy(current_settings.device_code, "LogNc 1", sizeof(current_settings.device_code) - 1);
    strncpy (current_settings.firmware_ver, "1.0.0", sizeof(current_settings.firmware_ver) - 1);
    strncpy(current_settings.model, "Test ver", sizeof(current_settings.model) - 1);
    strncpy(current_settings.serial, "RT01023", sizeof(current_settings.serial) - 1);
    strncpy(current_settings.trip_desc, "Test trip code", sizeof(current_settings.trip_desc) - 1);
    current_settings.trip_code = 0x01;

    // 센서 타입 (probe_type 용)
    current_settings.sensor_type = 1; // 예: 1이면 T&R(interval)로 해석 (CSV는 고정문자열로 처리됨)

    // Config 설정
    current_settings.start_delay = 60;
    current_settings.temp_interval = 600;
    //device_config.interval_time = current_settings.temp_interval;//
    current_settings.rad_interval = 600;
    current_settings.alarm_delay = 0;

    // Display 설정
    current_settings.display_temp_unit = 0;
    current_settings.display_dose_unit = 0;
    device_config.display_temp = current_settings.display_temp_unit;
    device_config.display_dose = current_settings.display_dose_unit;
    Save_IntervalInfo_To_Backup(&device_config);

    // 알람 설정값
    current_settings.alarm_rh1 = 2000; // devide 100 is u/Sv
    current_settings.alarm_rh2 = 5000;
    current_settings.alarm_th1 = 100; // device 10 is 'C
    current_settings.alarm_th2 =500;
    current_settings.alarm_tl1 = -200;
    current_settings.alarm_tl2 = -300;

    current_settings.alarm_delay_rh1 = 0;
	current_settings.alarm_delay_rh2 = 0;
	current_settings.alarm_delay_th1 = 0;
	current_settings.alarm_delay_th2 = 0;
	current_settings.alarm_delay_tl1 = 0;
	current_settings.alarm_delay_tl2 = 0;

	current_settings. mode_status = 0x00;
	current_settings.report_format = 0x01;
    current_settings.start_time_info = 0x00;
    current_settings.start_reservation_time = 0;
    current_settings.start_time_info_for_alarm = 0xFFFFFFFFUL;
    current_settings.start_mode = 0x00;
    current_settings.start_target_seconds = 0x00;  // start_delay 적용된 RTC 기준 초 시각
    current_settings.interval_duration_day= 0x00;
    current_settings.end_target_seconds= 0x00;
    current_settings.start_time_info_for_alarm = 0x00;
    Save_CurrentSettings();
//    printf("[Debug][Load] Loaded start_target_seconds: %lu\r\n", current_settings.start_target_seconds);
}

void Save_CurrentSettings(void)
{

    Save_DeviceSettings(&current_settings);
    printf("[Save] DeviceSettings saved to Flash.\n");

}




void Init_DeviceSettings(void) {
	memcpy(&current_settings, (const void*)SETTINGS_FLASH_ADDR, sizeof(DeviceSettings));

    uint8_t *p = (uint8_t*)&current_settings;
    int empty = 1;
    for (size_t i = 0; i < sizeof(DeviceSettings); i++) {
        if (p[i] != 0xFF) {
            empty = 0;
            break;
        }
    }
//
    if (empty) {
    	printf("--------------Default Setting call----------------");
    Load_DefaultSettings();
    }

}



//void AlarmDelay_InitStart_Flash(void)
//{
//    RTC_TimeTypeDef t; RTC_DateTypeDef d;
//    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
//    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN); // 시간 래치
//
//    uint32_t now_sec = t.Hours*3600 + t.Minutes*60 + t.Seconds;
//
//    Load_DefaultSettings(); // RAM 캐시 최신화
//    current_settings.start_time_info_for_alarm = now_sec; // Start 기준 초(0~86399)
//    current_settings.start_time_info = now_sec;
//    Save_CurrentSettings();                    // ★ Flash에 1회 기록
//
//    printf("[AlarmDelay] Start 기준초 저장: %02u:%02u:%02u (%lu)\r\n",
//           t.Hours, t.Minutes, t.Seconds, now_sec);
//}
void AlarmDelay_InitStart_Flash(void)
{
    RTC_TimeTypeDef t; RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);

    uint32_t now_epoch = ymd_to_epoch(d.Year, d.Month, d.Date,
                                      t.Hours, t.Minutes, t.Seconds);

    Set_EndTarget_FromNow_Days(current_settings.interval_duration_day);
    current_settings.start_time_info = now_epoch;
    current_settings.start_time_info_for_alarm = now_epoch; // ★ 이제 절대초로 저장 (이전에는 SOD 저장)
    Save_CurrentSettings();

    printf("[AlarmDelay] Gate start EPOCH=%lu (%02u:%02u:%02u)\r\n",
           (unsigned long)now_epoch, t.Hours, t.Minutes, t.Seconds);
}



static inline uint32_t rtc_now_sec(void)
{
    RTC_TimeTypeDef t; RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
    return t.Hours*3600 + t.Minutes*60 + t.Seconds;
}

static inline uint32_t sec_elapsed_from(uint32_t start_s, uint32_t now_s)
{
    return (now_s >= start_s) ? (now_s - start_s) : (86400 - start_s + now_s);
}

static inline uint16_t max6_u16(uint16_t a, uint16_t b, uint16_t c,
                                uint16_t d, uint16_t e, uint16_t f)
{
    uint16_t m=a; if(b>m)m=b; if(c>m)m=c; if(d>m)m=d; if(e>m)m=e; if(f>m)m=f; return m;
}

// 알람 평가 진입부에 먼저 호출해서 게이트 처리
// gate가 열리지 않은 알람은 '평가하지 않음'
//void AlarmDelay_Gate_BeforeCheck(bool *g_rh1, bool *g_rh2,
//                                 bool *g_th1, bool *g_th2,
//                                 bool *g_tl1, bool *g_tl2,
//                                 bool *skip_all_after_done)
//{
//    *skip_all_after_done = false;
//
//    Load_DefaultSettings(); // Flash -> RAM
//
//    // 모든 알람 딜레이 종료 이후라면 비교 스킵
//    if (current_settings.start_time_info_for_alarm == 0xFFFFFFFFUL) {
//        *g_rh1=*g_rh2=*g_th1=*g_th2=*g_tl1=*g_tl2 = true;
//        *skip_all_after_done = true;
//        // printf("[AlarmDelay] 완료 플래그 감지 → 시간 비교 스킵\r\n");
//        return;
//    }
//
//    uint32_t start_s = current_settings.start_time_info_for_alarm; // 0~86399
//    uint32_t now_s   = rtc_now_sec();
//    uint32_t elapsed = sec_elapsed_from(start_s, now_s);
//
//    // 구조체에 각 알람 딜레이 값 이미 존재 (RH/TH/TL 6개):contentReference[oaicite:5]{index=5}
//    uint16_t d_rh1 = current_settings.alarm_delay_rh1;
//    uint16_t d_rh2 = current_settings.alarm_delay_rh2;
//    uint16_t d_th1 = current_settings.alarm_delay_th1;
//    uint16_t d_th2 = current_settings.alarm_delay_th2;
//    uint16_t d_tl1 = current_settings.alarm_delay_tl1;
//    uint16_t d_tl2 = current_settings.alarm_delay_tl2;
//
//    *g_rh1 = (elapsed >= d_rh1);
//    *g_rh2 = (elapsed >= d_rh2);
//    *g_th1 = (elapsed >= d_th1);
//    *g_th2 = (elapsed >= d_th2);
//    *g_tl1 = (elapsed >= d_tl1);
//    *g_tl2 = (elapsed >= d_tl2);
//
//    // 모든 딜레이 다 지났으면, 이후부터는 비교 자체 생략하도록 플래그 세팅
//    uint16_t dmax = max6_u16(d_rh1,d_rh2,d_th1,d_th2,d_tl1,d_tl2);
//    if (elapsed >= dmax) {
//        current_settings.start_time_info_for_alarm = 0xFFFFFFFFUL; // ★ 완료 센티넬
//        Save_CurrentSettings();                           // Flash에 1회 갱신
//        // printf("[AlarmDelay] 모든 딜레이 종료 → 센티넬 기록(0xFFFFFFFF)\r\n");
//    }
//
//    // 디버그(원하면)
//    // printf("[AlarmDelay] elapsed=%lu RH1:%d RH2:%d TH1:%d TH2:%d TL1:%d TL2:%d\r\n",
//    //        elapsed, *g_rh1,*g_rh2,*g_th1,*g_th2,*g_tl1,*g_tl2);
//}

// Delay 타깃을 '절대초'로 저장
void Set_StartTargetTime_FromRTC(void)
{
    RTC_TimeTypeDef t; RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);

    uint32_t now_epoch = ymd_to_epoch(d.Year, d.Month, d.Date, t.Hours, t.Minutes, t.Seconds);

//    Load_DefaultSettings();
    // 최대 31일(요구사항), 더 길게 허용하려면 아래 clamp 제거
    uint32_t max_delay = 31u * 86400u;
    if (current_settings.start_delay > max_delay) current_settings.start_delay = max_delay;

    uint32_t target_epoch = now_epoch + current_settings.start_delay;
//    current_settings.start_time_info = target_epoch;         // ★ 절대초 저장
    current_settings.start_target_seconds = target_epoch;    // (과거 호환; 의미를 '절대초'로 통일)
    Save_CurrentSettings();

    // 알람은 정확히 Delay 만큼 뒤로
    RTC_SetAlarmA_SecondsFromNow(current_settings.start_delay -3);
    printf("[Delay] target_epoch=%lu, after=%us -> AlarmA set.\r\n",
           (unsigned long)target_epoch, (unsigned)current_settings.start_delay-3);
}

// 남은 시간 계산을 '절대초' 기준으로

void Get_And_Print_Seconds_Left_ToStart(void)
{
    // 1) 현재 epoch
    RTC_TimeTypeDef t;
    RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
    uint32_t now_epoch = ymd_to_epoch(d.Year, d.Month, d.Date, t.Hours, t.Minutes, t.Seconds);

    // 2) 타깃 epoch 우선 사용
    uint32_t target_epoch = current_settings.start_target_seconds;

    // 3) target_epoch가 비어 있으면 BKP에서만 "조심스럽게" 복구
    //    - BKP == 0xFFFFFFFF : 미설정 → 복구 금지
    //    - BKP < 86400       : 당일초(legacy)라 날짜를 알 수 없음 → 복구 금지
    //    - BKP >= 86400      : epoch 후보로 간주하여 복구 (플래시에 동기화)
    if (target_epoch == 0) {
//        uint32_t bkp = HAL_RTCEx_BKUPRead(&hrtc, BKP_WAKEUP_TIME);
//        if (bkp != 0xFFFFFFFFu && bkp >= 86400u) {
//            target_epoch = bkp;
//            current_settings.start_target_seconds = target_epoch;
//            Save_CurrentSettings();
//            printf("[Delay] recovered target from BKP: %lu\r\n", (unsigned long)target_epoch);
//        } else {
            // ★여기서 start_delay로 보정/재무장하지 않음★
            // 복구 불가: 남은 시간 표시는 0으로 두고 조용히 반환
            start_remaining = 0;
            Clear_WakeupTime_Backup();
            AlarmDelay_InitStart_Flash();
            Set_ModeStatus(mode_internal_backup_start);
            Switch_Backup_reg(MODE_Start);
            First_Measure = 0;
            Tick_Save = 0;
            Interval_LCD_Count = 1;
            RTC_During_Wakeup = 0;
            printf("[Delay] no valid target (epoch empty, BKP invalid). Skip auto-correction.\r\n");
            printf("[Delay->Start] 측정 시작 전환 완료.\r\n");
//        }
    }

    // 4) 남은 시간 계산
    if (target_epoch <= now_epoch) start_remaining = 0;
    else                           start_remaining = target_epoch - now_epoch;

    printf("Start 예정까지 남은 시간: %lu초\r\n", (unsigned long)start_remaining);

    // 5) 딜레이 종료 → 시작 전환 (AlarmA는 이미 Set_Wakeup...에서 설정됨)
    if (start_remaining == 0) {
        AlarmDelay_InitStart_Flash();
        Set_ModeStatus(mode_internal_backup_start);
        Clear_WakeupTime_Backup();
        Switch_Backup_reg(MODE_Start);
        First_Measure = 0;
        Tick_Save = 0;
        Interval_LCD_Count = 1;
        RTC_During_Wakeup = 0;
        printf("[Delay->Start] 측정 시작 전환 완료.\r\n");
    } else if (start_remaining <= 7) {
        Tick_Save = 0;
    }
}


void Set_ModeStatus(uint8_t mode_status)
{
    current_settings.mode_status = mode_status;
    printf("[MODE] Set mode_status = %u\r\n", mode_status);
    Save_CurrentSettings();                 // 플래시에 저장
}


