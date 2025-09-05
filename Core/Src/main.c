/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  *버전 업데이트
  *- 버튼 추가(유저버튼)
  *- 외장 Flash 로그 저장 (16바이트)
  *- Shutdown 모드 추가
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "spi_flash.h"
#include "math.h"
#include "stm32u0xx_hal.h"
#include "stm32u0xx_hal_pwr_ex.h"
#include "sensor_log.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "spi_flash.h"
#include "meas_data_log.h"
#include <stdint.h>
#include <stddef.h>
#include "stm32u0xx_hal_rtc.h"  // HAL_RTC_GetTime/GetDate, RTCHandle 정의
#include "stm32u0xx_hal_rtc_ex.h"
#include "filter.h"
#include "rad_usb.h"
#include "app_usbx_device.h"
#include "lcd.h"
#include "usb_msc_file_log.h"
#include <inttypes.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
extern UART_HandleTypeDef huart3;

#define BCD2BIN(x)  ((((x) >> 4) * 10) + ((x) & 0x0F))
#define RPULLUP_middle 210000.0f
#define RPULLUP_low 1800000.0f

static uint32_t log_send_idx = 0;    // 마지막으로 전송한 인덱스
static uint32_t max_log_idx = 0;

//////////////////////////////////RTC Backup Register
//#define RTC_MAGIC_VALUE  0x32F2
//#define BKP_DOSE_MAX      RTC_BKP_DR0   // Does Max Backup
//#define BKP_MODE_REG       RTC_BKP_DR1   // 모드 플래그, Index Number
//#define BKP_TEMP_MIN_MAX      RTC_BKP_DR2   // Temp Max/Min Back Up
//#define BKP_IDX_REG        RTC_BKP_DR3   // Temp, Rad Index,
//#define BKP_COUNT_REG      RTC_BKP_DR4   //rad_interval_count (메모리 줄이기 가능)
//#define BKP_PRE_RAD_VALUE      RTC_BKP_DR5
//#define BKP_MARK           RTC_BKP_DR6   // Mark, Alarm State (1byte만 사용)
//#define BKP_WAKEUP_TIME      RTC_BKP_DR7
//#define BKP_INTERVAL_INFO  RTC_BKP_DR8   // Interval Time (uint16) + Display Info (uint8 2개)


DeviceConfig device_config;



#define ALARM_STATE_POS_RH1   8
#define ALARM_STATE_POS_RH2  10
#define ALARM_STATE_POS_TH1  12
#define ALARM_STATE_POS_TH2  14
#define ALARM_STATE_POS_TL1  16
#define ALARM_STATE_POS_TL2  18
#define ALARM_STATE_MASK      0x3

#define SET_ALARM_STATE(v, pos, st)   (v = (v & ~(ALARM_STATE_MASK << pos)) | ((st & ALARM_STATE_MASK) << pos))
#define GET_ALARM_STATE(v, pos)       (((v) >> (pos)) & ALARM_STATE_MASK)

#define Measure_Rad_Time 10
#define Measure_Temp_Time 3




#define ALARMA_MAX_CHUNK_SEC   (31u * 86400u)
//Stop : 0x00  Start : 0x01  pause : 0x02  end : 0x03
uint16_t Load_IndexNum_From_Backup(void);
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

COMP_HandleTypeDef hcomp1;

LCD_HandleTypeDef hlcd;

LPTIM_HandleTypeDef hlptim1;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi3;
DMA_HandleTypeDef hdma_spi3_rx;
DMA_HandleTypeDef hdma_spi3_tx;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_DRD_FS;

/* USER CODE BEGIN PV */
uint16_t adc_value;
volatile uint32_t comp1_count = 0;
uint32_t count_log = 0;
uint16_t Index_num = 1;
uint16_t adc_value1;
uint16_t adc_value2;
float temperature1;
float temperature2;
float temperature1_avg = 0;
float temperature2_avg = 0;
float Display_temperature = 0;
uint32_t Tick_Save = 0;
uint32_t Interval_LCD_Count = 0;
static volatile bool   button_flag = 0;
uint8_t SW_count;
static volatile bool   USB_State = 0;
uint8_t   Meas_Mode = 0;
int8_t   First_Measure = 0;
//static uint8_t   Delay_Measure = 0;

#define RX_BUF_SIZE 64
uint8_t rx_buffer[RX_BUF_SIZE];
volatile uint8_t rx_index = 0;
volatile bool rx_flag = false;

uint32_t dose_log;

int USB_Test_count = 0;
uint8_t USB_send_buff[64] = {0xFF};
uint8_t USB_rev_buff[64] = {0xFF};
ULONG len;

volatile uint32_t last_button_tick = 0;
volatile uint8_t button_press_count = 0;
static uint32_t single_click_ts = 0;
#define DOUBLE_CLICK_MS 300

volatile uint8_t usb_ack_received = 0; // 응답 대기용

uint32_t LCD_data[4] = {0};
uint32_t LCD_data_dose[4] = {0};
uint8_t LCD_mode = 0;



volatile uint8_t measure_Rad_flag = 0;
uint16_t rad_ratio = 1;
uint16_t idx_rad, idx_temp;
uint8_t rad_interval_count;
uint32_t interval_meas_target = 0;
volatile bool RTC_During_Wakeup = 0;
uint32_t start_remaining;
UINT cur_record_num = 0;

static uint32_t now_epoch_from_rtc(void)
{
    RTC_TimeTypeDef t; RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
    return ymd_to_epoch(d.Year, d.Month, d.Date, t.Hours, t.Minutes, t.Seconds);
}


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_LCD_Init(void);
static void MX_RTC_Init(void);
static void MX_SPI3_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_COMP1_Init(void);
static void MX_LPTIM1_Init(void);
/* USER CODE BEGIN PFP */
void SPI_FLASH_Init(void);
void Switch_Backup_reg(SystemMode new_mode);
void Get_And_Print_Seconds_Left_ToStart(void);
static void PVD_Config(uint32_t level);


float read_temperature_steinhart_low(uint32_t adc_value)
{
    float r_ntc, ln_r, inv_T, T_K, T_C;
    float A,B,C;
    //-55,0,25
    A = 0.001127447424;
    B = 0.0002343835611;
    C = 0.00000008679789623;

    r_ntc = RPULLUP_low *((float)adc_value/(4095 - (float)adc_value));
    ln_r = logf(r_ntc);
    inv_T = A + B * ln_r + C * ln_r * ln_r * ln_r;
    T_K = 1.0f / inv_T;
    T_C = T_K - 273.15f;

    return T_C;
}


float read_temperature_steinhart_middle(uint32_t adc_value)
{
    float r_ntc, ln_r, inv_T, T_K, T_C;
    float A,B,C;
    //-30,10,50
    A = 0.0008515465431;
    B = 0.0002069512987;
    C = 0.00000007861588323;

    r_ntc = RPULLUP_middle *((float)adc_value/(4095 - (float)adc_value));
    ln_r = logf(r_ntc);
    inv_T = A + B * ln_r + C * ln_r * ln_r * ln_r;
    T_K = 1.0f / inv_T;
    T_C = T_K - 273.15f;

    return T_C;
}


float compare_temperature(float Temp_Middle, float Temp_Low)
{
    static uint8_t use_sensor1 = 1;

    if (use_sensor1)
    {
        if (Temp_Low <= -61.0f)
        {
            use_sensor1 = 0;
        }
    }
    else
    {
        if (Temp_Middle >= -59.0f)
        {
            use_sensor1 = 1;
        }
    }

    return (use_sensor1 ? Temp_Middle : Temp_Low);
}

////////////////////////////////////Device Info (Internal Flash) Function/////////////////////////////////////////////

////////////////////////////////////Read Temp Function/////////////////////////////////////////////
void Read_Temp(){
	ADC_ChannelConfTypeDef sConfig = {0};
	temperature1_avg = 0;
	temperature2_avg = 0;
	 for(int i=0; i < 10; i++)
		  	 		    {
		  	 		    sConfig.Channel = ADC_CHANNEL_0;
		  	 		    sConfig.Rank = ADC_REGULAR_RANK_1;
		  	 		    sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;

		  	 		    // thermisor1_middle temp 측정
		  	 		    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
		  	 		    HAL_ADC_Start(&hadc1);
		  	 		    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
		  	 		    adc_value1 = HAL_ADC_GetValue(&hadc1);
		  	 		    temperature1 = read_temperature_steinhart_middle(adc_value1);
		  	 		    temperature1_avg = temperature1_avg + temperature1;
		  	 		    HAL_ADC_Stop(&hadc1);

		  	 		    // thermisor2_low temp 측정
		  	 		    sConfig.Channel = ADC_CHANNEL_2;
		  	 		    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
		  	 		    HAL_ADC_Start(&hadc1);
		  	 		    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
		  	 		    adc_value2 = HAL_ADC_GetValue(&hadc1);
		  	 		    temperature2 = read_temperature_steinhart_low(adc_value2);
		  	 		    temperature2_avg = temperature2_avg + temperature2;
		  	 		    HAL_ADC_Stop(&hadc1);
		  	 		    }

		  	 		    temperature1_avg = temperature1_avg / 10;
		  	 		    temperature2_avg = temperature2_avg / 10;
		  	 		    Display_temperature  = compare_temperature(temperature1_avg, temperature2_avg);

}

void Write_buffer(void){
    RTC_DateTypeDef sDate;
    RTC_TimeTypeDef sTime;
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BCD);

    uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, BKP_MARK); // Mark 값 넣기
    device_config.mark = (uint8_t)(val & 0xFF);

    // 기본값은 measure_Rad_flag 그대로 사용
    uint8_t rad_mark = measure_Rad_flag;

    if (Display_temperature > 50.0f) {
        rad_mark = 0;
        printf("[RadMark] 온도 %.2f℃ > 50 → rad_measure_mark = 0 (무효 처리)\r\n", Display_temperature);
    }
    if (!rad_mark){
    	dose_log = Load_Pre_RAD_Value_From_Backup();
    }
    else{
    	Save_Pre_RAD_Value_To_Backup(dose_log);
    }

    log_entry_t entry = {
        .index             = Index_num,
        .year              = BCD2BIN(sDate.Year),
        .month             = BCD2BIN(sDate.Month),
        .day               = BCD2BIN(sDate.Date),
        .hour              = BCD2BIN(sTime.Hours),
        .minute            = BCD2BIN(sTime.Minutes),
        .second            = BCD2BIN(sTime.Seconds),
        .count             = count_log,
        .temperature       = Display_temperature * 10,   // 예: 39.5℃ → 395
        .dose              = dose_log,                   // 예: 12.50 mSv/h
        .mark              = device_config.mark,
        .rad_measure_mark  = rad_mark,                   // 조건 반영된 값
        .checksum          = 0,
        .reserved          = {0}
    };

    meas_data_log_write_entry(&entry);
    meas_data_log_read_last();

    Set_Wakeup_After_Delay(current_settings.temp_interval, &sTime, &sDate);

    Index_num++;
    Save_IndexNum_To_Backup(Index_num);
    Set_Mark_And_Save(&device_config, 0);
}


////////////////////////////////////Sleep Function/////////////////////////////////////////////

void RTC_Disable_All_Wakeup(void)
{
    // --- Wakeup Timer(WUT) ---
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);          // WUT 카운터 끄기
    __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_IT();         // EXTI line 20 IT 비활성
    __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_EVENT();      // EXTI line 20 Event 비활성
    __HAL_RTC_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);      // RTC WUT 플래그 클리어

    // --- Alarm A (원하시면 Alarm B도 같은 방식) ---
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);     // 알람 A 끄기
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF); // ALRAF 플래그 클리어
    // 필요하다면 Alarm B도:
    // HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_B);
    // __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRBF);

    // --- PWR Wakeup Flag (모든 소스 공통) ---
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
}
static bool is_leap(uint16_t y2000){ uint16_t y=2000+y2000; return ((y%4==0)&& (y%100!=0)) || (y%400==0); }
static uint8_t days_in_month(uint8_t m, uint8_t y2000){
    static const uint8_t dim[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    return (m==2) ? dim[1] + (is_leap(y2000)?1:0) : dim[m-1];
}

void RTC_SetAlarmA_SecondsFromNow(uint32_t after_sec)     // 지울예정
{
    RTC_TimeTypeDef t; RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);

    uint8_t Y=d.Year, M=d.Month, D=d.Date;
    uint32_t now = t.Hours*3600u + t.Minutes*60u + t.Seconds;
    uint32_t tgt = now + after_sec;

    // 날짜 보정
    while (tgt >= 86400u) {
        tgt -= 86400u;
        // ++D, 월말/윤년 처리
//        extern uint8_t days_in_month(uint8_t m, uint16_t y2000);
        if (++D > days_in_month(M, Y)) { D = 1; if(++M==13){ M=1; Y=(Y+1)%100; } }
    }

    // 기존 알람 정리
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);

    RTC_AlarmTypeDef a = {0};
    a.AlarmTime.Hours   = tgt/3600u;
    a.AlarmTime.Minutes = (tgt%3600u)/60u;
    a.AlarmTime.Seconds = tgt%60u;
    a.AlarmMask         = RTC_ALARMMASK_NONE;              // 날짜/시/분/초 모두 비교
    a.AlarmSubSecondMask= RTC_ALARMSUBSECONDMASK_ALL;
    a.Alarm             = RTC_ALARM_A;
    a.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;  // 날짜 비교
    a.AlarmDateWeekDay  = D;

    if (HAL_RTC_SetAlarm_IT(&hrtc, &a, RTC_FORMAT_BIN) != HAL_OK) Error_Handler();
}

// 예약 시작 시간(start_reservation_time = epoch)을 적용해
// current_settings.start_target_seconds 에 저장하고 AlarmA를 건다.
void Set_StartTargetTime_FromReservation(void)
{
    uint32_t target_epoch = current_settings.start_reservation_time; // 예약 타깃(epoch)
    if (target_epoch == 0u) {
        printf("[Reserve] start_reservation_time=0 → skip.\r\n");
        return;
    }

    // 현재 epoch
    RTC_TimeTypeDef t; RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
    uint32_t now_epoch = ymd_to_epoch(d.Year, d.Month, d.Date,
                                      t.Hours, t.Minutes, t.Seconds);

    if (target_epoch <= now_epoch) {
        // 이미 예약 시간이 지났음 → 즉시 시작 처리(원하는 정책으로)
        printf("[Reserve] target(%lu) <= now(%lu) → start immediately.\r\n",
               (unsigned long)target_epoch, (unsigned long)now_epoch);
        // AlarmDelay_InitStart_Flash(); Switch_Backup_reg(MODE_Start); ...
        return;
    }

    uint32_t remaining = target_epoch - now_epoch;
    uint32_t chunk;
    if (remaining > ALARMA_MAX_CHUNK_SEC) {
        // 너무 멀면 먼저 최대 청크까지만 자서 깨운 뒤 재무장 (현재 동작 유지)
        chunk = ALARMA_MAX_CHUNK_SEC;
    } else {
        // 타깃이 최대 청크 이하로 가까우면 5초 일찍 깨우기
        chunk = (remaining > 5u) ? (remaining - 5u) : 1u; // 최소 1초 보정
    }
    if (chunk == 0u) chunk = 1u; // 방어

    // 타이머 기준 필드(=epoch) 저장
    current_settings.start_target_seconds = target_epoch;   // 타깃은 ‘절대초’로 일원화
    Save_CurrentSettings();


    // AlarmA는 청크만큼만
    RTC_Disable_All_Wakeup();
    RTC_SetAlarmA_SecondsFromNow(chunk);

    printf("[Reserve] target=%lu, now=%lu, remain=%lu → chunk=%lu sec (AlarmA)\r\n",
           (unsigned long)target_epoch, (unsigned long)now_epoch,
           (unsigned long)remaining, (unsigned long)chunk);
}


void Maybe_Rearm_Reservation_OnWake(void)
{
    uint32_t target_epoch = current_settings.start_reservation_time;
    if (target_epoch == 0u) return;

    // 현재 epoch
    RTC_TimeTypeDef t; RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
    uint32_t now_epoch = ymd_to_epoch(d.Year, d.Month, d.Date,
                                      t.Hours, t.Minutes, t.Seconds);

    if (now_epoch < target_epoch) {
        // 아직 타깃 전 → 다음 청크 재무장
        Set_StartTargetTime_FromReservation();
    } else {
        // 타깃 도달(또는 초과) → 즉시 시작 처리(원하는 정책)
        printf("[Reserve] reached: now=%lu >= target=%lu\r\n",
               (unsigned long)now_epoch, (unsigned long)target_epoch);
        // AlarmDelay_InitStart_Flash(); Switch_Backup_reg(MODE_Start); ...
    }
}

void RTC_SetAlarmA_DaysLaterAt(uint8_t days_ahead, uint8_t hh, uint8_t mm, uint8_t ss)
{
    RTC_TimeTypeDef t; RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN); // 반드시 같이 읽기

    uint8_t Y = d.Year;   // 0~99 (2000+)
    uint8_t M = d.Month;  // 1~12
    uint8_t D = d.Date;   // 1~31

    // --- 1) 목표 날짜 계산 (≤31일 가정) ---
    uint32_t carry = days_ahead;
    while (carry){
        uint8_t dim = days_in_month(M, Y);
        uint8_t remain = dim - D;          // 이번 달에 남은 일수
        if (carry <= remain){ D += carry; carry = 0; }
        else { carry -= (remain + 1); D = 1; if(++M==13){ M=1; Y=(Y+1)%100; } }
    }

    // --- 2) 오늘+0일인데 목표 시각이 이미 지난 경우 다음날로 미룸 ---
    if (days_ahead == 0){
        uint32_t now_s = t.Hours*3600u + t.Minutes*60u + t.Seconds;
        uint32_t tgt_s = hh*3600u + mm*60u + ss;
        if (tgt_s <= now_s){
            uint8_t dim = days_in_month(M, Y);
            if (++D > dim){ D = 1; if(++M==13){ M=1; Y=(Y+1)%100; } }
        }
    }

    // --- 3) 기존 알람/플래그 정리 후 설정 ---
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);

    RTC_AlarmTypeDef a = {0};
    a.AlarmTime.Hours   = hh;
    a.AlarmTime.Minutes = mm;
    a.AlarmTime.Seconds = ss;
    a.AlarmMask         = RTC_ALARMMASK_NONE;                // 날짜/시/분/초 모두 비교
    a.AlarmSubSecondMask= RTC_ALARMSUBSECONDMASK_ALL;
    a.Alarm             = RTC_ALARM_A;
    a.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;    // '날짜' 사용
    a.AlarmDateWeekDay  = D;                                 // 목표 '일' 지정

    if (HAL_RTC_SetAlarm_IT(&hrtc, &a, RTC_FORMAT_BIN) != HAL_OK){
        Error_Handler();
    }
}

void Set_EndTarget_FromNow_Days(uint16_t days)
{
    if (days == 0) {
        // 종료 타깃 제거
        current_settings.end_target_seconds = 0;
        Save_CurrentSettings();
        printf("[EndTarget] cleared (days=0)\r\n");
        return;
    }

    uint32_t now = now_epoch_from_rtc();      // 현재 epoch (초)
    uint32_t add = (uint32_t)days * 86400u;   // 일 → 초
    uint32_t target = now + add;

    current_settings.end_target_seconds = target;
    Save_CurrentSettings();

    // 보기 좋은 날짜/시각으로도 함께 출력
    uint16_t y; uint8_t m, d, hh, mm, ss;
    epoch_to_ymdhms(now, &y, &m, &d, &hh, &mm, &ss);
    printf("[EndTarget] now=%04u-%02u-%02u %02u:%02u:%02u (epoch=%lu)\r\n",
           (unsigned)(2000 + y), (unsigned)m, (unsigned)d,
           (unsigned)hh, (unsigned)mm, (unsigned)ss,
           (unsigned long)now);

    epoch_to_ymdhms(target, &y, &m, &d, &hh, &mm, &ss);
    printf("[EndTarget] +%lu s (%u days) -> target=%04u-%02u-%02u %02u:%02u:%02u (epoch=%lu)\r\n",
           (unsigned long)add, (unsigned)days,
           (unsigned)(2000 + y), (unsigned)m, (unsigned)d,
           (unsigned)hh, (unsigned)mm, (unsigned)ss,
           (unsigned long)target);
}


static bool Has_Logging_Duration_Ended(void)
{
    if (current_settings.end_target_seconds == 0) return false;   // 미사용
    uint32_t now = now_epoch_from_rtc();
    return now >= current_settings.end_target_seconds;
}

void Save_Power_Function()
{
	HAL_PWREx_EnableUltraLowPowerMode();       // ULP 모드 활성화
	// — ADC, COMP, LCD, UART 해제 & 클럭 꺼버리기 —
	HAL_ADC_DeInit(&hadc1);      __HAL_RCC_ADC_CLK_DISABLE();
	HAL_COMP_DeInit(&hcomp1);    __HAL_RCC_COMP_CLK_DISABLE();
	HAL_LCD_DeInit(&hlcd);       __HAL_RCC_LCD_CLK_DISABLE();
	HAL_UART_DeInit(&huart3);    __HAL_RCC_USART3_CLK_DISABLE();
	// (필요하다면 GPIO 그룹 클럭도 꺼주세요)
	GPIO_InitTypeDef g = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();  // 코어가 꺼지기 전에
	// … (필요한 그룹 모두 클럭 Enable)
	g.Pin   = GPIO_PIN_ALL & ~GPIO_PIN_0;  // PA0 제외
	g.Mode  = GPIO_MODE_ANALOG;
	g.Pull  = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &g);
	// … 같은 방식으로 B, C, F 그룹까지
	// 그리고 __HAL_RCC_GPIOx_CLK_DISABLE() 로 다시 끄기
	// 내부 풀업 없는 HIGH 레벨 감지로 전환

}



void Get_in_Shutdown_Timer()
{
    Switch_Backup_reg(MODE_Start_Button);
    LCD_Clear_Display(LCD_data);
    printf("Getting on to sleep(Timer)\r\n");
    uint32_t sleep_time;

    if (rad_interval_count == 0) {
        sleep_time = current_settings.temp_interval - 14;
        printf("다음번엔 Rad 측정, Sleep Time = %lu sec\r\n", sleep_time);
    }

    else{
        sleep_time = current_settings.temp_interval - 5;
         printf("다음번엔 Temp 측정, Sleep Time = %lu sec\r\n", sleep_time);
    }

    // RTC Wake-up 타이머 설정
    RTC_Disable_All_Wakeup();

    Save_Power_Function();
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_LOW);
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2_HIGH);


    RTC_SetAlarmA_SecondsFromNow(sleep_time);

    HAL_SuspendTick();
    HAL_PWR_EnterSHUTDOWNMode();
    HAL_ResumeTick();
}


void Start_Wake_Timer(){
	Switch_Backup_reg(MODE_Start_Button);
	LCD_Clear_Display(LCD_data);
	RTC_Disable_All_Wakeup();

	   uint32_t sleep_time;

	    if (rad_interval_count == 0) {
	        sleep_time = current_settings.temp_interval - 14;
	//    	 sleep_time = 14;
	        printf("다음번엔 Rad 측정, Sleep Time = %lu sec\r\n", sleep_time);
	    }

	    else{
	        sleep_time = current_settings.temp_interval - 5;
	//    	sleep_time = 27;
	         printf("다음번엔 Temp 측정, Sleep Time = %lu sec\r\n", sleep_time);
	    }

//	    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, sleep_time, RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0) != HAL_OK)
//	        Error_Handler();
	    RTC_SetAlarmA_SecondsFromNow(sleep_time);
}

void Start_Delay_Timer()
{
	LCD_Clear_Display(LCD_data);

	RTC_Disable_All_Wakeup();

	Set_StartTargetTime_FromRTC();


}

// 알람/웨이크업 타이머 '설정 상태'는 유지하고, 발생 플래그만 지우는 버전
static void Clear_Wakeup_Flags_Only(void) {
    // RTC Alarm A 발생 플래그가 서 있었다면 지움(알람 동작 상태는 유지)
    if (__HAL_RTC_ALARM_GET_FLAG(&hrtc, RTC_FLAG_ALRAF) != RESET) {
        __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
    }
    // WUT 발생 플래그도 동일하게
    if (__HAL_RTC_WAKEUPTIMER_GET_FLAG(&hrtc, RTC_FLAG_WUTF) != RESET) {
        __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
    }
    // PWR Wakeup 플래그들 초기화
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
}


void Get_in_Shutdown(void)
{
    printf("Getting on to sleep (keep pre-armed alarm)\r\n");

    Save_Power_Function();

    // (A) 웨이크 핀 비활성 및 플래그만 정리 (알람/타이머는 건드리지 않음)
    HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
    HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN2);
    Clear_Wakeup_Flags_Only(); // ★ 알람 보존

    // (B) Shutdown 동안 핀을 '비활성 레벨'로 고정 (즉시 웨이크 방지)
    HAL_PWREx_EnablePullUpPullDownConfig();
    // WKUP1은 LOW가 활성 → 평소엔 HIGH가 안전 ⇒ Pull-Up
    HAL_PWREx_EnableGPIOPullUp  (PWR_GPIO_A, PWR_GPIO_BIT_0);   // PA0 ↑
    // WKUP2는 HIGH가 활성 → 평소엔 LOW가 안전 ⇒ Pull-Down
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_13);  // PC13 ↓

    // (C) 웨이크 핀 재활성화 (활성 극성은 기존과 동일)
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_LOW);
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2_HIGH);

    // Enable 직후 과거 잔여 플래그를 한 번 더 정리 (즉시 웨이크 방지)
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    // (D) Shutdown 진입
    HAL_SuspendTick();
    HAL_PWR_EnterSHUTDOWNMode();
    HAL_ResumeTick();

    printf("Wake Up!\r\n");
}

////////////////////////////////////FLASH Function/////////////////////////////////////////////




////////////////////////////////////Button Function/////////////////////////////////////////////


void Press_Action(void){
    if(button_flag){

        // ⭐️ 더블클릭만 별도 처리
        if (button_press_count == 2) {
            printf("Double Click!\r\n");
        	if (Meas_Mode == 2 || Meas_Mode == 4 || Meas_Mode == 3){
            Set_Mark_And_Save(&device_config, 1); // mark=1로 변경 및 백업
        	}
        	else if(Meas_Mode == 5){
        		 Switch_Backup_reg(MODE_Stop);
        		 Tick_Save = 0;
        		 First_Measure = 0;
        	}
	  	    if (Meas_Mode != 2){
	  		Tick_Save = 0;}
//            dump_log_entries();
	  	    button_press_count = 0;
            SW_count = 0; // 원래처럼 리셋, 빼도 됨
            button_flag = false;
            return;
        }
        else{
        // ⭐️ 이 아래는 기존 코드 100% 복사 (수정 없이 붙여넣어도 됨)
        bool button_flag_det = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
 	  	 if(!button_flag_det)
 	  	  {
 	  	    SW_count++;
 	  	 printf("SW_count : %d, LCD MODE : %d\r\n", SW_count, LCD_mode);

 	  	 		if (SW_count>=2){
 	  	 		printf("Mode Change\r\n");
 	  	 			if(Meas_Mode == 0){ // Boot
// 	  	 			 Index_num = 0;
// 	  	 		     Save_IndexNum_To_Backup(Index_num);
// 	  	 			 Switch_Backup_reg(MODE_Stop);
 	  	 			 SW_count = 0;
 	  	 			 button_flag = false;
 	  	 			Tick_Save = 0;
 	  	 			}

 	  	 			else if(Meas_Mode == 1){ // Stop
 	  	 		      if(current_settings.start_mode == 0x00){
 	  	 				if (current_settings.start_delay > 0){
 	  	 				Switch_Backup_reg(Mode_Start_Delay);
	   	  	 		     SW_count = 0;
	   	  	 			 button_flag = false;
	   	  	 		     Tick_Save = 0;
 	  	 				}
 	  	 				else{
 	  	 				 AlarmDelay_InitStart_Flash();
 	   	  	 			 Switch_Backup_reg(MODE_Start);
 	   	  	 			 Index_num = Load_IndexNum_From_Backup();
 	   	  	 			 Interval_LCD_Count = 1;
 	   	  	 		     SW_count = 0;
 	   	  	 			 button_flag = false;
 	   	  	 	     	 Tick_Save = 0;
 	   	  	 	         Interval_LCD_Count = 0;
 	  	 			    }
 	  	 			  }
 	  	 			  else if(current_settings.start_mode == 0x01){
 	  	 				 AlarmDelay_InitStart_Flash();
  	   	  	 			 Switch_Backup_reg(MODE_Start);
  	   	  	 			 Index_num = Load_IndexNum_From_Backup();
  	   	  	 			 Interval_LCD_Count = 1;
  	   	  	 		     SW_count = 0;
  	   	  	 			 button_flag = false;
  	   	  	 	     	 Tick_Save = 0;
  	   	  	 	         Interval_LCD_Count = 0;
 	  	 			  }
 	  	 			  else{
 	 	  	 				if (current_settings.start_delay > 0){
 	 	  	 				Switch_Backup_reg(Mode_Start_Delay);
 		   	  	 		     SW_count = 0;
 		   	  	 			 button_flag = false;
 		   	  	 		     Tick_Save = 0;
 	 	  	 				}
 	 	  	 				else{
 	 	  	 				 AlarmDelay_InitStart_Flash();
 	 	   	  	 			 Switch_Backup_reg(MODE_Start);
 	 	   	  	 			 Index_num = Load_IndexNum_From_Backup();
 	 	   	  	 			 Interval_LCD_Count = 1;
 	 	   	  	 		     SW_count = 0;
 	 	   	  	 			 button_flag = false;
 	 	   	  	 	     	 Tick_Save = 0;
 	 	   	  	 	         Interval_LCD_Count = 0;
 	 	  	 			    }
 	  	 			  }


                    Set_ModeStatus(mode_internal_backup_start);
 	  	 			Clear_WakeupTime_Backup();
 	  	 		    Clear_Backup_Index();
 	  	 			First_Measure = 0;
 	  	 			}

 	  	 			else if(Meas_Mode == 2 || Meas_Mode == 4){ // Start
 	  	 		     Switch_Backup_reg(MODE_Pause);
 	  	 		     button_flag = false;
 	  	 		    RTC_Disable_All_Wakeup();
                    Set_ModeStatus(mode_internal_backup_pause);
                    Clear_WakeupTime_Backup();
 	  	 		     SW_count = 0;
// 	  	 		     Index_num = 0;
// 	  	 		     Save_IndexNum_To_Backup(Index_num);
 	  	 		    Tick_Save = 0;
 	  	 		    First_Measure = 0;
 	  	 		    }

 	  	 			else if(Meas_Mode == 3){
 	  	 			RTC_Disable_All_Wakeup();
 	  	 		    Switch_Backup_reg(MODE_End);
                    Set_ModeStatus(mode_internal_backup_end);
 	  	 		    Tick_Save = 0;
 	  	 		    SW_count = 0;
 	  	 	     	button_flag = false;
 	  	 	        First_Measure = 0;
 	  	 		    }
 	  	 		    else if(Meas_Mode == 5){ // Start(Delay)
 	  	 		    AlarmDelay_InitStart_Flash();
 	  	 		    Switch_Backup_reg(MODE_Start);
 	 	  	 		Index_num = Load_IndexNum_From_Backup();
 	 	  	 	    Interval_LCD_Count = 1;
 	 	  	 		SW_count = 0;
 	 	  	 		button_flag = false;
 	 	  	 		First_Measure = 0;
 	 	  	 		Tick_Save = 0;
 	  	 		    }

 	  	 		}
 	  	 	 if (SW_count>9){
 	  	 		if(Meas_Mode == 6) // End Mode
 	  	 		{
 	  	 		RTC_Disable_All_Wakeup();
 	  	 		 Reset_All_Backup_Registers();
 	  	 		 First_Measure = 0;
 	  	 		SW_count = 0;
 	  	 		}
 	  	 	 }
 	  	   }


        else
        {
        	LCD_mode++;
	  	      if(Meas_Mode == 3){
	  	 	  	 Switch_Backup_reg(MODE_Start);
	  	 	     First_Measure = 0;
	  	        }
	  	 	if(Meas_Mode == 1 && First_Measure<=1){
	  	 		First_Measure = 2;
	  	 		LCD_mode = 1;
	  	 	}

//            button_press_count = 0;
            button_flag = false;
            SW_count = 0;
            Interval_LCD_Count = 1;
            printf("Button unpressed \r\n");
        }
	  	    if (Meas_Mode != 2){
	  		Tick_Save = 0;}
    }
        last_button_tick = 0;
    }
}


/////////////////////RTC////////////////////////////
void Set_RTC_TimeOnly(uint8_t yy, uint8_t mm, uint8_t dd, uint8_t hh, uint8_t mi, uint8_t ss)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    sTime.Hours = hh;
    sTime.Minutes = mi;
    sTime.Seconds = ss;
    sDate.Year = yy;
    sDate.Month = mm;
    sDate.Date = dd;
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;

    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

void PrintCurrentRTC(void)

{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    // 1) 현재 시간 가져오기 (BCD 형식)
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BCD);
    // 2) 현재 날짜 가져오기 (BCD 형식)
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BCD);
    // 3) BCD → 10진 변환
    uint8_t hour   = BCD2BIN(sTime.Hours);
    uint8_t minute = BCD2BIN(sTime.Minutes);
    uint8_t second = BCD2BIN(sTime.Seconds);
    uint8_t year  = BCD2BIN(sDate.Year);
    uint8_t month = BCD2BIN(sDate.Month);
    uint8_t day   = BCD2BIN(sDate.Date);
    // 4) 문자열로 포맷팅 (예: "2025-05-27 14:35:08\r\n")
    printf("RTC: 20%02u-%02u-%02u %02u:%02u:%02u\r\n", year, month, day, hour, minute, second);
}


//bool Check_And_Save_When_Target_Reached(void)
//{
//    uint32_t target_sec = HAL_RTCEx_BKUPRead(&hrtc, BKP_WAKEUP_TIME);
//
//    printf("[Debug] Read target_sec = %lu (0x%08lX)\n", target_sec, target_sec);
//
//    if ((target_sec & 0xFFFFFFFF) == 0xFFFFFFFF) {
//        if (Tick_Save >= Measure_Rad_Time - 1) {
//            printf("[Check] Initial mode: FF detected, Tick_Save=%lu → REACHED (조건 충족)\n", Tick_Save);
//            return true;
//        } else {
//            printf("[Check] Initial mode: FF detected, Tick_Save=%lu → NOT YET\n", Tick_Save);
//            return false;
//        }
//    }
//
//
//    RTC_TimeTypeDef sTime;
//    RTC_DateTypeDef sDate;
//
//    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BCD);
//    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BCD);
//
//    uint32_t now_sec = BCD2BIN(sTime.Hours) * 3600 +
//                       BCD2BIN(sTime.Minutes) * 60 +
//                       BCD2BIN(sTime.Seconds);
//
//    bool reached = (now_sec >= target_sec);
//
//    printf("[Check] Now=%lu, Target=%lu → %s\n",
//           now_sec, target_sec,
//           reached ? "REACHED" : "NOT YET");
//
//    return reached;
//}
bool Check_And_Save_When_Target_Reached(void)
{
    // BKP 레지스터에서 "타깃 시각" 읽기
    uint32_t target_raw = HAL_RTCEx_BKUPRead(&hrtc, BKP_WAKEUP_TIME);
    printf("[Check] BKP_WAKEUP_TIME(raw)=%lu (0x%08lX)\r\n",
           (unsigned long)target_raw, (unsigned long)target_raw);

    // 초기(미설정) 값 처리
    if (target_raw == 0xFFFFFFFFu) {
        bool reached_init = (Tick_Save >= (Measure_Rad_Time - 1));
        printf("[Check] BKP=0xFFFFFFFF → init mode, Tick_Save=%lu → %s\r\n",
               (unsigned long)Tick_Save, reached_init ? "REACHED" : "NOT YET");
        return reached_init;
    }

    // 현재 시각(BIN) 취득
    RTC_TimeTypeDef t;
    RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);

    // 당일 초(Seconds-Of-Day)
    uint32_t now_sod = (uint32_t)t.Hours * 3600u +
                       (uint32_t)t.Minutes * 60u +
                       (uint32_t)t.Seconds;

    bool reached;

    // 호환성:
    //  - 신버전: BKP에 '절대초(epoch)' 저장 (대략 1,000,000 이상인 값)
    //  - 구버전: BKP에 '당일 초(0~86399)' 저장
    if (target_raw >= 1000000u) {
        // epoch 비교
        uint32_t now_epoch = ymd_to_epoch(d.Year, d.Month, d.Date,
                                          t.Hours, t.Minutes, t.Seconds);
        reached = (now_epoch >= target_raw);
        printf("[Check][epoch] now=%lu, target=%lu → %s\r\n",
               (unsigned long)now_epoch, (unsigned long)target_raw,
               reached ? "REACHED" : "NOT YET");
    } else {
        // legacy: 당일 초 비교
        reached = (now_sod >= target_raw);
        printf("[Check][legacy-sod] now=%lu, target=%lu → %s\r\n",
               (unsigned long)now_sod, (unsigned long)target_raw,
               reached ? "REACHED" : "NOT YET");
    }

    return reached;
}






//////////////////////////////Dose Calculate
//void Count_Filter(void){
//    float    blockAvg, ema, Cal_dose ;
//    bool     blockReady;
//    static bool ema_started  = false;
//    count_log = comp1_count;
//    blockReady = Filter_Update((float)count_log, &blockAvg, &ema);
//
//    char msg[80];
//    int  len = snprintf(msg, sizeof(msg), "Raw=%lu  Avg(blk)=%.2f", count_log, blockAvg);
//
//    if (blockReady) {
//        // 새 Block이 나왔을 때만 EMA·Dose 계산 및 출력
//    	ema_started = true;
//    	Cal_dose = 41.468f * ema + 0.1908f;
//    	len += snprintf(msg+len, sizeof(msg)-len,"EMA=%.2f  Dose=%.2fmSv/h", ema, Cal_dose);
//    	dose_log = (uint32_t)(Cal_dose * 100.0f + 0.5f);
//    }
//
//    if(!ema_started){
//    	Cal_dose = 41.468f * blockAvg + 0.1908f;
//    	len += snprintf(msg+len, sizeof(msg)-len,"  Average Dose=%.2fmSv/h", Cal_dose);
//    	dose_log = (uint32_t)(Cal_dose * 100.0f + 0.5f);
//    }
//
//    else {
//        // Block은 진행 중, EMA 업데이트 안함
//        Cal_dose = 41.468f * blockAvg + 0.1908f;
//        len += snprintf(msg+len, sizeof(msg)-len, "  Average Dose=%.2fmSv/h", Cal_dose);
//        len += snprintf(msg+len, sizeof(msg)-len, "  EMA(prev)=%.2f", ema);
//        dose_log = (uint32_t)(Cal_dose * 100.0f + 0.5f);
//    }
//    strcat(msg, "\r\n");
//    HAL_UART_Transmit(&huart3, (uint8_t*)msg, len+2, HAL_MAX_DELAY);
//}

//void Last_Count_Filter(void){
//	// 1) 마지막 EMA 계산만 수행
//	count_log = comp1_count;
//	float blockAvg, ema, Cal_dose;
//	char msg[80];
//	bool ready = Filter_Update((float)count_log, &blockAvg, &ema);
//	int  len = snprintf(msg, sizeof(msg), "Raw=%lu  Avg(blk)=%.2f", count_log, blockAvg);
//
//	if (!ready) {
//	// 블록이 완성되지 않았다면 EMA 결과 반영
//		Filter_ForceEMA(&blockAvg, &ema);
//	   Cal_dose = 41.468f * ema + 0.1908f;
//	   len += snprintf(msg+len, sizeof(msg)-len,
//	      "  LAST EMA=%.2f Last Dose=%.2fmSv/h", ema, Cal_dose);
//	  dose_log = (uint32_t)(Cal_dose * 100.0f + 0.5f);
//	}
//	strcat(msg, "\r\n");
//	HAL_UART_Transmit(&huart3, (uint8_t*)msg, len+2, HAL_MAX_DELAY);
//}


void Count_Filter_Kalman(void)
{
    static bool kalmanInit = false;
    static double x_est = 0.0;   // 추정값 (Raw 기반)
    static double P = 1.0;       // 초기 공분산
    const double Q = 0.005;      // 프로세스 잡음
    const double R = 0.995;      // 측정 잡음

    count_log = comp1_count;
    double measurement = (double)count_log;

    if (!kalmanInit) {
        x_est = measurement;
        P = 1.0;
        kalmanInit = true;

        printf("[INIT] Raw=%.0f x_est(Raw)=%.6f P=%.6f\r\n",
               measurement, x_est, P);
        return;
    }

    double P_pred = P + Q;
    double K = P_pred / (P_pred + R);
    double delta = measurement - x_est;
    x_est = x_est + K * delta;
    P = (1.0 - K) * P_pred;

    double Cal_dose = 45.429 * x_est + -4.8496;
    if (Cal_dose < 0) Cal_dose = 0;
    dose_log = (uint32_t)(Cal_dose * 100.0 + 0.5);

    printf("Raw=%.0f x_est(Raw)=%.6f Dose=%.4f\r\n",
           measurement, x_est, Cal_dose);
}










//////////////////////BackUp Function/////////////////////////////
SystemMode Backup_GetMode(void)
{
    uint32_t val;
    SystemMode mode;

    // 1) DR1에서 값 읽기 (32비트)
    val = HAL_RTCEx_BKUPRead(&hrtc, BKP_MODE_REG);

    // 2) 하위 16비트만 추출
    uint16_t mode_raw = (uint16_t)(val & 0xFFFF);

    // 3) 유효 범위 검사
    if (mode_raw > MODE_End) {
        mode = MODE_Boot;
    } else {
        mode = (SystemMode)mode_raw;
    }

    // 4) 모드를 문자열로 변환
    const char *mode_str;
    switch (mode) {
        case MODE_Boot:         mode_str = "MODE_Boot";         Meas_Mode = 0; break;
        case MODE_Stop:         mode_str = "MODE_Stop";         Meas_Mode = 1; break;
        case MODE_Start:        mode_str = "MODE_Start";        Meas_Mode = 2; break;
        case MODE_Pause:        mode_str = "MODE_Pause";        Meas_Mode = 3; break;
        case MODE_Start_Button: mode_str = "MODE_Start_Button"; Meas_Mode = 4; break;
        case Mode_Start_Delay:  mode_str = "Mode_Start_Delay" ; Meas_Mode = 5; break;
        case MODE_End:          mode_str = "MODE_End";          Meas_Mode = 6; break;
        default:                mode_str = "MODE_Boot";         Meas_Mode = 0; break;
    }

    // 5) UART3로 출력 (printf 사용)
    printf("Current Mode: %s (raw=0x%08lX)\r\n", mode_str, val);

    return mode;
}




void Backup_SetMode(SystemMode mode)
{
    HAL_PWR_EnableBkUpAccess();
    // Index는 그대로 두고, Mode만 하위 16비트에 저장
    uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, BKP_MODE_REG);
    val = (val & 0xFFFF0000) | (mode & 0xFFFF);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_MODE_REG, val);
}
void Switch_Backup_reg(SystemMode new_mode)
{
    SystemMode prev_mode = Backup_GetMode();
    if (prev_mode != new_mode) {
        Backup_SetMode(new_mode);
        Backup_GetMode();
    }
    SystemMode cur_mode = Backup_GetMode(); // 이때 Meas_Mode도 자동 세팅됨

    switch (cur_mode) {
        case MODE_Boot:   Meas_Mode = 0; break;
        case MODE_Stop:
        	RTC_Disable_All_Wakeup();
        	Meas_Mode = 1;
        	break;
        case MODE_Start:  Meas_Mode = 2; break;
        case MODE_Pause:
        	Clear_WakeupTime_Backup();
        	RTC_Disable_All_Wakeup();
        	Meas_Mode = 3;
        	break;
        case MODE_Start_Button: Tick_Save = Interval_LCD_Count+2;  Meas_Mode = 4; break;
        case Mode_Start_Delay:
        	RTC_Disable_All_Wakeup();
        	if(current_settings.start_mode == 0x04){
        		Set_StartTargetTime_FromReservation();
        	}
        	else{
        	Start_Delay_Timer();}
        	Meas_Mode = 5;
        	break;
        case MODE_End:
        	RTC_Disable_All_Wakeup(); Meas_Mode = 6; break;
        default:          Meas_Mode = 0; break;
    }


    /* 호출 예시
    Switch_Backup_reg(MODE_Boot);
    Switch_Backup_reg(MODE_Stop);
    Switch_Backup_reg(MODE_Start);
    Switch_Backup_reg(MODE_Pause);
    Switch_Backup_reg(MODE_Start_Button);
    Switch_Backup_reg(Mode_Start_Delay);
    */
}




void Clear_WakeupTime_Backup(void)
{
    HAL_PWR_EnableBkUpAccess();
    // 2. 쓰기
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_WAKEUP_TIME, 0xFFFFFFFF);

    // 3. 확인용 읽기
    uint32_t check = HAL_RTCEx_BKUPRead(&hrtc, BKP_WAKEUP_TIME);
    printf("[Clear] BKP_WAKEUP_TIME Write: 0xFFFFFFFF, ReadBack = 0x%08lX\n", check);
}

void Save_IndexNum_To_Backup(uint16_t idx) {
    HAL_PWR_EnableBkUpAccess();
    uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, BKP_MODE_REG);
    val = (val & 0x0000FFFF) | (((uint32_t)idx) << 16);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_MODE_REG, val);
}

void Save_Dose_To_Backup(DeviceConfig *cfg)
{
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_DOSE_MAX, cfg->dose_max);
    printf("[Backup] Save Dose Max=%u (Raw: 0x%08lX)\n", cfg->dose_max, (unsigned long)cfg->dose_max);
}


void Save_Temp_To_Backup(DeviceConfig *cfg)
{
    // temp_min: 상위 16비트, temp_max: 하위 16비트
    uint32_t pack = (((uint16_t)cfg->temp_min) << 16) | ((uint16_t)cfg->temp_max);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_TEMP_MIN_MAX, pack);

    printf("[Backup] Save Temp Max=%d, Min=%d\n", cfg->temp_max, cfg->temp_min);
}


void Save_Backup_Index(void) {
    uint32_t reg3 = ((uint32_t)idx_temp << 16) | (idx_rad & 0xFFFF);
    uint32_t reg4 = (uint32_t)rad_interval_count;

    HAL_RTCEx_BKUPWrite(&hrtc, BKP_IDX_REG, reg3);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_COUNT_REG, reg4);

    printf("[Debug] Saved Backup Index: idx_rad=%u, idx_temp=%u, rad_interval_count=%u "
           "(Raw: reg3=0x%08lX, reg4=0x%08lX)\r\n",
           idx_rad, idx_temp, rad_interval_count, reg3, reg4);
}
void Clear_Backup_Index(void) {
    idx_rad = 0;
    idx_temp = 0;
    rad_interval_count = 0;

    uint32_t reg3 = 0;
    uint32_t reg4 = 0;

    HAL_RTCEx_BKUPWrite(&hrtc, BKP_IDX_REG, reg3);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_COUNT_REG, reg4);

    printf("[Clear] Backup Index Cleared: idx_rad=0, idx_temp=0, rad_interval_count=0 "
           "(Raw: reg3=0x%08lX, reg4=0x%08lX)\r\n", reg3, reg4);
}



void Save_MarkAndAlarmState_To_Backup(DeviceConfig *cfg)
{
    uint32_t val = ((uint32_t)(cfg->mark) & 0xFF)           // 0~7
                 | ((uint32_t)(cfg->alarm_state) << 8);     // 8~19 (6*2bit)
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_MARK, val);
    printf("[Debug] Saved Mark=0x%02X, AlarmState=0x%04X (Packed=0x%08lX)\r\n",
           cfg->mark, cfg->alarm_state, val);
}



void Save_IntervalInfo_To_Backup(DeviceConfig *cfg)
{
    uint32_t info_pack = ((uint32_t)cfg->interval_time & 0xFFFF)
                       | (((uint32_t)cfg->display_temp & 0xFF) << 16)
                       | (((uint32_t)cfg->display_dose & 0xFF) << 24);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_INTERVAL_INFO, info_pack);
}

void Save_Mark_To_Backup(uint8_t mark)
{
    uint32_t prev_val = HAL_RTCEx_BKUPRead(&hrtc, BKP_MARK);
    uint32_t alarm_bits = (prev_val & 0xFFFFFF00);  // 상위 alarm_state 보존
    uint32_t new_val = (mark & 0xFF) | alarm_bits;
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_MARK, new_val);

    printf("[Debug] Saved ONLY Mark=0x%02X (Prev=0x%08lX → New=0x%08lX)\r\n",
           mark, prev_val, new_val);
}

void Set_Mark_And_Save(DeviceConfig* cfg, uint8_t mark_value)
{
    cfg->mark = mark_value;
    Save_Mark_To_Backup(cfg->mark);
}

void Save_WakeupTime_To_Backup(uint32_t target_sec)
{
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_WAKEUP_TIME, target_sec);
    uint32_t verify = HAL_RTCEx_BKUPRead(&hrtc, BKP_WAKEUP_TIME);

    printf("[Backup] Save Wakeup Target Time = %lu (Raw: 0x%08lX)\n", target_sec, target_sec);
    printf("[Verify] ReadBack Value = %lu (Raw: 0x%08lX)\n", verify, verify);
}

void Save_Pre_RAD_Value_To_Backup(uint32_t value)
{
    HAL_PWR_EnableBkUpAccess();   // BKUP 영역 쓰기 권한 허용
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_PRE_RAD_VALUE, value);
    printf("[Backup] Save Pre_RAD_Value = %lu (Raw: 0x%08lX)\r\n", value, value);
}


void Save_All_Config_To_BackupRegister(DeviceConfig *cfg)
{
    Save_Temp_To_Backup(cfg);
    Save_Dose_To_Backup(cfg);

    Save_MarkAndAlarmState_To_Backup(cfg);
    Save_IntervalInfo_To_Backup(cfg);
}

uint16_t Load_IndexNum_From_Backup(void)
{
    uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, BKP_MODE_REG);
    uint16_t idx = (uint16_t)((val >> 16) & 0xFFFF);
//    printf("[Debug] Unpacked IndexNum: %u (Raw:0x%08lX)\r\n", idx, val);
    return idx;
}
void Load_Temp_From_Backup(DeviceConfig *cfg)
{
    uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, BKP_TEMP_MIN_MAX);
    cfg->temp_max = (int16_t)(val & 0xFFFF);           // 하위 16비트
    cfg->temp_min = (int16_t)((val >> 16) & 0xFFFF);   // 상위 16비트

    printf("[Backup] Load Temp Max=%d, Min=%d\n", cfg->temp_max, cfg->temp_min);
}


void Load_Dose_From_Backup(DeviceConfig *cfg)
{
    cfg->dose_max = HAL_RTCEx_BKUPRead(&hrtc, BKP_DOSE_MAX);
    printf("[Backup] Load Dose Max=%u (Raw: 0x%08lX)\n", cfg->dose_max, (unsigned long)cfg->dose_max);
}


void Load_Backup_Index(void) {
    // 1. 백업 메모리에서 값 읽기
    uint32_t reg3 = HAL_RTCEx_BKUPRead(&hrtc, BKP_IDX_REG);
    uint32_t reg4 = HAL_RTCEx_BKUPRead(&hrtc, BKP_COUNT_REG);

    idx_rad = (uint16_t)(reg3 & 0xFFFF);
    idx_temp = (uint16_t)((reg3 >> 16) & 0xFFFF);
    rad_interval_count = (uint8_t)(reg4 & 0xFF);

    printf("[DEBUG] Load Backup: idx_rad=%u, idx_temp=%u, rad_interval_count=%u\r\n",
           idx_rad, idx_temp, rad_interval_count);

    // 2. 전역 rad_ratio 계산
    rad_ratio = current_settings.rad_interval / current_settings.temp_interval;
    if (rad_ratio == 0) rad_ratio = 1;

//    printf("[DEBUG] rad_interval=%u, temp_interval=%u, rad_ratio=%u\r\n",
//           current_settings.rad_interval,
//           current_settings.temp_interval,
//           rad_ratio);

    // 3. Rad 측정 여부 판단 및 카운터 처리
    if (rad_interval_count == 0) {
        measure_Rad_flag = 1;                   // 이번 Interval에서 Rad 측정
        printf("[DEBUG] Rad + Temp Measurement scheduled\r\n");
    } else {
        measure_Rad_flag = 0;                   // Rad 측정 없음
        printf("[DEBUG] Temp Only (rad_count=%u)\r\n", rad_interval_count);
    }

}




void Load_MarkAndAlarmState_From_Backup(DeviceConfig *cfg)
{
    uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, BKP_MARK);
    cfg->mark = (uint8_t)(val & 0xFF);
    cfg->alarm_state = (uint32_t)(val >> 8);  // 16비트 전부 사용

    // 알람 상태별 문자열
    const char* alarm_str(uint8_t state) {
        switch(state) {
            case 0: return "OFF";
            case 1: return "ON";
            case 2: return "DIS";
            default: return "?";
        }
    }

    printf("[Debug] Mark=0x%02X | RH1=%s RH2=%s TH1=%s TH2=%s TL1=%s TL2=%s (Raw:0x%08lX)\r\n",
           cfg->mark,
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_RH1) == ALARM_DISABLE ? "DIS" :
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_RH1) == ALARM_ON      ? "ON"  : "OFF",
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_RH2) == ALARM_DISABLE ? "DIS" :
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_RH2) == ALARM_ON      ? "ON"  : "OFF",
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_TH1) == ALARM_DISABLE ? "DIS" :
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_TH1) == ALARM_ON      ? "ON"  : "OFF",
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_TH2) == ALARM_DISABLE ? "DIS" :
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_TH2) == ALARM_ON      ? "ON"  : "OFF",
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_TL1) == ALARM_DISABLE ? "DIS" :
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_TL1) == ALARM_ON      ? "ON"  : "OFF",
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_TL2) == ALARM_DISABLE ? "DIS" :
           GET_ALARM_STATE(cfg->alarm_state, ALARM_STATE_POS_TL2) == ALARM_ON      ? "ON"  : "OFF",
           cfg->alarm_state);   // ⬅️ 여기 Raw는 언팩된 alarm_state를 출력해야 함

}



void Load_IntervalInfo_From_Backup(DeviceConfig *cfg)
{
    uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, BKP_INTERVAL_INFO);
    cfg->interval_time = (uint16_t)(val & 0xFFFF);
    cfg->display_temp = (uint8_t)((val >> 16) & 0xFF);
    cfg->display_dose = (uint8_t)((val >> 24) & 0xFF);
    printf("[Debug] Unpacked IntervalInfo: Interval=%u, DispTemp=%u, DispDose=%u (Raw:0x%08lX)\r\n", current_settings.temp_interval, cfg->display_temp, cfg->display_dose, val);
}




uint32_t Load_Pre_RAD_Value_From_Backup(void)
{
    uint32_t value = HAL_RTCEx_BKUPRead(&hrtc, BKP_PRE_RAD_VALUE);
    printf("[Backup] Load Pre_RAD_Value = %lu (Raw: 0x%08lX)\r\n", value, value);
    return value;
}



void Load_All_Config_From_BackupRegister(DeviceConfig *cfg)
{
    Load_Temp_From_Backup(cfg);
    Load_Dose_From_Backup(cfg);
    Load_Backup_Index();
    Load_MarkAndAlarmState_From_Backup(cfg);
    Load_IntervalInfo_From_Backup(cfg);
    Index_num = Load_IndexNum_From_Backup();
//    Load_Pre_RAD_Value_From_Backup();
}

void Update_TempDose_MinMax(float temp_avg, uint32_t dose_now, DeviceConfig *cfg)
{
    int16_t new_temp = (int16_t)(temp_avg * 10);
    uint32_t new_dose = (uint32_t)dose_now;
    bool update = false;

    // 이전 값들 출력
    printf("[Backup][Prev] Tmax:%d Tmin:%d Dmax:%u\n", cfg->temp_max, cfg->temp_min, cfg->dose_max);
    printf("[Backup][New Input] Temp:%d Dose:%u\n", new_temp, new_dose);

    // 최초만 동시에 저장 Debug 필요
    if (cfg->temp_max == INT16_MIN || cfg->temp_min == INT16_MAX || cfg->dose_max == 0xFFFFFFFF) {
        cfg->temp_max = new_temp;
        cfg->temp_min = new_temp;
        cfg->dose_max = new_dose;
        update = true;
    } else {
        if (new_temp >= cfg->temp_max) {
            printf("  [Update] temp_max: %d -> %d\n", cfg->temp_max, new_temp);
            cfg->temp_max = new_temp;
            update = true;
        }
        if (new_temp < cfg->temp_min) {
            printf("  [Update] temp_min: %d -> %d\n", cfg->temp_min, new_temp);
            cfg->temp_min = new_temp;
            update = true;
        }
        if (temp_avg <= 50.0f && new_dose > cfg->dose_max) {
            printf("  [Update] dose_max: %u -> %u\n", cfg->dose_max, new_dose);
            cfg->dose_max = new_dose;
            update = true;
        }
    }

    if (update) {
        Save_Temp_To_Backup(cfg);
        Save_Dose_To_Backup(cfg);
        printf("[Backup][Updated] Tmax:%d Tmin:%d Dmax:%u\n", cfg->temp_max, cfg->temp_min, cfg->dose_max);
    }
}




//uint8_t Check_Rad_Measure_Need(uint8_t *rad_interval_count) {
//    // rad_ratio 계산 (내부 Flash 값 기반)
//    rad_ratio = current_settings.rad_interval / current_settings.temp_interval;
//
//    if (rad_ratio == 0) rad_ratio = 1;
//
//    if (*rad_interval_count == 0) {
//        measure_Rad_flag = 1;                        // 이번 Interval에서 Rad 측정
//    } else {
//        measure_Rad_flag = 0;
//    }
//
////    return measure_Rad_flag;
//}

void Set_Wakeup_After_Delay(uint32_t delay_sec,
                            const RTC_TimeTypeDef *sTime_bcd,
                            const RTC_DateTypeDef *sDate_bcd)
{
    // BCD → BIN 변환
    uint16_t y2000 = (uint16_t)BCD2BIN(sDate_bcd->Year);
    uint8_t  mon   = (uint8_t) BCD2BIN(sDate_bcd->Month);
    uint8_t  day   = (uint8_t) BCD2BIN(sDate_bcd->Date);
    uint8_t  hh    = (uint8_t) BCD2BIN(sTime_bcd->Hours);
    uint8_t  mm    = (uint8_t) BCD2BIN(sTime_bcd->Minutes);
    uint8_t  ss    = (uint8_t) BCD2BIN(sTime_bcd->Seconds);

    // 현재 epoch
    uint32_t now_epoch = ymd_to_epoch(y2000, mon, day, hh, mm, ss);

    // 최소/최대 가드
    if (delay_sec == 0) delay_sec = 1;
    const uint32_t max_delay = 31u * 86400u; // 최대 31일 (요구사항)
    if (delay_sec > max_delay) delay_sec = max_delay;

    // 목표 epoch
    uint32_t target_epoch = now_epoch + delay_sec;

    // BKP에 epoch 저장
    Save_WakeupTime_To_Backup(target_epoch);

    // AlarmA 설정
    RTC_Disable_All_Wakeup();
    RTC_SetAlarmA_SecondsFromNow(delay_sec);

    printf("[Delay] now=%04u-%02u-%02u %02u:%02u:%02u, after=%lu s → target(epoch)=%lu\r\n",
           (unsigned)(2000 + y2000), (unsigned)mon, (unsigned)day,
           (unsigned)hh, (unsigned)mm, (unsigned)ss,
           (unsigned long)delay_sec, (unsigned long)target_epoch);
}

void Reset_All_Backup_Registers(void)
{
    HAL_PWR_EnableBkUpAccess();
    Set_ModeStatus(mode_internal_backup_stop);
    // 주요 Backup Register 모두 0으로 초기화
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_DOSE_MAX,        0x00000000);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_MODE_REG,        0x00000000);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_TEMP_MIN_MAX,    0x00000000);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_IDX_REG,         0x00000000);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_COUNT_REG,       0x00000000);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_MARK,            0x00000000);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_WAKEUP_TIME,     0xFFFFFFFF);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_INTERVAL_INFO,   0x00000000);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_PRE_RAD_VALUE,   0x00000000);

    device_config.temp_max = INT16_MIN;
    device_config.temp_min = INT16_MAX;
    device_config.dose_max = 0xFFFFFFFF;
    device_config.mark = 0;
    device_config.alarm_state = 0;
    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1, ALARM_OFF);
    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2, ALARM_OFF);
    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1, ALARM_OFF);
    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2, ALARM_OFF);
    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1, ALARM_OFF);
    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2, ALARM_OFF);

    Save_All_Config_To_BackupRegister(&device_config);
    Switch_Backup_reg(MODE_Boot);
    idx_rad = 0;
    idx_temp = 0;
    current_settings.start_mode = 0x00;
    current_settings.start_time_info = 0x00;
    current_settings.start_target_seconds = 0x00;  // start_delay 적용된 RTC 기준 초 시각
    current_settings.start_reservation_time= 0x00;
    current_settings.interval_duration_day= 0x00;
    current_settings.end_target_seconds= 0x00;
    current_settings.start_time_info_for_alarm = 0x00;
    Save_CurrentSettings();

    printf("[RTC] All Backup Registers RESET (0x00000000)\r\n");
}

/////////////////////Measure Routine////////////////////

void Short_Measure(void){
	Count_Filter_Kalman();
	Read_Temp();
	if (Tick_Save>7){
	  Get_in_Shutdown();
	  Tick_Save = 0;
	}
	printf("Tick_Save : %lu\r\n", Tick_Save);
     comp1_count = 0;
	 Tick_Save++;
}

void Non_Measure(void){

	if (Tick_Save>7){
	  Get_in_Shutdown();
	  Tick_Save = 0;
	}
	printf("Tick_Save : %lu\r\n", Tick_Save);
     comp1_count = 0;
	 Tick_Save++;
}
//
//void Non_Measure_RTC_CLEAR(void){
//
//	if (Tick_Save>7){
//RTC_Disable_All_Wakeup();
//	  Get_in_Shutdown();
//	  Tick_Save = 0;
//	}
//	printf("Tick_Save : %lu\r\n", Tick_Save);
//     comp1_count = 0;
//	 Tick_Save++;
//}


void Interval_Measure(void){
	 if (measure_Rad_flag) {
	Count_Filter_Kalman();
	printf("Tick_Save : %lu, Interval_LCD_Count : %lu\r\n", Tick_Save, Interval_LCD_Count);
//	PrintCurrentRTC();
	if (Check_And_Save_When_Target_Reached() && Interval_LCD_Count == 0){
		printf("Get in Timer Shut Down\r\n");
		Read_Temp();
		Update_TempDose_MinMax(Display_temperature, dose_log, &device_config);
		Check_And_Run_Alarms(Display_temperature, dose_log);
	    Write_buffer();
	    rad_interval_count = rad_ratio - 1;     // 카운터 리셋
	    idx_temp++;
	    idx_rad++;
	    Save_Backup_Index();
	    Get_in_Shutdown_Timer();
	    Tick_Save = 0;
	}

	else if(Check_And_Save_When_Target_Reached() && Interval_LCD_Count>0) {
		printf("Start Timer & No shutdown\r\n");
		Read_Temp();
		Update_TempDose_MinMax(Display_temperature, dose_log, &device_config);
		Check_And_Run_Alarms(Display_temperature, dose_log);
	    Write_buffer();
	    rad_interval_count = rad_ratio - 1;     // 카운터 리셋
	    idx_temp++;
	    idx_rad++;
	    Save_Backup_Index();
		Switch_Backup_reg(MODE_Start_Button);
		Start_Wake_Timer();
	}
	 comp1_count = 0;
	 Tick_Save++;
	 }

	 else{
		 if(Check_And_Save_When_Target_Reached()){
		 Read_Temp();
		 PrintCurrentRTC();
		 dose_log = 0x0000;
		 printf("[Debug] Display_temperature = %.2f°C\n", Display_temperature);
		 Check_And_Run_Alarms(Display_temperature, dose_log);
			if (Interval_LCD_Count == 0){
//				Check_And_Run_Alarms(Display_temperature, dose_log, &current_settings);
				printf("Get in Timer Shut Down\r\n");
				Update_TempDose_MinMax(Display_temperature, dose_log, &device_config);
				dose_log = 0x0000;
			    Write_buffer();
			    rad_interval_count--;
			    idx_temp++;
			    Save_Backup_Index();
			    Get_in_Shutdown_Timer();
			    Tick_Save = 0;
			}
			else if(Interval_LCD_Count>0) {
//				Check_And_Run_Alarms(Display_temperature, dose_log, &current_settings);
				printf("Start Timer & No shutdown\r\n");
				Update_TempDose_MinMax(Display_temperature, dose_log, &device_config);
				dose_log = 0x0000;
				rad_interval_count--;
			    Write_buffer();
			    idx_temp++;
			    Save_Backup_Index();
				Switch_Backup_reg(MODE_Start_Button);
				Start_Wake_Timer();
			}
		 }
			 comp1_count = 0;
			 Tick_Save++;
	 }

}


////////////////////////////Alarm Function/////////////////////
// N초 뒤에 알람 A 한 번 울리게
void RTC_SetAlarmA_AfterSeconds(uint32_t after_sec) {
    RTC_TimeTypeDef t; RTC_DateTypeDef d;
    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);

    uint32_t now = t.Hours*3600u + t.Minutes*60u + t.Seconds;
    uint32_t tgt = (now + after_sec) % 86400u;

    RTC_AlarmTypeDef a = {0};
    a.AlarmTime.Hours   = tgt / 3600u;
    a.AlarmTime.Minutes = (tgt % 3600u) / 60u;
    a.AlarmTime.Seconds = tgt % 60u;
    a.AlarmMask         = RTC_ALARMMASK_DATEWEEKDAY;       // 날짜 무시(오늘/내일 자동)
    a.AlarmSubSecondMask= RTC_ALARMSUBSECONDMASK_ALL;
    a.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    a.AlarmDateWeekDay  = 1;
    a.Alarm             = RTC_ALARM_A;

    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
    if (HAL_RTC_SetAlarm_IT(&hrtc, &a, RTC_FORMAT_BIN) != HAL_OK) Error_Handler();
}

//void Check_And_Run_Alarms(float temp_avg, uint32_t dose)
//{
//    // ──[1] 딜레이 게이트 계산 ─────────────────────────────────────────────
//    bool gate_rh1 = true, gate_rh2 = true, gate_th1 = true, gate_th2 = true, gate_tl1 = true, gate_tl2 = true;
//    bool delay_done_skip = false; // true면 시간 비교 스킵(이미 모든 딜레이 종료 상태)
//
//    // Flash에 저장된 Start 기준초(0~86399) 사용, 0xFFFFFFFF면 이미 종료 상태
//    // Load_CurrentSettings(); // current_settings는 RAM 캐시를 사용한다고 가정
//    if (current_settings.start_time_info_for_alarm == 0xFFFFFFFFUL) {
//        delay_done_skip = true; // 모든 딜레이 지남 → 비교 스킵
//    } else if (current_settings.start_time_info_for_alarm >= 86400UL) {
//        // [가드] 비정상 값이면 즉시 현재 시각으로 초기화 후 저장
//        RTC_TimeTypeDef t; RTC_DateTypeDef d;
//        HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
//        HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
//        uint32_t now_s_fix = (uint32_t)t.Hours * 3600u + (uint32_t)t.Minutes * 60u + (uint32_t)t.Seconds;
//
//        current_settings.start_time_info_for_alarm = now_s_fix;
//        Save_CurrentSettings();
//
//        printf("[AlarmDelay][Fix] start_time_info invalid -> reset to %lu sec\r\n",
//               (unsigned long)now_s_fix);
//
//        // 이 호출 사이클에서는 비교를 건너뛰어 안전하게 다음 사이클부터 정상 동작
//        delay_done_skip = false;
//        gate_rh1 = gate_rh2 = gate_th1 = gate_th2 = gate_tl1 = gate_tl2 = false;
//    }  else {
//        // RTC 현재 초
//        RTC_TimeTypeDef t; RTC_DateTypeDef d;
//        HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
//        HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
//        uint32_t now_s = (uint32_t)t.Hours * 3600u + (uint32_t)t.Minutes * 60u + (uint32_t)t.Seconds;
//
//        uint32_t start_s = current_settings.start_time_info_for_alarm; // 0~86399
//        uint32_t elapsed = (now_s >= start_s) ? (now_s - start_s) : (86400u - start_s + now_s);
//
//        // 알람별 Delay(초) 임계
//        uint16_t d_rh1 = current_settings.alarm_delay_rh1;
//        uint16_t d_rh2 = current_settings.alarm_delay_rh2;
//        uint16_t d_th1 = current_settings.alarm_delay_th1;
//        uint16_t d_th2 = current_settings.alarm_delay_th2;
//        uint16_t d_tl1 = current_settings.alarm_delay_tl1;
//        uint16_t d_tl2 = current_settings.alarm_delay_tl2;
//
//        gate_rh1 = (elapsed >= d_rh1);
//        gate_rh2 = (elapsed >= d_rh2);
//        gate_th1 = (elapsed >= d_th1);
//        gate_th2 = (elapsed >= d_th2);
//        gate_tl1 = (elapsed >= d_tl1);
//        gate_tl2 = (elapsed >= d_tl2);
//
//        // 모든 딜레이 경과 시 센티넬 기록 → 이후부터 비교 스킵
//        uint16_t dmax = d_rh1;
//        if (d_rh2 > dmax) dmax = d_rh2;
//        if (d_th1 > dmax) dmax = d_th1;
//        if (d_th2 > dmax) dmax = d_th2;
//        if (d_tl1 > dmax) dmax = d_tl1;
//        if (d_tl2 > dmax) dmax = d_tl2;
//
//        if (elapsed >= dmax) {
//            current_settings.start_time_info_for_alarm = 0xFFFFFFFFUL; // 완료 플래그
//            Save_CurrentSettings(); // Flash에 1회 기록
//            delay_done_skip = true;
//            printf("[AlarmDelay] All delays passed (elapsed=%lu >= %u). Skip time gate hereafter.\r\n",
//                   (unsigned long)elapsed, dmax);
//        } else {
//            printf("[AlarmDelay] elapsed=%lu  RH1:%u(%d) RH2:%u(%d) TH1:%u(%d) TH2:%u(%d) TL1:%u(%d) TL2:%u(%d)\r\n",
//                   (unsigned long)elapsed,
//                   d_rh1, gate_rh1, d_rh2, gate_rh2, d_th1, gate_th1,
//                   d_th2, gate_th2, d_tl1, gate_tl1, d_tl2, gate_tl2);
//        }
//    }
//
//    // ──[2] 기존 알람 평가 로직 (게이트 적용) ─────────────────────────────
//    (void)temp_avg; // 현재는 Display_temperature를 사용하므로 파라미터 미사용 처리
//    (void)dose;     // 현재는 dose_log를 사용
//
//    int16_t  temp_x10 = (int16_t)(Display_temperature * 10.0f);
//    uint16_t dose_now = dose_log;
//
//    printf("[Alarm Debug] temp_avg=%.2f (x10=%d), dose=%u\r\n",
//           Display_temperature, temp_x10, dose_now);
//
//    // Radiation High1
//    if (delay_done_skip || gate_rh1) {
//        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1);
//        if (st != ALARM_DISABLE) {
//            if (dose_now >= current_settings.alarm_rh1) {
//                if (st != ALARM_ON) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1, ALARM_ON);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Radiation High1 -> ON\r\n");
//                }
//            } else {
//                if (st != ALARM_OFF) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1, ALARM_OFF);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Radiation High1 -> OFF\r\n");
//                }
//            }
//        }
//    } else {
//        printf("[AlarmDelay] RH1 waiting...\r\n");
//    }
//
//    // Radiation High2
//    if (delay_done_skip || gate_rh2) {
//        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2);
//        if (st != ALARM_DISABLE) {
//            if (dose_now >= current_settings.alarm_rh2) {
//                if (st != ALARM_ON) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2, ALARM_ON);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Radiation High2 -> ON\r\n");
//                }
//            } else {
//                if (st != ALARM_OFF) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2, ALARM_OFF);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Radiation High2 -> OFF\r\n");
//                }
//            }
//        }
//    } else {
//        printf("[AlarmDelay] RH2 waiting...\r\n");
//    }
//
//    // Temperature High1
//    if (delay_done_skip || gate_th1) {
//        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1);
//        if (st != ALARM_DISABLE) {
//            if (temp_x10 >= current_settings.alarm_th1) {
//                if (st != ALARM_ON) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1, ALARM_ON);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Temp High1 -> ON\r\n");
//                }
//            } else {
//                if (st != ALARM_OFF) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1, ALARM_OFF);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Temp High1 -> OFF\r\n");
//                }
//            }
//        }
//    } else {
//        printf("[AlarmDelay] TH1 waiting...\r\n");
//    }
//
//    // Temperature High2
//    if (delay_done_skip || gate_th2) {
//        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2);
//        if (st != ALARM_DISABLE) {
//            if (temp_x10 >= current_settings.alarm_th2) {
//                if (st != ALARM_ON) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2, ALARM_ON);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Temp High2 -> ON\r\n");
//                }
//            } else {
//                if (st != ALARM_OFF) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2, ALARM_OFF);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Temp High2 -> OFF\r\n");
//                }
//            }
//        }
//    } else {
//        printf("[AlarmDelay] TH2 waiting...\r\n");
//    }
//
//    // Temperature Low1
//    if (delay_done_skip || gate_tl1) {
//        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1);
//        if (st != ALARM_DISABLE) {
//            if (temp_x10 <= current_settings.alarm_tl1) {
//                if (st != ALARM_ON) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1, ALARM_ON);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Temp Low1 -> ON\r\n");
//                }
//            } else {
//                if (st != ALARM_OFF) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1, ALARM_OFF);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Temp Low1 -> OFF\r\n");
//                }
//            }
//        }
//    } else {
//        printf("[AlarmDelay] TL1 waiting...\r\n");
//    }
//
//    // Temperature Low2
//    if (delay_done_skip || gate_tl2) {
//        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2);
//        if (st != ALARM_DISABLE) {
//            if (temp_x10 <= current_settings.alarm_tl2) {
//                if (st != ALARM_ON) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2, ALARM_ON);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Temp Low2 -> ON\r\n");
//                }
//            } else {
//                if (st != ALARM_OFF) {
//                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2, ALARM_OFF);
//                    Save_MarkAndAlarmState_To_Backup(&device_config);
//                    printf("[Alarm] Temp Low2 -> OFF\r\n");
//                }
//            }
//        }
//    } else {
//        printf("[AlarmDelay] TL2 waiting...\r\n");
//    }
//
//    printf("[Debug] Check_And_Run_Alarms: End\r\n");
//}

void Check_And_Run_Alarms(float temp_avg, uint32_t dose)
{
    // ──[1] 알람 딜레이 게이트: epoch 기반으로 변경 ───────────────────────
    bool gate_rh1 = true, gate_rh2 = true, gate_th1 = true, gate_th2 = true, gate_tl1 = true, gate_tl2 = true;
    bool delay_done_skip = false;

    // Flash → RAM 캐시(current_settings)는 이미 유지된다고 가정
    uint32_t start_epoch = current_settings.start_time_info_for_alarm;

    if (start_epoch == 0xFFFFFFFFu) {
        // 모든 딜레이 종료 후 다시는 게이트하지 않음
        delay_done_skip = true;
    } else {
        // 현재 epoch
        RTC_TimeTypeDef t; RTC_DateTypeDef d;
        HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
        uint32_t now_epoch = ymd_to_epoch(d.Year, d.Month, d.Date, t.Hours, t.Minutes, t.Seconds);

        // 경과 시간(음수 방지 가드)
        uint32_t elapsed = (now_epoch >= start_epoch) ? (now_epoch - start_epoch) : 0u;

        // 알람별 Delay(초)
        uint16_t d_rh1 = current_settings.alarm_delay_rh1;
        uint16_t d_rh2 = current_settings.alarm_delay_rh2;
        uint16_t d_th1 = current_settings.alarm_delay_th1;
        uint16_t d_th2 = current_settings.alarm_delay_th2;
        uint16_t d_tl1 = current_settings.alarm_delay_tl1;
        uint16_t d_tl2 = current_settings.alarm_delay_tl2;

        gate_rh1 = (elapsed >= d_rh1);
        gate_rh2 = (elapsed >= d_rh2);
        gate_th1 = (elapsed >= d_th1);
        gate_th2 = (elapsed >= d_th2);
        gate_tl1 = (elapsed >= d_tl1);
        gate_tl2 = (elapsed >= d_tl2);

        // 모든 딜레이 통과 시 센티넬 기록
        uint16_t dmax = d_rh1;
        if (d_rh2 > dmax) dmax = d_rh2;
        if (d_th1 > dmax) dmax = d_th1;
        if (d_th2 > dmax) dmax = d_th2;
        if (d_tl1 > dmax) dmax = d_tl1;
        if (d_tl2 > dmax) dmax = d_tl2;

        if (elapsed >= dmax) {
            current_settings.start_time_info_for_alarm = 0xFFFFFFFFu;
            Save_CurrentSettings(); // 1회 저장
            delay_done_skip = true;
            printf("[AlarmDelay] epoch gate done: elapsed=%lu >= %u, sentinel set.\r\n",
                   (unsigned long)elapsed, dmax);
        } else {
            printf("[AlarmDelay] elapsed=%lu(s)  RH1:%u RH2:%u TH1:%u TH2:%u TL1:%u TL2:%u\r\n",
                   (unsigned long)elapsed, gate_rh1, gate_rh2, gate_th1, gate_th2, gate_tl1, gate_tl2);
        }
    }

    // ──[2] 원래 알람 비교 로직(게이트 적용)은 그대로 ───────────────────────
    (void)temp_avg; // 현재는 Display_temperature / dose_log 사용
    (void)dose;

    int16_t  temp_x10 = (int16_t)(Display_temperature * 10.0f);
    uint16_t dose_now = dose_log;

    printf("[Alarm Debug] temp_avg=%.2f (x10=%d), dose=%u\r\n",
           Display_temperature, temp_x10, dose_now);

    // Radiation High1
    if (delay_done_skip || gate_rh1) {
        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1);
        if (st != ALARM_DISABLE) {
            if (dose_now >= current_settings.alarm_rh1) {
                if (st != ALARM_ON) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1, ALARM_ON);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Radiation High1 -> ON\r\n");
                }
            } else {
                if (st != ALARM_OFF) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1, ALARM_OFF);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Radiation High1 -> OFF\r\n");
                }
            }
        }
    } else {
        printf("[AlarmDelay] RH1 waiting...\r\n");
    }

    // Radiation High2
    if (delay_done_skip || gate_rh2) {
        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2);
        if (st != ALARM_DISABLE) {
            if (dose_now >= current_settings.alarm_rh2) {
                if (st != ALARM_ON) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2, ALARM_ON);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Radiation High2 -> ON\r\n");
                }
            } else {
                if (st != ALARM_OFF) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2, ALARM_OFF);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Radiation High2 -> OFF\r\n");
                }
            }
        }
    } else {
        printf("[AlarmDelay] RH2 waiting...\r\n");
    }

    // Temperature High1
    if (delay_done_skip || gate_th1) {
        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1);
        if (st != ALARM_DISABLE) {
            if (temp_x10 >= current_settings.alarm_th1) {
                if (st != ALARM_ON) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1, ALARM_ON);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Temp High1 -> ON\r\n");
                }
            } else {
                if (st != ALARM_OFF) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1, ALARM_OFF);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Temp High1 -> OFF\r\n");
                }
            }
        }
    } else {
        printf("[AlarmDelay] TH1 waiting...\r\n");
    }

    // Temperature High2
    if (delay_done_skip || gate_th2) {
        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2);
        if (st != ALARM_DISABLE) {
            if (temp_x10 >= current_settings.alarm_th2) {
                if (st != ALARM_ON) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2, ALARM_ON);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Temp High2 -> ON\r\n");
                }
            } else {
                if (st != ALARM_OFF) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2, ALARM_OFF);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Temp High2 -> OFF\r\n");
                }
            }
        }
    } else {
        printf("[AlarmDelay] TH2 waiting...\r\n");
    }

    // Temperature Low1
    if (delay_done_skip || gate_tl1) {
        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1);
        if (st != ALARM_DISABLE) {
            if (temp_x10 <= current_settings.alarm_tl1) {
                if (st != ALARM_ON) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1, ALARM_ON);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Temp Low1 -> ON\r\n");
                }
            } else {
                if (st != ALARM_OFF) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1, ALARM_OFF);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Temp Low1 -> OFF\r\n");
                }
            }
        }
    } else {
        printf("[AlarmDelay] TL1 waiting...\r\n");
    }

    // Temperature Low2
    if (delay_done_skip || gate_tl2) {
        AlarmState st = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2);
        if (st != ALARM_DISABLE) {
            if (temp_x10 <= current_settings.alarm_tl2) {
                if (st != ALARM_ON) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2, ALARM_ON);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Temp Low2 -> ON\r\n");
                }
            } else {
                if (st != ALARM_OFF) {
                    SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2, ALARM_OFF);
                    Save_MarkAndAlarmState_To_Backup(&device_config);
                    printf("[Alarm] Temp Low2 -> OFF\r\n");
                }
            }
        }
    } else {
        printf("[AlarmDelay] TL2 waiting...\r\n");
    }

    printf("[Debug] Check_And_Run_Alarms: End\r\n");
}


void Alarm_Radiation_High1(void) {
    printf("[Alarm] Radiation High 1 Triggered!\r\n");
}


void Alarm_Radiation_High2(void) {
    printf("[Alarm] Radiation High 2 Triggered!\r\n");
}


void Alarm_Temperature_High1(void) {
    printf("[Alarm] Alarm_Temperature_High1 Triggered!\r\n");
}


void Alarm_Temperature_High2(void) {
    printf("[Alarm] Alarm_Temperature_High2 Triggered!\r\n");
}

void Alarm_Temperature_Low1(void) {
    printf("[Alarm] Alarm_Temperature_Low 1 Triggered!\r\n");
}


void Alarm_Temperature_Low2(void) {
    printf("[Alarm] Alarm_Temperature_Low 2 Triggered!\r\n");
}


////////////////Debug Function

void send_device_setting_example(void) {
//    DeviceSettingPacket_t pkt = {0};
//    pkt.start[0] = DEVICE_PACKET_START_0;
//    pkt.start[1] = DEVICE_PACKET_START_1;
//    pkt.cmd_id   = 0x10;           // 예시 명령어
//    pkt.len      = 3;
//    pkt.data[0]  = 0xAA;
//    pkt.data[1]  = 0xBB;
//    pkt.data[2]  = 0xCC;
//    pkt.checksum = DeviceSetting_CalcChecksum(&pkt);
//    pkt.end[0]   = DEVICE_PACKET_END_0;
//    pkt.end[1]   = DEVICE_PACKET_END_1;
//
//    DeviceSetting_Send(&pkt);  // 송신
}


int  send_log_entry_usb(const log_entry_t *e)
{
    DeviceSettingPacket_t pkt = {0};
    pkt.start[0] = DEVICE_PACKET_START_0;
    pkt.start[1] = DEVICE_PACKET_START_1;
    pkt.cmd_id   = 0x21;   // 예시: Log 송신 명령
    pkt.len      = 17;

    // [Data Index (2 bytes)] (LSB, MSB)
    pkt.data[0] = e->index & 0xFF;
    pkt.data[1] = (e->index >> 8) & 0xFF;
    // [Timestamp (6 bytes)]
    pkt.data[2] = e->year;
    pkt.data[3] = e->month;
    pkt.data[4] = e->day;
    pkt.data[5] = e->hour;
    pkt.data[6] = e->minute;
    pkt.data[7] = e->second;
    // [Temp (2 bytes)] (LSB, MSB)
    pkt.data[8]  = e->temperature & 0xFF;
    pkt.data[9]  = (e->temperature >> 8) & 0xFF;
    // [CPS (2 bytes)] (LSB, MSB)
    pkt.data[10] = e->count & 0xFF;
    pkt.data[11] = (e->count >> 8) & 0xFF;
    // [Dose (4 bytes)] (LSB, ..., MSB)
    pkt.data[12] = e->dose & 0xFF;
    pkt.data[13] = (e->dose >> 8) & 0xFF;
    pkt.data[14] = (e->dose >> 16) & 0xFF;
    pkt.data[15] = (e->dose >> 24) & 0xFF;
    // [Mark (1 byte)]
    pkt.data[16] = e->mark;

    //FIXME: Change to CRC8_STANDARD func.
    pkt.checksum = DeviceSetting_CalcChecksum(&pkt);

    return DeviceSetting_Send(&pkt);
}

void send_all_log_entries_usb(void)
{
  uint8_t send_buff[64] = {0};

  for (int i = 0; i < 512; i++)
  {
    send_buff[0] = i;
    printf("Send:hid: %d\r\n",i);
    if(USB_Send_HidReport(send_buff,64)) {
      printf("send hid report %d failed\r\n",i);
    }
     for (volatile int d = 0; d < 20000; d++)
      {
        __NOP(); // No operation instruction
      }
  }
#if 0
    log_entry_t entry;
    uint32_t max_idx = LOG_MAX_SIZE / ENTRY_SIZE;

    printf("[USB] LOG_MAX_SIZE=%lu, ENTRY_SIZE=%lu, max_idx=%lu\r\n",
               (unsigned long)LOG_MAX_SIZE, (unsigned long)ENTRY_SIZE, (unsigned long)max_idx);
   for (uint32_t i = 0; i < max_idx; i++)
    // for (uint32_t i = 0; i < 30; i++)
    {
        // meas_data_log_read_entry(i, &entry);
        // printf("[USB] idx=%lu year=%u index=%u\n", (unsigned long)i, entry.year, entry.index);

        // int ret = send_log_entry_usb(&entry);
        // printf("[USB] send_log_entry_usb() ret = %d\n", ret);

//        HAL_Delay(2);
       meas_data_log_read_entry(i, &entry);
       // 비어있으면 종료 (index가 0xFFFF 또는 0xFF == 미기록)
       if (entry.year == 0xFF) break;
       int ret = send_log_entry_usb(&entry);
       printf("[USB] send_log_entry_usb() ret = %d\n", ret);
//        HAL_Delay(2); // 필요에 따라 속도 조절 (최소 1~2ms)
    }
    printf("Send All Flash Data\r\n");
#endif
}



//#define USB_BURST_SIZE   15      // 한 번에 보낼 개수
//#define BURST_DELAY_MS   100     // burst마다 대기시간(ms)
//
//void send_all_log_entries_usb(void)
//{
//    log_entry_t entry;
//    uint32_t max_idx = LOG_MAX_SIZE / ENTRY_SIZE;
//    uint32_t burst_count = 0;
//
//    printf("[USB] LOG_MAX_SIZE=%lu, ENTRY_SIZE=%lu, max_idx=%lu\r\n",
//           (unsigned long)LOG_MAX_SIZE, (unsigned long)ENTRY_SIZE, (unsigned long)max_idx);
//
//    // for (uint32_t i = 0; i < max_idx; i++)
//    for (uint32_t i = 0; i < 15; i++)
//    {
//        meas_data_log_read_entry(i, &entry);
//
//        // 빈 엔트리는 skip
//        if (entry.year == 0xFF) continue;
//
//        printf("[USB] idx=%lu year=%u index=%u\n", (unsigned long)i, entry.year, entry.index);
//
//        // 패킷 송신 (ret 값 확인)
//        int ret = send_log_entry_usb(&entry);
//        printf("[USB] send_log_entry_usb() ret = %d\n", ret);
//
//        burst_count++;
//        if (burst_count >= USB_BURST_SIZE) {
//            HAL_Delay(BURST_DELAY_MS); // burst마다 대기
//            burst_count = 0;
//        }
//        // 만약 항상 빠르게 보내야 한다면 HAL_Delay(2); 등 추가 가능
//    }
//    printf("Send All Flash Data\r\n");
//}

// rad_usb.c 혹은 main.c 중 원하는 위치(함수 선언부)
//void send_all_log_entries_usb(void)
//{
//    log_entry_t entry;
//    uint32_t max_idx = LOG_MAX_SIZE / ENTRY_SIZE;
//    uint32_t burst_count = 0;
//    uint32_t sent = 0;
//
//    printf("[USB] LOG_MAX_SIZE=%lu, ENTRY_SIZE=%lu, max_idx=%lu\r\n",
//           (unsigned long)LOG_MAX_SIZE, (unsigned long)ENTRY_SIZE, (unsigned long)max_idx);
//
//    for (uint32_t i = 0; i < max_idx; i++)
//    {
//        meas_data_log_read_entry(i, &entry);
//
//        if (entry.year == 0xFF) continue;
//
//        int ret = send_log_entry_usb(&entry); // 1개씩 송신
//        sent++;
//        burst_count++;
//
//        printf("[USB] idx=%lu year=%u index=%u | ret=%d\n", (unsigned long)i, entry.year, entry.index, ret);
//
//        // 15개씩 전송 후 ACK 기다림
//        if (burst_count >= 15) {
//            usb_ack_received = 0; // flag 초기화
//            printf("[USB] Waiting for host ACK...\n");
//            uint32_t wait_tick = HAL_GetTick();
//            while (!usb_ack_received) {
//                // 최대 5초까지 대기 (timeout)
//                if (HAL_GetTick() - wait_tick > 5000) {
//                    printf("[USB] Host ACK Timeout\n");
//                    break;
//                }
//                HAL_Delay(1);
//            }
//            burst_count = 0;
//        }
//    }
//    printf("Send All Flash Data (Total sent: %lu)\r\n", (unsigned long)sent);
//}


// 전체 로그 개수를 미리 계산 (최초 요청 시)
void start_log_usb_transfer(void) {
    log_send_idx = 0;
    max_log_idx = LOG_MAX_SIZE / ENTRY_SIZE;
}

// 15개씩만 전송
void send_next_log_entries_usb(void)
{
    log_entry_t entry;
    uint32_t sent_count = 0;

    // 실제로 빈 엔트리 검사하고 끝내도록
    while (sent_count < 15 && log_send_idx < max_log_idx) {
        meas_data_log_read_entry(log_send_idx, &entry);

        // 엔트리가 비면 중단
        if (entry.year == 0xFF) break;

        send_log_entry_usb(&entry);
        log_send_idx++;
        sent_count++;
    }
    // printf 등으로 현재 진행상황 표시 가능
    printf("[USB] %lu/%lu log entries sent\r\n", (unsigned long)log_send_idx, (unsigned long)max_log_idx);
}




//void USB_HID_Receive(uint8_t* data, ULONG* len)
//{
//    USB_Get_HidReport(data, len);
//    for (int i = 0; i < *len; i++) {
//        printf("%d ", data[i]);
//    }
//    printf("\n");
//}

/////////////////////////////////////////////
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Change io to uart3.
int __io_putchar(int ch)
{
  if (ch == '\n') {
    uint8_t ret = '\r';
    HAL_UART_Transmit(&huart3, &ret, 1, 0xFFFF);
  }
  HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}
extern volatile int usb_hid_send_all_datas_flag;
extern volatile int usbx_host_req_records;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
   SystemClock_Config();
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  HAL_NVIC_SetPriority(RTC_TAMP_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(RTC_TAMP_IRQn);

  /* USER CODE END Init */

  /* Configure the system clock */
//  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_LCD_Init();

//  MX_RTC_Init();
  MX_SPI3_Init();
  MX_USART3_UART_Init();
  MX_COMP1_Init();
  MX_LPTIM1_Init();
  /* USER CODE BEGIN 2 */
  LCD_Clear_Display(LCD_data);
  HAL_PWR_EnableBkUpAccess();
  PVD_Config(PWR_PVDLEVEL_0);

//  for (int i = 0; i < 10; i++) {
//      printf("BKP DR%d = 0x%04lX\r\n", i, HAL_RTCEx_BKUPRead(&hrtc, i));
//  }
  if ( HAL_RTCEx_BKUPRead(&hrtc, BKP_MODE_REG) == 0x0000)
    {
      MX_RTC_Init();
      device_config.temp_max = INT16_MIN;
      device_config.temp_min = INT16_MAX;
      device_config.dose_max = 0xFFFFFFFF;
      device_config.mark = 0;
      device_config.alarm_state = 0;
      SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1, ALARM_DISABLE);
      SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2, ALARM_OFF);
      SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1, ALARM_DISABLE);
      SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2, ALARM_OFF);
      SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1, ALARM_DISABLE);
      SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2, ALARM_OFF);

      printf("[Debug][Set] alarm_state Raw: 0x%08lX\n", device_config.alarm_state);
      printf("[Debug][Set] RH1=%u, RH2=%u, TH1=%u, TH2=%u, TL1=%u, TL2=%u\n",
          GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1),
          GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2),
          GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1),
          GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2),
          GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1),
          GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2));
//      device_config.display_temp = 1;
//      device_config.display_dose = 1;
      // ----------- 반드시 디폴트값을 Backup Register에 Write -----------
      Save_All_Config_To_BackupRegister(&device_config);
      HAL_RTCEx_BKUPWrite(&hrtc, BKP_MODE_REG, MODE_Boot);
      idx_rad = 0;
      printf("First Boot\r\n");
      Load_DefaultSettings();
    }
  uint32_t pwrmode_bits = (COMP1->CSR & (COMP_CSR_PWRMODE_1 | COMP_CSR_PWRMODE_0));
  if (pwrmode_bits == (COMP_CSR_PWRMODE_1 | COMP_CSR_PWRMODE_0)) {
      // Ultra-low-power 모드가 활성화됨
	  printf("Low Power OK\r\n");

  } else {
      // 다른 모드 (High/Medium)인 경우
	  printf( "Low Power non\r\n");
  }

  SPI_FLASH_Init();

  DeviceSettings cfg;   // 내부 Flash Read
//  Load_DeviceSettings(&cfg);
  current_settings = cfg;

  Init_DeviceSettings();

  printf( "Regulatior On!\r\n");
  HAL_GPIO_WritePin(Regulator_En_GPIO_Port, Regulator_En_Pin, GPIO_PIN_SET); // 핀 High 상태로 출력

  HAL_ADCEx_Calibration_Start(&hadc1);
  uint32_t cal_factor = HAL_ADCEx_Calibration_GetValue(&hadc1);
  HAL_ADCEx_Calibration_SetValue(&hadc1, cal_factor);
  HAL_COMP_Start(&hcomp1);
  uint32_t prevTick = HAL_GetTick();
  uint32_t Tick_Save = 0;


//  Load_Temp_From_Backup(&device_config);
  Load_All_Config_From_BackupRegister(&device_config);
  meas_data_log_fast_init(Index_num);
  printf("[DEBUG] rad_interval=%u, temp_interval=%u, rad_ratio=%u\r\n",
            current_settings.rad_interval,
            current_settings.temp_interval,
            rad_ratio);


  static uint8_t usb_inited = 0;
  bool is_log_file_gen = 0;
  int i =0;


  USB_State = HAL_GPIO_ReadPin(USB_VBUS_GPIO_Port, USB_VBUS_Pin);
  printf("VBUS HAL_GPIO_ReadPin: %d\r\n", HAL_GPIO_ReadPin(USB_VBUS_GPIO_Port, USB_VBUS_Pin));
  printf("VBUS IDR: %d\r\n", (USB_VBUS_GPIO_Port->IDR & USB_VBUS_Pin) ? 1 : 0);



  if (USB_State){ // 전력 상관 없을시 수정
	  //FIXME: clean usb volume, one time formating??
//	  RAD_USBX_Clean_Vol();
	  RTC_Disable_All_Wakeup();
		  LCD_Clear_Display(LCD_data);
		  LCD_Display_USB_Load(LCD_data);// Loading USB화면 // Loading USB화면
//	  RAD_USBX_Fatfs_format_disk();
	  //FIXME: One time generated file, if file already existed, it will not created.
	  // It shoulb be done after the first time setting configuration from PC
		if(current_settings.mode_status == 0x03){ // Stop : 0x00  Start : 0x01  pause : 0x02  Over : 0x03
		RAD_Fatfs_MountOnly();
//			RAD_USBX_Clean_Vol();
//			RAD_USBX_Fatfs_format_disk();

      file_log_time_t start_time;
	    start_time.year = 00;
	    start_time.month = 0;
	    start_time.day = 0;
	    start_time.hour = 0;
	    start_time.minute = 0;
	    start_time.second = 0;
	    file_log_time_t stop_time;
	    stop_time.year = 0;
	    stop_time.month = 0;
	    stop_time.day = 0;
	    stop_time.hour = 0;
	    stop_time.minute = 0;
	    stop_time.second = 0;
      logging_summary_t tmp;
      tmp.mean_kinetic_temp = 0.0;
      tmp.start_time = start_time;
      tmp.stop_time = stop_time;
//      tmp.data_points_radiation_count = 0;
      tmp.data_points_temp_count = idx_temp;
	 //FIXME: One time generated file, if file already existed, it will not created.
	 // It shoulb be done after the first time setting configuration from PC
	 csv_gen_template_file(&current_settings);

   //FIXME: Next is generate PDF report, implementing...
     if(current_settings.report_format != 0x00){
    pdf_gen_template_file();
    pdf_append_all_flash_log_entries();
    pdf_gen_completed_report(&tmp,&current_settings);
     }
		}
//		HAL_Delay(1000);
    RAD_USBX_Device_Init();
    usb_inited = 1;
  }
//  Reset_All_Backup_Registers();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (USB_State){
		  if (!usb_inited){
			      RTC_Disable_All_Wakeup();
				  LCD_Clear_Display(LCD_data);
				  LCD_Display_USB_Load(LCD_data);// Loading USB화면// Loading USB화면
			if(current_settings.mode_status == 0x03){ // Stop : 0x00  Start : 0x01  pause : 0x02  Over : 0x03
			RAD_Fatfs_MountOnly();
//			RAD_USBX_Clean_Vol();
//	        RAD_USBX_Fatfs_format_disk();
	        file_log_time_t start_time;
	  	    start_time.year = 00;
	  	    start_time.month = 0;
	  	    start_time.day = 0;
	  	    start_time.hour = 0;
	  	    start_time.minute = 0;
	  	    start_time.second = 0;
	  	    file_log_time_t stop_time;
	  	    stop_time.year = 0;
	  	    stop_time.month = 0;
	  	    stop_time.day = 0;
	  	    stop_time.hour = 0;
	  	    stop_time.minute = 0;
	  	    stop_time.second = 0;
	        logging_summary_t tmp;
	        tmp.mean_kinetic_temp = 0.0;
	        tmp.start_time = start_time;
	        tmp.stop_time = stop_time;
	        tmp.data_points_radiation_count = 0;
	        tmp.data_points_temp_count  = idx_temp;
		 //FIXME: One time generated file, if file already existed, it will not created.
		 // It shoulb be done after the first time setting configuration from PC
		 csv_gen_template_file(&current_settings);

	     //FIXME: Next is generate PDF report, implementing...
	     if(current_settings.report_format != 0x00){
	    pdf_gen_template_file();
	    pdf_append_all_flash_log_entries();
	    pdf_gen_completed_report(&tmp,&current_settings);
	     }
			}
//			HAL_Delay(1000);
		  RAD_USBX_Device_Init();
		  usb_inited = 1;
		  }
      // Update summary log file, only
      // For example:
		  if(is_log_file_gen == 0) {

		  is_log_file_gen = 1;
		  }
		  RAD_USBX_Device_Process();
		  LCD_Clear_Display(LCD_data);
		  LCD_Display_USB(LCD_data);
		  USB_HID_Receive(USB_rev_buff, &len);

		  if (usbx_host_req_records == 1) {
		      static uint32_t usb_send_record_prevTick = 0;
//		      static UINT cur_record_num = 0;

		      if ((HAL_GetTick() - usb_send_record_prevTick) >= USB_HID_SEND_RECORDS_SPEED_MS) {
		          // ✅ 송신 시도 → 함수에서 usbx_host_req_records를 0으로 변경 가능
//		          printf("Send:hid: %u\r\n", cur_record_num);
		          if (USB_HID_Send_Record(cur_record_num) != 0) {
//		              printf("Send record %u failed\r\n", cur_record_num);
//		              usbx_host_req_records = 0; // 실패 시에도 종료 (필요시 제거 가능)
//		              cur_record_num = 0;
		          } else {
		              cur_record_num++;
		          }

		          usb_send_record_prevTick = HAL_GetTick();
		      }
		  }
	  }


	  if (HAL_GetTick() - prevTick >= 1000)
	  {
		  Press_Action();
		  Read_Temp();

		  if (USB_State){

			  USB_State = HAL_GPIO_ReadPin(USB_VBUS_GPIO_Port, USB_VBUS_Pin);

			  if (!USB_State) {
			     printf("USB non State!\r\n");
			     Save_CurrentSettings();
//			     usb_inited = 0;
			     LCD_Clear_Display(LCD_data);
			  if(current_settings.start_mode == 0x00 || current_settings.start_mode == 0x01){
			     if(current_settings.mode_status == 0x00){
			    	 Set_ModeStatus(mode_internal_backup_stop);
			    	 printf("Button Start_Stop\r\n");
 	  	 		     Switch_Backup_reg(MODE_Stop);
 	  	 		    Tick_Save = 0;
 	  	 		    First_Measure = 0;

			     }
			     else if (current_settings.mode_status == 0x01 || current_settings.mode_status == 0x02){
			    	 printf("Return to Pause Mode\r\n");
			       Set_ModeStatus(mode_internal_backup_pause);
	  	 		    Switch_Backup_reg(MODE_Pause);
	  	 		    Tick_Save = 0;
	  	 		    First_Measure = 0;

			     }
			     else if (current_settings.mode_status == 0x03){
			    	 printf("Return to End Mode\r\n");
			    	 Set_ModeStatus(mode_internal_backup_end);
		  	 		    Switch_Backup_reg(MODE_End);
		  	 		    Tick_Save = 0;
		  	 		    First_Measure = 0;
			     }
			   }
			  else { // SW Start
				  if(current_settings.mode_status == 0x00){ // SW Start
					  printf("SW Start !!!!\r\n");
	  	 		      if(current_settings.start_mode == 0x03){
	  	 		    	printf("SW Delay Mode\r\n");// SW Delay Start 전체
	  	 				if (current_settings.start_delay > 0){// SW Start Delay
	  	 				Switch_Backup_reg(Mode_Start_Delay);
   	  	 		        SW_count = 0;
   	  	 			    button_flag = false;
   	  	 		        Tick_Save = 0;
	  	 				}
	  	 				else{
	  	 					printf("SW Delay Mode : Delay is 0\r\n");// SW Delay Start 전체
	  	 			     Set_ModeStatus(mode_internal_backup_start);
	   	  	 			 Switch_Backup_reg(MODE_Start);
	   	  	 			 Index_num = Load_IndexNum_From_Backup();
	   	  	 			 Interval_LCD_Count = 1;
	   	  	 		     SW_count = 0;
	   	  	 			 button_flag = false;
	   	  	 	     	 Tick_Save = 0;
	  	 			    }
	  	 			  }
	  	 			  else if(current_settings.start_mode == 0x02){
	  	 				printf("SW Start Mode, Direct start\r\n");// SW Delay Start 전체
	  	 				Set_ModeStatus(mode_internal_backup_start);
	   	  	 			 Switch_Backup_reg(MODE_Start);
	   	  	 			 Index_num = Load_IndexNum_From_Backup();
	   	  	 			 Interval_LCD_Count = 1;
	   	  	 		     SW_count = 0;
	   	  	 			 button_flag = false;
	   	  	 	     	 Tick_Save = 0;
	  	 			  }
	  	 			  else if(current_settings.start_mode == 0x04){ // Target time start mode
	  	 				printf("Target time start\r\n");// SW Delay Start 전체
	   	  	 			 Switch_Backup_reg(Mode_Start_Delay);
	   	  	 			 Index_num = Load_IndexNum_From_Backup();
	   	  	 			 Interval_LCD_Count = 1;
	   	  	 		     SW_count = 0;
	   	  	 			 button_flag = false;
	   	  	 	     	 Tick_Save = 0;
	  	 			  }
	  	 		   Tick_Save = 0;
			      }
			  }

			  }

		  }
		  else{
			  if (First_Measure>1){

			  if (Meas_Mode == 0){ // ////////////////////////////////////////Boot
				  printf("Boot\r\n");
				  RTC_Disable_All_Wakeup();
				  meas_data_log_erase();  // USB Boot
				  RAD_USBX_Clean_Vol();
				  RAD_USBX_Fatfs_format_disk();
				  Set_ModeStatus(mode_internal_backup_stop);


				  Switch_Backup_reg(MODE_Stop);

			  }
			  else if (Meas_Mode == 1){ ///////////////////////////////////// Stop
				  Short_Measure();
				  switch (LCD_mode) {
				         case 1:

				        	 LCD_Display_Temp(Display_temperature, LCD_data, current_settings.display_temp_unit);
				        	 LCD_Display_Dose(dose_log, LCD_data_dose, current_settings.display_dose_unit);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 Small_Stop_Display(LCD_data);
				        	 //Mark, Alarm, Log-R,T
				        	 break;
				         case 2:
				        	 LCD_Display_date(LCD_data);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 Small_Stop_Display(LCD_data);
				        	 //Mark
				        	 break;
				         case 3:
				        	 LCD_Display_Time(LCD_data);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 Small_Stop_Display(LCD_data);
				        	 break;

				         default:
				        	 LCD_Display_Temp(Display_temperature, LCD_data, current_settings.display_temp_unit);
				        	 LCD_Display_Dose(dose_log, LCD_data_dose, current_settings.display_dose_unit);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 Small_Stop_Display(LCD_data);
				        	 LCD_mode = 1;
				        	 break;
				  }
				  printf("Stop\r\n");
			  }
			  else if (Meas_Mode == 2){ ///////////////////////////////////Start (Interval)

				  Interval_Measure();


				  if (Interval_LCD_Count>0){
					  if(Interval_LCD_Count<=7 ){
				  switch (LCD_mode) {
				         case 1:
				        	 LCD_Display_Temp(Display_temperature, LCD_data, current_settings.display_temp_unit);
				        	 LCD_Display_Dose(dose_log, LCD_data_dose, current_settings.display_dose_unit);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 Small_Start_Display(LCD_data);
				        	 LCD_Display_Alarm(LCD_data);
				        	 Mark_Display(LCD_data);
				        	 //Mark, Alarm, Log-R,T
				        	 break;

				         case 2:

				        	 LCD_Display_LP(idx_temp, LCD_data, 0);
				        	 Small_Start_Display(LCD_data);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 LCD_Display_Alarm(LCD_data);
				        	 Mark_Display(LCD_data);
				        	 break;

				         case 3:

				        	 LCD_Display_LP(idx_rad, LCD_data, 1);
				        	 Small_Start_Display(LCD_data);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 LCD_Display_Alarm(LCD_data);
				        	 Mark_Display(LCD_data);
				        	 break;

				         case 4:
				        	 LCD_Display_Temp_MinMax((float)device_config.temp_max , LCD_data, 0x00);
				        	 LCD_Display_Dose(device_config.dose_max/10, LCD_data_dose, 0x00);
				        	 Max_Display(LCD_data);
				        	 Small_Start_Display(LCD_data);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 LCD_Display_Alarm(LCD_data);
				        	 Mark_Display(LCD_data);
				        	 break;

				         case 5:
				        	 LCD_Display_Temp_MinMax((float)device_config.temp_min , LCD_data, 0x00);
				        	 Blank_Dose_Display(LCD_data);
				        	 Min_Display(LCD_data);
				        	 Small_Start_Display(LCD_data);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 LCD_Display_Alarm(LCD_data);
				        	 Mark_Display(LCD_data);
				        	 break;

				         case 6:
				        	 LCD_Display_date(LCD_data);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 Small_Start_Display(LCD_data);
				        	 LCD_Display_Alarm(LCD_data);
				        	 Mark_Display(LCD_data);
				        	 //Mark
				        	 break;
				         case 7:
				        	 LCD_Display_Time(LCD_data);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 Small_Start_Display(LCD_data);
				        	 LCD_Display_Alarm(LCD_data);
				        	 Mark_Display(LCD_data);
				        	 break;

				         default:
				        	 LCD_Display_Temp(Display_temperature, LCD_data, current_settings.display_temp_unit);
				        	 LCD_Display_Dose(dose_log, LCD_data_dose, current_settings.display_dose_unit);
				        	 LCD_Display_Battery(0, LCD_data);
				        	 Small_Start_Display(LCD_data);
				        	 LCD_Display_Alarm(LCD_data);
				        	 Mark_Display(LCD_data);
				        	 LCD_mode = 1;
				        	 break;
				  }
				  Interval_LCD_Count++;
					  }
					  else{
						  Interval_LCD_Count = 0;
					  }
				  }

				  printf("Start (Interval)\r\n");
			  }
			  else if (Meas_Mode == 3){ ///////////////////////////////////// Pause
				   Short_Measure();
				   LCD_Display_PauseMode(LCD_data);



				  printf("Pause\r\n");
			  }
			  else if(Meas_Mode == 4) { // Start(Button)
				  Short_Measure();
				  switch (LCD_mode) {
			         case 1:
			        	 LCD_Display_Temp(Display_temperature, LCD_data, current_settings.display_temp_unit);
			        	 LCD_Display_Dose(dose_log, LCD_data_dose, current_settings.display_dose_unit);
			        	 LCD_Display_Battery(0, LCD_data);
			        	 Small_Start_Display(LCD_data);
			        	 LCD_Display_Alarm(LCD_data);
			        	 Mark_Display(LCD_data);
			        	 //Mark, Alarm, Log-R,T
			        	 break;

			         case 2:

			        	 LCD_Display_LP(idx_temp, LCD_data, 0);
			        	 Small_Start_Display(LCD_data);
			        	 LCD_Display_Battery(0, LCD_data);
			        	 LCD_Display_Alarm(LCD_data);
			        	 Mark_Display(LCD_data);
			        	 break;

			         case 3:

			        	 LCD_Display_LP(idx_rad, LCD_data, 1);
			        	 Small_Start_Display(LCD_data);
			        	 LCD_Display_Battery(0, LCD_data);
			        	 LCD_Display_Alarm(LCD_data);
			        	 Mark_Display(LCD_data);
			        	 break;

			         case 4:
			        	 LCD_Display_Temp_MinMax((float)device_config.temp_max , LCD_data, 0x00);
			        	 LCD_Display_Dose(device_config.dose_max/10, LCD_data_dose, 0x00);
			        	 Max_Display(LCD_data);
			        	 Small_Start_Display(LCD_data);
			        	 LCD_Display_Battery(0, LCD_data);
			        	 LCD_Display_Alarm(LCD_data);
			        	 Mark_Display(LCD_data);
			        	 break;

			         case 5:
			        	 LCD_Display_Temp_MinMax((float)device_config.temp_min , LCD_data, 0x00);
			        	 Blank_Dose_Display(LCD_data);
			        	 Min_Display(LCD_data);
			        	 Small_Start_Display(LCD_data);
			        	 LCD_Display_Battery(0, LCD_data);
			        	 LCD_Display_Alarm(LCD_data);
			        	 Mark_Display(LCD_data);
			        	 break;

			         case 6:
			        	 LCD_Display_date(LCD_data);
			        	 LCD_Display_Battery(0, LCD_data);
			        	 Small_Start_Display(LCD_data);
			        	 LCD_Display_Alarm(LCD_data);
			        	 Mark_Display(LCD_data);
			        	 //Mark
			        	 break;
			         case 7:
			        	 LCD_Display_Time(LCD_data);
			        	 LCD_Display_Battery(0, LCD_data);
			        	 Small_Start_Display(LCD_data);
			        	 LCD_Display_Alarm(LCD_data);
			        	 Mark_Display(LCD_data);
			        	 break;

			         default:
			        	 LCD_Display_Temp(Display_temperature, LCD_data, current_settings.display_temp_unit);
			        	 LCD_Display_Dose(dose_log, LCD_data_dose, current_settings.display_dose_unit);
			        	 LCD_Display_Battery(0, LCD_data);
			        	 Small_Start_Display(LCD_data);
			        	 LCD_Display_Alarm(LCD_data);
			        	 Mark_Display(LCD_data);
			        	 LCD_mode = 1;
			        	 break;
				  }
				  printf("Start(Button)\r\n");
			  }
			  else if(Meas_Mode == 5) { // Start(Delay)
//			  		Short_Measure();
				    Non_Measure();
			  		Get_And_Print_Seconds_Left_ToStart();
			  		LCD_Display_DelayMode(start_remaining, LCD_data);
			  	   }
			  else if(Meas_Mode == 6) { // End
				  Non_Measure();
  				  switch (LCD_mode) {
			             case 1:

			        	     LCD_Display_LP(idx_temp, LCD_data, 0);
			        	     Small_Stop_Display(LCD_data);
			        	     LCD_Display_Battery(0, LCD_data);
			           	     Mark_Display(LCD_data);
			        	     break;

			             case 2:

			        	     LCD_Display_LP(idx_rad, LCD_data, 1);
			        	     Small_Stop_Display(LCD_data);
			        	     LCD_Display_Battery(0, LCD_data);
			        	     Mark_Display(LCD_data);
			        	     break;

  				         case 3:
  				        	 LCD_Display_Temp_MinMax((float)device_config.temp_max , LCD_data, 0x00);
  				        	 LCD_Display_Dose(device_config.dose_max, LCD_data_dose, 0x00);
  				        	 Max_Display(LCD_data);
  				        	 Small_Stop_Display(LCD_data);
  				        	 LCD_Display_Battery(0, LCD_data);
  				        	 Mark_Display(LCD_data);
  				        	 break;

  				         case 4:
  				        	 LCD_Display_Temp_MinMax((float)device_config.temp_min , LCD_data, 0x00);
  				        	 Blank_Dose_Display(LCD_data);
  				        	 Min_Display(LCD_data);
  				        	 Small_Stop_Display(LCD_data);
  				        	 LCD_Display_Battery(0, LCD_data);
  				        	 Mark_Display(LCD_data);
  				        	 break;

  				         case 5:
  				        	 LCD_Display_date(LCD_data);
  				        	 LCD_Display_Battery(0, LCD_data);
  				        	 Small_Stop_Display(LCD_data);
  				        	 Mark_Display(LCD_data);
  				        	 //Mark
  				        	 break;
  				         case 6:
  				        	 LCD_Display_Time(LCD_data);
  				        	 LCD_Display_Battery(0, LCD_data);
  				        	 Small_Stop_Display(LCD_data);
  				        	 Mark_Display(LCD_data);
  				        	 break;

  				         default:

			        	     LCD_Display_LP(idx_temp, LCD_data, 0);
			        	     Small_Stop_Display(LCD_data);
			        	     LCD_Display_Battery(0, LCD_data);
			           	     Mark_Display(LCD_data);
  				        	 LCD_mode = 1;
  				        	 break;
  				  }
				  printf("End\r\n");
			  }
		  }

			  else{
				  if (First_Measure == 0){
					  if(Meas_Mode == 2 || Meas_Mode == 4){
						  if (Has_Logging_Duration_Ended()) {
			 	  	 			RTC_Disable_All_Wakeup();
			 	  	 		    Switch_Backup_reg(MODE_End);
			                    Set_ModeStatus(mode_internal_backup_end);
			 	  	 		    Tick_Save = 0;
			 	  	 		    SW_count = 0;
			 	  	 	     	button_flag = false;
			 	  	 	        First_Measure = 0;
						  }
					  }
					  LCD_Clear_Display(LCD_data);
				  }
			  	  First_Measure++;
			  	  printf("First Count : '%u'\r\n", First_Measure);

			  if (Meas_Mode == 0){ // Boot
				  printf("Boot\r\n");
				  LCD_Display_Boot(LCD_data);
			  }
			  else if (Meas_Mode == 1){ // Stop
				  printf("Stop\r\n");
				  LCD_mode = 1;
				  LCD_Display_StopMode(LCD_data);
				  RTC_Disable_All_Wakeup();
			  }
			  else if (Meas_Mode == 2 ){
				  if(!RTC_During_Wakeup)
				 {
					  if(Interval_LCD_Count >=1){
				      // Start (Interval)
				      if (First_Measure==1){
				      LCD_Display_InTime_LT(current_settings.temp_interval, LCD_data);
				      }
				      else {
				      LCD_Display_InTime_LR(current_settings.rad_interval, LCD_data);
				      LCD_mode = 1;
				      }
				   }
				 }
				 else{
					  if (Interval_LCD_Count>0){
					  switch (LCD_mode) {
					         case 1:
					        	 LCD_Display_Temp(Display_temperature, LCD_data, current_settings.display_temp_unit);
					        	 LCD_Display_Dose(dose_log, LCD_data_dose, current_settings.display_dose_unit);
					        	 LCD_Display_Battery(0, LCD_data);
					        	 Small_Start_Display(LCD_data);
					        	 Mark_Display(LCD_data);
					        	 //Mark, Alarm, Log-R,T
					        	 break;

					         case 2:

					        	 LCD_Display_LP(idx_temp, LCD_data, 0);
					        	 Small_Start_Display(LCD_data);
					        	 LCD_Display_Battery(0, LCD_data);
					        	 Mark_Display(LCD_data);
					        	 break;

					         case 3:

					        	 LCD_Display_LP(idx_rad, LCD_data, 1);
					        	 Small_Start_Display(LCD_data);
					        	 LCD_Display_Battery(0, LCD_data);
					        	 Mark_Display(LCD_data);
					        	 break;

					         case 4:
					        	 LCD_Display_Temp((float)device_config.temp_max , LCD_data, current_settings.display_temp_unit);
					        	 LCD_Display_Dose(device_config.dose_max/10, LCD_data_dose, 0x00);
					        	 Max_Display(LCD_data);
					        	 Small_Start_Display(LCD_data);
					        	 LCD_Display_Battery(0, LCD_data);
					        	 Mark_Display(LCD_data);
					        	 break;

					         case 5:
					        	 LCD_Display_Temp((float)device_config.temp_min , LCD_data, current_settings.display_temp_unit);
					        	 Blank_Dose_Display(LCD_data);
					        	 Min_Display(LCD_data);
					        	 Small_Start_Display(LCD_data);
					        	 LCD_Display_Battery(0, LCD_data);
					        	 Mark_Display(LCD_data);
					        	 break;

					         case 6:
					        	 LCD_Display_date(LCD_data);
					        	 LCD_Display_Battery(0, LCD_data);
					        	 Small_Start_Display(LCD_data);
					        	 Mark_Display(LCD_data);
					        	 //Mark
					        	 break;
					         case 7:
					        	 LCD_Display_Time(LCD_data);
					        	 LCD_Display_Battery(0, LCD_data);
					        	 Small_Start_Display(LCD_data);
					        	 Mark_Display(LCD_data);
					        	 break;

					         default:
					        	 LCD_Display_Temp(Display_temperature, LCD_data, current_settings.display_temp_unit);
					        	 LCD_Display_Dose(dose_log, LCD_data_dose, current_settings.display_dose_unit);
					        	 LCD_Display_Battery(0, LCD_data);
					        	 Small_Start_Display(LCD_data);
					        	 Mark_Display(LCD_data);
					        	 LCD_mode = 1;
					        	 break;
					  }
					  }
				 }
			  }
			  else if (Meas_Mode == 3){ // Pause
				  printf("Pause\r\n");
				  LCD_mode = 1;
				  LCD_Display_PauseMode(LCD_data);
			  }
			  else if(Meas_Mode == 4) { // Start(Button)
				  // Start (Interval)
				  if (First_Measure==1){

				  LCD_Display_InTime_LT(current_settings.temp_interval, LCD_data);

				  }
				  else {
				  LCD_Display_InTime_LR(current_settings.rad_interval, LCD_data);
				  LCD_mode = 1;
				  }

			  }
			  else if(Meas_Mode == 5) { // Start(Delay)
				  if (First_Measure==1){
					  Maybe_Rearm_Reservation_OnWake();
				  }
			  	  Get_And_Print_Seconds_Left_ToStart();
			  	  LCD_Display_DelayMode(start_remaining, LCD_data);  // Delay Display 로 교체
			  }
			  else if(Meas_Mode == 6) { // End
				  LCD_Display_EndMode(LCD_data);
				  RTC_Disable_All_Wakeup();
			  }
			  comp1_count = 0;
			  }
			  HAL_LCD_UpdateDisplayRequest(& hlcd);


		  }
		  if (!usb_inited)
		  USB_State = HAL_GPIO_ReadPin(USB_VBUS_GPIO_Port, USB_VBUS_Pin);
	  	  prevTick = HAL_GetTick();
//
	  }
    
            /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
//void SystemClock_Config(void)
//{
//  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
//  RCC_CRSInitTypeDef RCC_CRSInitStruct = {0};
//
//  /** Configure the main internal regulator output voltage
//  */
//  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
//
//  /** Configure LSE Drive Capability
//  */
//  HAL_PWR_EnableBkUpAccess();
//  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
//
//  /** Initializes the RCC Oscillators according to the specified parameters
//  * in the RCC_OscInitTypeDef structure.
//  */
//  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE
//                              |RCC_OSCILLATORTYPE_HSI48;
//  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
//  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
//  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
//  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
//  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
//  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
//  RCC_OscInitStruct.PLL.PLLN = 6;
//  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
//  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
//  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
//  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//  {
//    Error_Handler();
//  }
//
//  /** Initializes the CPU, AHB and APB buses clocks
//  */
//  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//                              |RCC_CLOCKTYPE_PCLK1;
//  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
//
//  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//
//  /** Enable the CRS clock
//  */
//  __HAL_RCC_CRS_CLK_ENABLE();
//
//  /** Configures CRS
//  */
//  RCC_CRSInitStruct.Prescaler = RCC_CRS_SYNC_DIV1;
//  RCC_CRSInitStruct.Source = RCC_CRS_SYNC_SOURCE_LSE;
//  RCC_CRSInitStruct.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
//  RCC_CRSInitStruct.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000,32768);
//  RCC_CRSInitStruct.ErrorLimitValue = 34;
//  RCC_CRSInitStruct.HSI48CalibrationValue = 32;
//
//  HAL_RCCEx_CRSConfig(&RCC_CRSInitStruct);
//}
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_CRSInitTypeDef RCC_CRSInitStruct = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  // ✅ HSI와 LSE만 사용, PLL은 비활성화
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;  // PLL OFF
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  // ✅ SYSCLK = HSI (16MHz)
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;  // PLL이 아닌 HSI 직접 사용
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  // ✅ Flash 대기시간 0 (16MHz)
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }

  // ✅ CRS (LSE 동기화) 설정은 유지
  __HAL_RCC_CRS_CLK_ENABLE();

  RCC_CRSInitStruct.Prescaler = RCC_CRS_SYNC_DIV1;
  RCC_CRSInitStruct.Source = RCC_CRS_SYNC_SOURCE_LSE;
  RCC_CRSInitStruct.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
  RCC_CRSInitStruct.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000, 32768); // 48MHz 기준
  RCC_CRSInitStruct.ErrorLimitValue = 34;
  RCC_CRSInitStruct.HSI48CalibrationValue = 32;

  HAL_RCCEx_CRSConfig(&RCC_CRSInitStruct);
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_160CYCLES_5;
  hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_160CYCLES_5;
  hadc1.Init.OversamplingMode = DISABLE;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */
//  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_160CYCLES_5;
//  hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_160CYCLES_5;
  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief COMP1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_COMP1_Init(void)
{

  /* USER CODE BEGIN COMP1_Init 0 */

  /* USER CODE END COMP1_Init 0 */

  /* USER CODE BEGIN COMP1_Init 1 */

  /* USER CODE END COMP1_Init 1 */
  hcomp1.Instance = COMP1;
  hcomp1.Init.InputPlus = COMP_INPUT_PLUS_IO2;
  hcomp1.Init.InputMinus = COMP_INPUT_MINUS_IO1;
  hcomp1.Init.OutputPol = COMP_OUTPUTPOL_NONINVERTED;
  hcomp1.Init.WindowOutput = COMP_WINDOWOUTPUT_EACH_COMP;
  hcomp1.Init.Hysteresis = COMP_HYSTERESIS_HIGH;
  hcomp1.Init.BlankingSrce = COMP_BLANKINGSRC_NONE;
  hcomp1.Init.Mode = COMP_POWERMODE_ULTRALOWPOWER;
  hcomp1.Init.WindowMode = COMP_WINDOWMODE_DISABLE;
  hcomp1.Init.TriggerMode = COMP_TRIGGERMODE_IT_FALLING;
  if (HAL_COMP_Init(&hcomp1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN COMP1_Init 2 */
//	  hcomp1.Init.InputPlus    = COMP_INPUT_PLUS_IO2;    // 예: PB2가 IN+라면 IO1
//	  hcomp1.Init.InputMinus   = COMP_INPUT_MINUS_IO1;   // PC4을 IN–으로 사용
//	  hcomp1.Init.TriggerMode = COMP_TRIGGERMODE_IT_FALLING;
//	  hcomp1.Init.Mode = COMP_POWERMODE_ULTRALOWPOWER;



  /* USER CODE END COMP1_Init 2 */

}

/**
  * @brief LCD Initialization Function
  * @param None
  * @retval None
  */
static void MX_LCD_Init(void)
{

  /* USER CODE BEGIN LCD_Init 0 */

  /* USER CODE END LCD_Init 0 */

  /* USER CODE BEGIN LCD_Init 1 */

  /* USER CODE END LCD_Init 1 */
  hlcd.Instance = LCD;
  hlcd.Init.Prescaler = LCD_PRESCALER_1;
  hlcd.Init.Divider = LCD_DIVIDER_16;
  hlcd.Init.Duty = LCD_DUTY_1_4;
  hlcd.Init.Bias = LCD_BIAS_1_4;
  hlcd.Init.VoltageSource = LCD_VOLTAGESOURCE_INTERNAL;
  hlcd.Init.Contrast = LCD_CONTRASTLEVEL_0;
  hlcd.Init.DeadTime = LCD_DEADTIME_0;
  hlcd.Init.PulseOnDuration = LCD_PULSEONDURATION_1;
  hlcd.Init.BlinkMode = LCD_BLINKMODE_OFF;
  hlcd.Init.BlinkFrequency = LCD_BLINKFREQUENCY_DIV8;
  hlcd.Init.MuxSegment = LCD_MUXSEGMENT_DISABLE;
  if (HAL_LCD_Init(&hlcd) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the High Driver
  */
  __HAL_LCD_HIGHDRIVER_ENABLE(&hlcd);
  /* USER CODE BEGIN LCD_Init 2 */
//  void LCD_Clear_Display();
  /* USER CODE END LCD_Init 2 */

}

/**
  * @brief LPTIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPTIM1_Init(void)
{

  /* USER CODE BEGIN LPTIM1_Init 0 */

  /* USER CODE END LPTIM1_Init 0 */

  /* USER CODE BEGIN LPTIM1_Init 1 */

  /* USER CODE END LPTIM1_Init 1 */
  hlptim1.Instance = LPTIM1;
  hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
  hlptim1.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV1;
  hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
  hlptim1.Init.Period = 32767;
  hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
  hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
  hlptim1.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
  hlptim1.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
  hlptim1.Init.RepetitionCounter = 0;
  if (HAL_LPTIM_Init(&hlptim1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPTIM1_Init 2 */

  /* USER CODE END LPTIM1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
  hrtc.Init.BinMode = RTC_BINARY_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_TUESDAY;
  sDate.Month = RTC_MONTH_MAY;
  sDate.Date = 0x27;
  sDate.Year = 0x25;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
  */

  /* USER CODE BEGIN RTC_Init 2 */

  //  HAL_RTCEx_SetSmoothCalib(
  //    &hrtc,
  //    RTC_SMOOTHCALIB_PERIOD_32SEC,      // 32 초마다 보정 주기
  //    RTC_SMOOTHCALIB_PLUSPULSES_RESET,  // 제거 모드 설정
  //    60                                 // 한 주기당 제거할 펄스 수
  //  );

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_HalfDuplex_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_DRD_FS.Instance = USB_DRD_FS;
  hpcd_USB_DRD_FS.Init.dev_endpoints = 8;
  hpcd_USB_DRD_FS.Init.speed = USBD_FS_SPEED;
  hpcd_USB_DRD_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_DRD_FS.Init.Sof_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.battery_charging_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_DRD_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Regulator_En_Pin|Middle_Temp_GND_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Low_Temp_GND_GPIO_Port, Low_Temp_GND_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USB_VBUS_Pin */
  GPIO_InitStruct.Pin = USB_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(USB_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : User_Button_Pin */
  GPIO_InitStruct.Pin = User_Button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(User_Button_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Flash_CS_Pin */
  GPIO_InitStruct.Pin = Flash_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Flash_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Regulator_En_Pin */
  GPIO_InitStruct.Pin = Regulator_En_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Regulator_En_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Low_Temp_GND_Pin */
  GPIO_InitStruct.Pin = Low_Temp_GND_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Low_Temp_GND_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Middle_Temp_GND_Pin */
  GPIO_InitStruct.Pin = Middle_Temp_GND_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Middle_Temp_GND_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
//  HAL_NVIC_EnableIRQ(ADC_COMP1_2_IRQn);
//  HAL_NVIC_SetPriority(ADC_COMP1_2_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
    Backup_GetMode();
    if (Meas_Mode == 4 && HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
    	Meas_Mode = 2;
    }

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp)
  {
      if (hcomp->Instance == COMP1)
      {
          comp1_count++;
      }
  }
  void COMP1_2_3_IRQHandler(void)
  {
      HAL_COMP_IRQHandler(&hcomp1);
  }
void EXTI0_1_IRQHandler(void)
{
    // User_Button_Pin 이 PA0 이면 이렇게 호출
    HAL_GPIO_EXTI_IRQHandler(User_Button_Pin);
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_0)
    {
        // ⭐️ 실제 버튼이 눌려있는 상태(=Low)인지 한번 더 확인 (안정성 증가)
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) // 0=눌림
        {
            uint32_t now = HAL_GetTick();
            if (now - last_button_tick < DOUBLE_CLICK_MS) {
                button_press_count++;
            } else {
                button_press_count = 1;
            }
            last_button_tick = now;
            button_flag = true;

            if(Meas_Mode == 2){
                Interval_LCD_Count = 1;
            }
        }
    }
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{

    // 깨어난 이후 처리할 동작
	printf("Wake Up by RTC Timer!\r\n");
	    if(Meas_Mode != 5){
	        Meas_Mode = 2;
	        Tick_Save = Interval_LCD_Count;
	        Tick_Save = 0;
	        First_Measure = -1;
	        RTC_During_Wakeup = 1;
	    }
	    if (Meas_Mode == 2 || Meas_Mode == 4){
	    Load_Backup_Index();}
    //FIXME: is it correct to pass ** pointer?
	    RTC_Disable_All_Wakeup();
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc) {
    // 깨어난 이후 처리할 동작
	printf("Wake Up by RTC Alarm!\r\n");
	    if(Meas_Mode != 5){
	        Meas_Mode = 2;
	        Tick_Save = Interval_LCD_Count;
	        Tick_Save = 0;
	        First_Measure = -1;
	        RTC_During_Wakeup = 1;
	    }
	    if (Meas_Mode == 2 || Meas_Mode == 4){
	    Load_Backup_Index();}
    //FIXME: is it correct to pass ** pointer?
	    RTC_Disable_All_Wakeup();
}

static void PVD_Config(uint32_t level)
{
  PWR_PVDTypeDef sConfigPVD;

  sConfigPVD.PVDLevel = level;
  sConfigPVD.Mode = PWR_PVD_MODE_IT_FALLING;
  HAL_PWR_ConfigPVD(&sConfigPVD);
  HAL_PWR_EnablePVD();
}


void HAL_PWR_PVDCallback(void)
{
	Get_in_Shutdown();
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
