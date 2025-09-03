/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u0xx_hal.h"
#include "ux_api.h"   // ULONG, UINT 등 정의

/* Private includes --------------------------------------------Check_And_Run_Alarms
 * --------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern RTC_HandleTypeDef hrtc;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void MX_USB_PCD_Init(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USB_VBUS_Pin GPIO_PIN_13
#define USB_VBUS_GPIO_Port GPIOC
#define User_Button_Pin GPIO_PIN_0
#define User_Button_GPIO_Port GPIOA
#define Flash_CS_Pin GPIO_PIN_4
#define Flash_CS_GPIO_Port GPIOA
#define Regulator_En_Pin GPIO_PIN_10
#define Regulator_En_GPIO_Port GPIOB
#define Low_Temp_GND_Pin GPIO_PIN_2
#define Low_Temp_GND_GPIO_Port GPIOD
#define Middle_Temp_GND_Pin GPIO_PIN_6
#define Middle_Temp_GND_GPIO_Port GPIOB
////////////////////////////////RTC Backup Register
#define RTC_MAGIC_VALUE  0x32F2
#define BKP_DOSE_MAX      RTC_BKP_DR0   // Does Max Backup
#define BKP_MODE_REG       RTC_BKP_DR1   // 모드 플래그, Index Number
#define BKP_TEMP_MIN_MAX      RTC_BKP_DR2   // Temp Max/Min Back Up
#define BKP_IDX_REG        RTC_BKP_DR3   // Temp, Rad Index,
#define BKP_COUNT_REG      RTC_BKP_DR4   //rad_interval_count (메모리 줄이기 가능)
#define BKP_PRE_RAD_VALUE      RTC_BKP_DR5
#define BKP_MARK           RTC_BKP_DR6   // Mark, Alarm State (1byte만 사용)
#define BKP_WAKEUP_TIME      RTC_BKP_DR7
#define BKP_INTERVAL_INFO  RTC_BKP_DR8   // Interval Time (uint16) + Display Info (uint8 2개)

/* USER CODE BEGIN Private defines */
typedef enum {
    ALARM_OFF     = 0,
    ALARM_ON      = 1,
    ALARM_DISABLE = 2
} AlarmState;

//FIXME: Alarm State Bit Position (16 bit variable but shift 18 pos?)
#define ALARM_STATE_POS_RH1   8
#define ALARM_STATE_POS_RH2  10
#define ALARM_STATE_POS_TH1  12
#define ALARM_STATE_POS_TH2  14
#define ALARM_STATE_POS_TL1  16
#define ALARM_STATE_POS_TL2  18
#define ALARM_STATE_MASK      0x3

#define mode_internal_backup_stop  0x00
#define mode_internal_backup_start 0x01
#define mode_internal_backup_pause 0x02
#define mode_internal_backup_end   0x03

#define SET_ALARM_STATE(v, pos, st)   (v = (v & ~(ALARM_STATE_MASK << pos)) | ((st & ALARM_STATE_MASK) << pos))
#define GET_ALARM_STATE(v, pos)       (((v) >> (pos)) & ALARM_STATE_MASK)

typedef struct {
    int16_t temp_max, temp_min;
    uint32_t dose_max;
    uint16_t alarm_rh1;
    uint16_t alarm_rh2;
    uint16_t alarm_th1;
    uint16_t alarm_th2;
    uint16_t alarm_tl1;
    uint16_t alarm_tl2;
    uint8_t mark;
    uint16_t delay;
    uint32_t alarm_state;    // 6개 Alarm 상태 (각 2비트, 총 12비트)
    uint16_t interval_time;
    uint8_t display_temp, display_dose;
} DeviceConfig;



typedef enum {
    MODE_Boot   = 1,
    MODE_Stop   = 2,
    MODE_Start  = 3,
    MODE_Pause  = 4,
    MODE_Start_Button = 5,
	Mode_Start_Delay = 6,
	MODE_End = 7
} SystemMode;

void Switch_Backup_reg(SystemMode new_mode);
void Save_IndexNum_To_Backup(uint16_t idx);


void Save_All_Config_To_BackupRegister(DeviceConfig *cfg);
void Save_Temp_To_Backup(DeviceConfig *cfg);
void Save_Dose_To_Backup(DeviceConfig *cfg);
void Save_AlarmRH_To_Backup(DeviceConfig *cfg);
void Save_AlarmTH_To_Backup(DeviceConfig *cfg);
void Save_AlarmTL_To_Backup(DeviceConfig *cfg);
void Save_MarkAndAlarmState_To_Backup(DeviceConfig *cfg);
void Save_Delay_To_Backup(DeviceConfig *cfg);
void Save_IntervalInfo_To_Backup(DeviceConfig *cfg);
uint32_t Load_Pre_RAD_Value_From_Backup(void);
void Save_Pre_RAD_Value_To_Backup(uint32_t value);


uint16_t Load_IndexNum_From_Backup(void);
void Load_Temp_From_Backup(DeviceConfig *cfg);
void Load_Dose_From_Backup(DeviceConfig *cfg);
void Load_AlarmRH_From_Backup(DeviceConfig *cfg);
void Load_AlarmTH_From_Backup(DeviceConfig *cfg);
void Load_AlarmTL_From_Backup(DeviceConfig *cfg);
void Load_MarkAndAlarmState_From_Backup(DeviceConfig *cfg);
void Load_Delay_From_Backup(DeviceConfig *cfg);
void Load_IntervalInfo_From_Backup(DeviceConfig *cfg);
void Load_All_Config_From_BackupRegister(DeviceConfig *cfg);
uint32_t Load_WakeupTime_From_Backup(void);

void Set_RTC_TimeOnly(uint8_t yy, uint8_t mm, uint8_t dd,
                      uint8_t hh, uint8_t mi, uint8_t ss);
void Set_Mark_And_Save(DeviceConfig* cfg, uint8_t mark_value);
// main.h (실제 정의가 main.c에 있는 것들)
void Set_RTC_TimeOnly(uint8_t yy, uint8_t mm, uint8_t dd, uint8_t hh, uint8_t mi, uint8_t ss);
void Set_Mark_And_Save(DeviceConfig* cfg, uint8_t mark_value);
void Set_Wakeup_After_Delay(uint32_t delay_sec, const RTC_TimeTypeDef *sTime_bcd, const RTC_DateTypeDef *sDate_bcd);

void Clear_WakeupTime_Backup(void);
void Clear_Backup_Index(void);
void Reset_All_Backup_Registers(void);
void Check_And_Run_Alarms(float temp_avg, uint32_t dose);

// rad_usb.h (rad_usb.c에서 제공하는 것들)
void USB_HID_Receive(uint8_t* data, ULONG* len);
void send_all_log_entries_usb(void);

// usb_msc_pdf_file_log.h (정의가 해당 C 파일에 있다면)
void pdf_append_all_flash_log_entries(void);

// (pdf 유틸이 다른 파일에 있다면 거기에 맞춰 옮기기)


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
