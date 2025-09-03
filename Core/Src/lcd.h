#ifndef SRC_LCD_H_
#define SRC_LCD_H_

#include "stm32u0xx_hal.h"
#include "stm32u0xx_hal_lcd.h"
#include <stdint.h>
#include <stdbool.h>

// LCD 핸들러 선언
extern LCD_HandleTypeDef hlcd;
#define BCD2BIN(x)  ((((x) >> 4) * 10) + ((x) & 0x0F))    // ← 추가!

// ------- COM/SEG 매크로 --------
#define MCU_LCD_COM0          LCD_RAM_REGISTER0
#define MCU_LCD_COM0_1        LCD_RAM_REGISTER1
#define MCU_LCD_COM1          LCD_RAM_REGISTER2
#define MCU_LCD_COM1_1        LCD_RAM_REGISTER3
#define MCU_LCD_COM2          LCD_RAM_REGISTER4
#define MCU_LCD_COM2_1        LCD_RAM_REGISTER5
#define MCU_LCD_COM3          LCD_RAM_REGISTER6
#define MCU_LCD_COM3_1        LCD_RAM_REGISTER7

#define MCU_LCD_SEG0_SHIFT    0
#define MCU_LCD_SEG1_SHIFT    1
#define MCU_LCD_SEG2_SHIFT    2
#define MCU_LCD_SEG3_SHIFT    3
#define MCU_LCD_SEG4_SHIFT    4
#define MCU_LCD_SEG5_SHIFT    5
#define MCU_LCD_SEG6_SHIFT    6
#define MCU_LCD_SEG7_SHIFT    7
#define MCU_LCD_SEG8_SHIFT    8
#define MCU_LCD_SEG9_SHIFT    9
#define MCU_LCD_SEG10_SHIFT   10
#define MCU_LCD_SEG11_SHIFT   11
#define MCU_LCD_SEG12_SHIFT   12
#define MCU_LCD_SEG13_SHIFT   13
#define MCU_LCD_SEG14_SHIFT   14
#define MCU_LCD_SEG15_SHIFT   15
#define MCU_LCD_SEG16_SHIFT   16
#define MCU_LCD_SEG17_SHIFT   17
#define MCU_LCD_SEG18_SHIFT   18
#define MCU_LCD_SEG19_SHIFT   19
#define MCU_LCD_SEG20_SHIFT   20
#define MCU_LCD_SEG21_SHIFT   21
#define MCU_LCD_SEG22_SHIFT   22
#define MCU_LCD_SEG23_SHIFT   23
#define MCU_LCD_SEG24_SHIFT   24
#define MCU_LCD_SEG25_SHIFT   25
#define MCU_LCD_SEG26_SHIFT   26
#define MCU_LCD_SEG27_SHIFT   27
#define MCU_LCD_SEG28_SHIFT   28
#define MCU_LCD_SEG29_SHIFT   29
#define MCU_LCD_SEG30_SHIFT   30
#define MCU_LCD_SEG31_SHIFT   31
#define MCU_LCD_SEG32_SHIFT   0
#define MCU_LCD_SEG33_SHIFT   1
#define MCU_LCD_SEG34_SHIFT   2
#define MCU_LCD_SEG35_SHIFT   3
#define MCU_LCD_SEG36_SHIFT   4
#define MCU_LCD_SEG37_SHIFT   5
#define MCU_LCD_SEG38_SHIFT   6
#define MCU_LCD_SEG39_SHIFT   7
#define MCU_LCD_SEG40_SHIFT   8
#define MCU_LCD_SEG41_SHIFT   9
#define MCU_LCD_SEG42_SHIFT   10
#define MCU_LCD_SEG43_SHIFT   11
#define MCU_LCD_SEG44_SHIFT   12

#define MCU_LCD_SEG0          (1U << MCU_LCD_SEG0_SHIFT)
#define MCU_LCD_SEG1          (1U << MCU_LCD_SEG1_SHIFT)
#define MCU_LCD_SEG2          (1U << MCU_LCD_SEG2_SHIFT)
#define MCU_LCD_SEG3          (1U << MCU_LCD_SEG3_SHIFT)
#define MCU_LCD_SEG4          (1U << MCU_LCD_SEG4_SHIFT)
#define MCU_LCD_SEG5          (1U << MCU_LCD_SEG5_SHIFT)
#define MCU_LCD_SEG6          (1U << MCU_LCD_SEG6_SHIFT)
#define MCU_LCD_SEG7          (1U << MCU_LCD_SEG7_SHIFT)
#define MCU_LCD_SEG8          (1U << MCU_LCD_SEG8_SHIFT)
#define MCU_LCD_SEG9          (1U << MCU_LCD_SEG9_SHIFT)
#define MCU_LCD_SEG11         (1U << MCU_LCD_SEG11_SHIFT)
#define MCU_LCD_SEG12         (1U << MCU_LCD_SEG12_SHIFT)
#define MCU_LCD_SEG13         (1U << MCU_LCD_SEG13_SHIFT)
#define MCU_LCD_SEG14         (1U << MCU_LCD_SEG14_SHIFT)
#define MCU_LCD_SEG15         (1U << MCU_LCD_SEG15_SHIFT)
#define MCU_LCD_SEG16         (1U << MCU_LCD_SEG16_SHIFT)
#define MCU_LCD_SEG17         (1U << MCU_LCD_SEG17_SHIFT)
#define MCU_LCD_SEG19         (1U << MCU_LCD_SEG19_SHIFT)
#define MCU_LCD_SEG21         (1U << MCU_LCD_SEG21_SHIFT)
#define MCU_LCD_SEG23         (1U << MCU_LCD_SEG23_SHIFT)
#define MCU_LCD_SEG24         (1U << MCU_LCD_SEG24_SHIFT)
#define MCU_LCD_SEG25         (1U << MCU_LCD_SEG25_SHIFT)
#define MCU_LCD_SEG26         (1U << MCU_LCD_SEG26_SHIFT)
#define MCU_LCD_SEG27         (1U << MCU_LCD_SEG27_SHIFT)
#define MCU_LCD_SEG44         (1U << MCU_LCD_SEG44_SHIFT)

// LCD_COM, LCD_SEG 바로 사용용 (main.c에서)
#define LCD_COM0          MCU_LCD_COM0
#define LCD_COM0_1        MCU_LCD_COM0_1
#define LCD_COM1          MCU_LCD_COM1
#define LCD_COM1_1        MCU_LCD_COM1_1
#define LCD_COM2          MCU_LCD_COM2
#define LCD_COM2_1        MCU_LCD_COM2_1
#define LCD_COM3          MCU_LCD_COM3
#define LCD_COM3_1        MCU_LCD_COM3_1

#define LCD_SEG0          MCU_LCD_SEG0
#define LCD_SEG1          MCU_LCD_SEG1
#define LCD_SEG2          MCU_LCD_SEG2
#define LCD_SEG3          MCU_LCD_SEG3
#define LCD_SEG4          MCU_LCD_SEG4
#define LCD_SEG5          MCU_LCD_SEG5
#define LCD_SEG6          MCU_LCD_SEG6
#define LCD_SEG7          MCU_LCD_SEG7
#define LCD_SEG8          MCU_LCD_SEG8
#define LCD_SEG9          MCU_LCD_SEG9
#define LCD_SEG11         MCU_LCD_SEG11
#define LCD_SEG12         MCU_LCD_SEG12
#define LCD_SEG13         MCU_LCD_SEG13
#define LCD_SEG14         MCU_LCD_SEG14
#define LCD_SEG15         MCU_LCD_SEG15
#define LCD_SEG16         MCU_LCD_SEG16
#define LCD_SEG17         MCU_LCD_SEG17
#define LCD_SEG19         MCU_LCD_SEG19
#define LCD_SEG21         MCU_LCD_SEG21
#define LCD_SEG23         MCU_LCD_SEG23
#define LCD_SEG24         MCU_LCD_SEG24
#define LCD_SEG25         MCU_LCD_SEG25
#define LCD_SEG26         MCU_LCD_SEG26
#define LCD_SEG27         MCU_LCD_SEG27
#define LCD_SEG44         MCU_LCD_SEG44

// ----------- 함수 선언 (lcd.c 함수 전부) --------
void LCD_Display_Dose(uint32_t dose, uint32_t *data, uint8_t Packet_dose_unit);
void LCD_DigitNumber_Dose_Calculate(uint16_t number, uint32_t *data);
void LCD_Display_Dose_Number(uint8_t thousands, uint8_t hundreds, uint8_t tens, uint8_t ones, uint32_t *data);
void LCD_Display_Temp(float Temp, uint32_t *data, uint8_t Packet_temp_unit);
void LCD_Display_Temp_MinMax(float Temp, uint32_t *data, uint8_t Packet_temp_unit);
void LCD_DigitNumber_Temp_Calculate(int32_t number, uint32_t *data);
void LCD_Display_Temp_Number(uint8_t thousands, uint8_t hundreds, uint8_t tens, uint8_t ones, uint32_t *data);
void LCD_Display_date(uint32_t *data);
void LCD_Display_Time(uint32_t *data);
void LCD_DigitNumber_Year_Hour_Calculate(uint16_t number, uint32_t *data);
void LCD_DigitNumber_Month_Min_Calculate(uint16_t number, uint32_t *data);
void LCD_DigitNumber_Day_Sec_Calculate(uint16_t number, uint32_t *data);
void LCD_Display_Year_Hour_Number(uint8_t tens, uint8_t ones, uint32_t *data);
void LCD_Display_Month_Min_Number(uint8_t tens, uint8_t ones, uint32_t *data);
void LCD_Display_Day_Sec_Number(uint8_t tens, uint8_t ones, uint32_t *data);
void LCD_Display_Battery(uint8_t battery_status, uint32_t *data);
void LCD_Display_DelayMode(uint32_t sec, uint32_t *data);

void LCD_Display_USB(uint32_t* data);
void LCD_Display_Boot(uint32_t* data);
void LCD_Display_FirstMeasure(uint32_t* data);
void LCD_Display_StartMode(uint32_t* data);
void LCD_Display_StopMode(uint32_t* data);
void LCD_Display_PauseMode(uint32_t* data);
void LCD_Display_EndMode(uint32_t *data);
void LCD_Display_Etc(uint32_t* data);
void LCD_Clear_Display(uint32_t* data);

void LCD_Display_InTime_LT(uint32_t sec, uint32_t *data);
void LCD_Display_InTime_LR(uint32_t sec, uint32_t *data);
void LCD_DigitNumber_LP_Calculate(uint16_t number, uint32_t *data);
void LCD_Display_LP_Number(uint8_t ten_thousands, uint8_t thousands, uint8_t hundreds, uint8_t tens, uint8_t ones, uint32_t *data);
void LCD_Display_InTime(uint16_t sec, uint32_t *data);
void LCD_DigitNumber_InTime_Calculate(uint16_t number, uint32_t *data);
void LCD_Display_USB_Load(uint32_t *data);

void LCD_Alarm_RH1_Display(uint32_t *data);
void LCD_Alarm_RH2_Display(uint32_t *data);
void LCD_Alarm_TH1_Display(uint32_t *data);
void LCD_Alarm_TH2_Display(uint32_t *data);
void LCD_Alarm_TL1_Display(uint32_t *data);
void LCD_Alarm_TL2_Display(uint32_t *data);
void LCD_Display_Ring_R(uint32_t *data);
void LCD_Display_Ring_T(uint32_t *data);
void LCD_Display_Alarm(uint32_t *data);
//void LCD_Display_USB();
//void LCD_Display_Boot();
//void LCD_Display_FirstMeasure();
//void LCD_Display_StartMode();
//void LCD_Display_StopMode();
//void LCD_Display_PauseMode();
//void LCD_Display_Etc();

#endif /* SRC_LCD_H_ */
