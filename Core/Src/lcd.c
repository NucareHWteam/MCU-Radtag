#include <stdint.h>
#include <stdbool.h>
#include "lcd.h"
#include "main.h"
#include "stm32u0xx_hal.h"
#include "stm32u0xx_hal_lcd.h"
#include <string.h>

extern LCD_HandleTypeDef hlcd;
extern DeviceConfig device_config;

////////////////////////////////////////////Boot//////////////////////////////////////////

////////////////////////////////////////////dose//////////////////////////////////////////


void LCD_Display_Dose(uint32_t dose, uint32_t *data, uint8_t Packet_dose_unit)
{
    int32_t dose_int = 0;
    dose = dose / 10.0;

    if (Packet_dose_unit == 0x00) // uSv/h, mSv/h unit
    {
        if (dose > 9999)
        {
            float mSv_dose = dose / 10000.0f;
            dose_int = (int32_t)(mSv_dose * 10.0f);  // mSv * 10
            LCD_DigitNumber_Dose_Calculate(dose_int, data);
            data[0] |= 0x0000008;  // mSv/h 표시
        }
        else
        {
            dose_int = (int32_t)(dose);  // uSv 그대로
            LCD_DigitNumber_Dose_Calculate(dose_int, data);
            data[2] |= 0x0800000;  // uSv/h 표시
        }
    }
    else // mR/h, R/h unit
    {
        float mR_dose = dose * 100.0f;

        if (mR_dose > 99999.0f)
        {
            float R_dose = mR_dose / 10000.0f;
            dose_int = (int32_t)(R_dose);  // R * 10
            LCD_DigitNumber_Dose_Calculate(dose_int, data);
            data[1] |= 0x0800000;  // R/h 표시
        }
        else
        {
            dose_int = (int32_t)(mR_dose);  // 반올림한 mR 값
            LCD_DigitNumber_Dose_Calculate(dose_int, data);
            data[0] |= 0x0800000;  // mR/h 표시
        }
    }
    data[3] = data[3] | 0x0800000;

	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}


void LCD_DigitNumber_Dose_Calculate(uint16_t number, uint32_t *data)
{
	uint8_t thousands= 0;
	uint8_t hundreds= 0;
	uint8_t tens= 0;
	uint8_t ones = 0;
    thousands = number / 1000;
    number %= 1000;
    hundreds  = number / 100;
    number %= 100;
    tens      = number / 10;
    ones      = number % 10;
    LCD_Display_Dose_Number(thousands, hundreds, tens, ones, data);
}

void LCD_Display_Dose_Number(uint8_t thousands, uint8_t hundreds, uint8_t tens, uint8_t ones, uint32_t *data)
{
	uint32_t Dose_data_tho[4] = {0};
	uint32_t Dose_data_hun[4] = {0};
	uint32_t Dose_data_ten[4] = {0};
	uint32_t Dose_data_one[4] = {0};

    switch (thousands)
    {
        case 0:
        	Dose_data_tho[0] = 0x0000000;
        	Dose_data_tho[1] = 0x0000000;
        	Dose_data_tho[2] = 0x0000000;
        	Dose_data_tho[3] = 0x0000000;
            break;

        case 1:
        	Dose_data_tho[0] = 0x0000000;
        	Dose_data_tho[1] = 0x1000000;
        	Dose_data_tho[2] = 0x1000000;
        	Dose_data_tho[3] = 0x0000000;
            break;

        case 2:
        	Dose_data_tho[0] = 0x1000000;
        	Dose_data_tho[1] = 0x1000000;
        	Dose_data_tho[2] = 0x2000000;
        	Dose_data_tho[3] = 0x3000000;
            break;

        case 3:
        	Dose_data_tho[0] = 0x1000000;
        	Dose_data_tho[1] = 0x1000000;
        	Dose_data_tho[2] = 0x3000000;
        	Dose_data_tho[3] = 0x1000000;
            break;

        case 4:
        	Dose_data_tho[0] = 0x0000000;
        	Dose_data_tho[1] = 0x3000000;
        	Dose_data_tho[2] = 0x3000000;
        	Dose_data_tho[3] = 0x0000000;
            break;

        case 5:
        	Dose_data_tho[0] = 0x1000000;
        	Dose_data_tho[1] = 0x2000000;
        	Dose_data_tho[2] = 0x3000000;
        	Dose_data_tho[3] = 0x1000000;
            break;

        case 6:
        	Dose_data_tho[0] = 0x1000000;
        	Dose_data_tho[1] = 0x2000000;
        	Dose_data_tho[2] = 0x3000000;
        	Dose_data_tho[3] = 0x3000000;
            break;

        case 7:
        	Dose_data_tho[0] = 0x1000000;
        	Dose_data_tho[1] = 0x3000000;
        	Dose_data_tho[2] = 0x1000000;
        	Dose_data_tho[3] = 0x0000000;
            break;

        case 8:
        	Dose_data_tho[0] = 0x1000000;
        	Dose_data_tho[1] = 0x3000000;
        	Dose_data_tho[2] = 0x3000000;
        	Dose_data_tho[3] = 0x3000000;
            break;

        case 9:
        	Dose_data_tho[0] = 0x1000000;
        	Dose_data_tho[1] = 0x3000000;
        	Dose_data_tho[2] = 0x3000000;
        	Dose_data_tho[3] = 0x1000000;
            break;
    }
    switch (hundreds)
    {
    	case 0:
    		if(thousands != 0)
			{
    			Dose_data_hun[0] = 0x0004000;
    			Dose_data_hun[1] = 0x000C000;
    			Dose_data_hun[2] = 0x0004000;
    			Dose_data_hun[3] = 0x000C000;
			}
    		else
    		{
    			Dose_data_hun[0] = 0x0000000;
    			Dose_data_hun[1] = 0x0000000;
    			Dose_data_hun[2] = 0x0000000;
    			Dose_data_hun[3] = 0x0000000;
    		}
    		break;

    	case 1:
    		Dose_data_hun[0] = 0x0000000;
    		Dose_data_hun[1] = 0x0004000;
    		Dose_data_hun[2] = 0x0004000;
    		Dose_data_hun[3] = 0x0000000;
    		break;

		case 2:
			Dose_data_hun[0] = 0x0004000;
			Dose_data_hun[1] = 0x0004000;
			Dose_data_hun[2] = 0x0008000;
			Dose_data_hun[3] = 0x000C000;
			break;

		case 3:
			Dose_data_hun[0] = 0x0004000;
			Dose_data_hun[1] = 0x0004000;
			Dose_data_hun[2] = 0x000C000;
			Dose_data_hun[3] = 0x0004000;
			break;

		case 4:
			Dose_data_hun[0] = 0x0000000;
			Dose_data_hun[1] = 0x000C000;
			Dose_data_hun[2] = 0x000C000;
			Dose_data_hun[3] = 0x0000000;
			break;

		case 5:
			Dose_data_hun[0] = 0x0004000;
			Dose_data_hun[1] = 0x0008000;
			Dose_data_hun[2] = 0x000C000;
			Dose_data_hun[3] = 0x0004000;
			break;

		case 6:
			Dose_data_hun[0] = 0x0004000;
			Dose_data_hun[1] = 0x0008000;
			Dose_data_hun[2] = 0x000C000;
			Dose_data_hun[3] = 0x000C000;
			break;

		case 7:
			Dose_data_hun[0] = 0x0004000;
			Dose_data_hun[1] = 0x000C000;
			Dose_data_hun[2] = 0x0004000;
			Dose_data_hun[3] = 0x0000000;
			break;

		case 8:
			Dose_data_hun[0] = 0x0004000;
			Dose_data_hun[1] = 0x000C000;
			Dose_data_hun[2] = 0x000C000;
			Dose_data_hun[3] = 0x000C000;
			break;

		case 9:
			Dose_data_hun[0] = 0x0004000;
			Dose_data_hun[1] = 0x000C000;
			Dose_data_hun[2] = 0x000C000;
			Dose_data_hun[3] = 0x0004000;
			break;
    }
    switch (tens)
    {
    	case 0:
    		Dose_data_ten[0] = 0x0001000;
    		Dose_data_ten[1] = 0x0003000;
    		Dose_data_ten[2] = 0x0001000;
    		Dose_data_ten[3] = 0x0003000;
    		break;

    	case 1:
    		Dose_data_ten[0] = 0x0000000;
    		Dose_data_ten[1] = 0x0001000;
    		Dose_data_ten[2] = 0x0001000;
    		Dose_data_ten[3] = 0x0000000;
    		break;

		case 2:
			Dose_data_ten[0] = 0x0001000;
			Dose_data_ten[1] = 0x0001000;
			Dose_data_ten[2] = 0x0002000;
			Dose_data_ten[3] = 0x0003000;
			break;

		case 3:
			Dose_data_ten[0] = 0x0001000;
			Dose_data_ten[1] = 0x0001000;
			Dose_data_ten[2] = 0x0003000;
			Dose_data_ten[3] = 0x0001000;
			break;

		case 4:
			Dose_data_ten[0] = 0x0000000;
			Dose_data_ten[1] = 0x0003000;
			Dose_data_ten[2] = 0x0003000;
			Dose_data_ten[3] = 0x0000000;
			break;

		case 5:
			Dose_data_ten[0] = 0x0001000;
			Dose_data_ten[1] = 0x0002000;
			Dose_data_ten[2] = 0x0003000;
			Dose_data_ten[3] = 0x0001000;
			break;

		case 6:
			Dose_data_ten[0] = 0x0001000;
			Dose_data_ten[1] = 0x0002000;
			Dose_data_ten[2] = 0x0003000;
			Dose_data_ten[3] = 0x0003000;
			break;

		case 7:
			Dose_data_ten[0] = 0x0001000;
			Dose_data_ten[1] = 0x0003000;
			Dose_data_ten[2] = 0x0001000;
			Dose_data_ten[3] = 0x0000000;
			break;

		case 8:
			Dose_data_ten[0] = 0x0001000;
			Dose_data_ten[1] = 0x0003000;
			Dose_data_ten[2] = 0x0003000;
			Dose_data_ten[3] = 0x0003000;
			break;

		case 9:
			Dose_data_ten[0] = 0x0001000;
			Dose_data_ten[1] = 0x0003000;
			Dose_data_ten[2] = 0x0003000;
			Dose_data_ten[3] = 0x0001000;
			break;
    }
    switch (ones)
    {
    	case 0:
        	Dose_data_one[0] = 0x0000040;
        	Dose_data_one[1] = 0x0000060;
        	Dose_data_one[2] = 0x0000040;
        	Dose_data_one[3] = 0x0000060;
    		break;

    	case 1:
    		Dose_data_one[0] = 0x0000000;
    		Dose_data_one[1] = 0x0000040;
    		Dose_data_one[2] = 0x0000040;
    		Dose_data_one[3] = 0x0000000;
    		break;

		case 2:
			Dose_data_one[0] = 0x0000040;
			Dose_data_one[1] = 0x0000040;
			Dose_data_one[2] = 0x0000020;
			Dose_data_one[3] = 0x0000060;
			break;

		case 3:
			Dose_data_one[0] = 0x0000040;
			Dose_data_one[1] = 0x0000040;
			Dose_data_one[2] = 0x0000060;
			Dose_data_one[3] = 0x0000040;
			break;

		case 4:
			Dose_data_one[0] = 0x0000000;
			Dose_data_one[1] = 0x0000060;
			Dose_data_one[2] = 0x0000060;
			Dose_data_one[3] = 0x0000000;
			break;

		case 5:
			Dose_data_one[0] = 0x0000040;
			Dose_data_one[1] = 0x0000020;
			Dose_data_one[2] = 0x0000060;
			Dose_data_one[3] = 0x0000040;
			break;

		case 6:
			Dose_data_one[0] = 0x0000040;
			Dose_data_one[1] = 0x0000020;
			Dose_data_one[2] = 0x0000060;
			Dose_data_one[3] = 0x0000060;
			break;

		case 7:
			Dose_data_one[0] = 0x0000040;
			Dose_data_one[1] = 0x0000060;
			Dose_data_one[2] = 0x0000040;
			Dose_data_one[3] = 0x0000000;
			break;

		case 8:
			Dose_data_one[0] = 0x0000040;
			Dose_data_one[1] = 0x0000060;
			Dose_data_one[2] = 0x0000060;
			Dose_data_one[3] = 0x0000060;
			break;

		case 9:
			Dose_data_one[0] = 0x0000040;
			Dose_data_one[1] = 0x0000060;
			Dose_data_one[2] = 0x0000060;
			Dose_data_one[3] = 0x0000040;
			break;
    }
    for(int i = 0; i<4; i++)
    {
    	data[i] = Dose_data_tho[i] | Dose_data_hun[i] | Dose_data_ten[i] | Dose_data_one[i];
    }
}
/////////////////////////////////////Temp//////////////////////////////////
void LCD_Display_Temp(float Temp, uint32_t *data, uint8_t Packet_temp_unit)
{
	HAL_LCD_Clear(&hlcd);
	memset(data, 0, sizeof(data));
    int32_t temp_int;


    if(Packet_temp_unit == 0x01) // F, C unit
    {
    	Temp = (Temp * 9.0f / 5.0f) + 32.0f;
    }

    temp_int = (int32_t)(Temp * 10.0f);
    LCD_DigitNumber_Temp_Calculate(temp_int, data);

    if(Packet_temp_unit == 0x01)
    {
        data[0] = data[0] | 0x0000004;   // F on
    }
    else
    {
        data[0] = data[0] | 0x0000200;   // C on
    }
    data[0] = data[0] | 0x0000001; // 소수점표시
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}


void LCD_Display_Temp_MinMax(float Temp, uint32_t *data, uint8_t Packet_temp_unit)
{
	HAL_LCD_Clear(&hlcd);
	memset(data, 0, sizeof(data));
    int32_t temp_int;
    float temp_float = Temp / 10.0f;


    if(Packet_temp_unit == 0x01) // F, C unit
    {
    	temp_float = (temp_float * 9.0f / 5.0f) + 32.0f;
    }

    temp_int = (int32_t)(temp_float * 10.0f);
    LCD_DigitNumber_Temp_Calculate(temp_int, data);

    if(Packet_temp_unit == 0x01)
    {
        data[0] = data[0] | 0x0000004;   // F on
    }
    else
    {
        data[0] = data[0] | 0x0000200;   // C on
    }
    data[0] = data[0] | 0x0000001; // 소수점표시
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}

void LCD_DigitNumber_Temp_Calculate(int32_t number, uint32_t *data)
{
	int8_t thousands = 0;
	int8_t hundreds= 0;
	int8_t tens= 0;
	int8_t ones = 0;
	int16_t A = 0;
	if(number < 0)
	{
		A = -number;
	}
	else
	{
		A = number;
	}
	thousands = A /1000;
	A %= 1000;
    hundreds  = A / 100;
    A %= 100;
    tens      = A / 10;
    ones      = A % 10;
    LCD_Display_Temp_Number(thousands, hundreds, tens, ones, data);
	if(number < 0 && number > -100)
	{
		data[2] = data[2] | 0x0000200;  //-부호 on, 8번세그먼트에 표시
	}
	else if(number < -99 && number > -1000)
	{
		data[2] = data[2] | 0x0000080;  //-부호 on, 7번세그먼트에 표시
	}
	else if(number < -999 && number > -10000)
	{
		data[2] = data[2] | 0x0000004;  //-부호 on, 6번세그먼트에 표시
	}
}

void LCD_Display_Temp_Number(uint8_t thousands, uint8_t hundreds, uint8_t tens, uint8_t ones, uint32_t *data)
{
	uint32_t Temp_data_tho[4] = {0};
	uint32_t Temp_data_hun[4] = {0};
	uint32_t Temp_data_ten[4] = {0};
	uint32_t Temp_data_one[4] = {0};

	switch (thousands)
	    {
	    	case 0:
	    		Temp_data_tho[0] = 0x0000000;
	    		Temp_data_tho[1] = 0x0000000;
	    		Temp_data_tho[2] = 0x0000000;
	    		Temp_data_tho[3] = 0x0000000;
	    		break;

	    	case 1:
	    		Temp_data_tho[0] = 0x0000000;
	    		Temp_data_tho[1] = 0x0000100;
	    		Temp_data_tho[2] = 0x0000100;
	    		Temp_data_tho[3] = 0x0000000;
	    		break;

			case 2:
				Temp_data_tho[0] = 0x0000100;
				Temp_data_tho[1] = 0x0000100;
				Temp_data_tho[2] = 0x0000080;
				Temp_data_tho[3] = 0x0000180;
				break;

			case 3:
				Temp_data_tho[0] = 0x0000100;
				Temp_data_tho[1] = 0x0000100;
				Temp_data_tho[2] = 0x0000180;
				Temp_data_tho[3] = 0x0000100;
				break;

			case 4:
				Temp_data_tho[0] = 0x0000000;
				Temp_data_tho[1] = 0x0000180;
				Temp_data_tho[2] = 0x0000180;
				Temp_data_tho[3] = 0x0000000;
				break;

			case 5:
				Temp_data_tho[0] = 0x0000100;
				Temp_data_tho[1] = 0x0000080;
				Temp_data_tho[2] = 0x0000180;
				Temp_data_tho[3] = 0x0000100;
				break;

			case 6:
				Temp_data_tho[0] = 0x0000100;
				Temp_data_tho[1] = 0x0000080;
				Temp_data_tho[2] = 0x0000180;
				Temp_data_tho[3] = 0x0000180;
				break;

			case 7:
				Temp_data_tho[0] = 0x0000100;
				Temp_data_tho[1] = 0x0000180;
				Temp_data_tho[2] = 0x0000100;
				Temp_data_tho[3] = 0x0000000;
				break;

			case 8:
				Temp_data_tho[0] = 0x0000100;
				Temp_data_tho[1] = 0x0000180;
				Temp_data_tho[2] = 0x0000180;
				Temp_data_tho[3] = 0x0000180;
				break;

			case 9:
				Temp_data_tho[0] = 0x0000100;
				Temp_data_tho[1] = 0x0000180;
				Temp_data_tho[2] = 0x0000180;
				Temp_data_tho[3] = 0x0000100;
				break;
	    }

    switch (hundreds)
    {
    	case 0:
    		if(thousands == 0)
    		{
    			Temp_data_hun[0] = 0x0000000;
    			Temp_data_hun[1] = 0x0000000;
    			Temp_data_hun[2] = 0x0000000;
    			Temp_data_hun[3] = 0x0000000;
    		}
    		else
    		{
    			Temp_data_hun[0] = 0x0200000;
    			Temp_data_hun[1] = 0x0200200;
    			Temp_data_hun[2] = 0x0200000;
    			Temp_data_hun[3] = 0x0200200;
    		}
    		break;

    	case 1:
    		Temp_data_hun[0] = 0x0000000;
    		Temp_data_hun[1] = 0x0200000;
    		Temp_data_hun[2] = 0x0200000;
    		Temp_data_hun[3] = 0x0000000;
    		break;

		case 2:
			Temp_data_hun[0] = 0x0200000;
			Temp_data_hun[1] = 0x0200000;
			Temp_data_hun[2] = 0x0000200;
			Temp_data_hun[3] = 0x0200200;
			break;

		case 3:
			Temp_data_hun[0] = 0x0200000;
			Temp_data_hun[1] = 0x0200000;
			Temp_data_hun[2] = 0x0200200;
			Temp_data_hun[3] = 0x0200000;
			break;

		case 4:
			Temp_data_hun[0] = 0x0000000;
			Temp_data_hun[1] = 0x0200200;
			Temp_data_hun[2] = 0x0200200;
			Temp_data_hun[3] = 0x0000000;
			break;

		case 5:
			Temp_data_hun[0] = 0x0200000;
			Temp_data_hun[1] = 0x0000200;
			Temp_data_hun[2] = 0x0200200;
			Temp_data_hun[3] = 0x0200000;
			break;

		case 6:
			Temp_data_hun[0] = 0x0200000;
			Temp_data_hun[1] = 0x0000200;
			Temp_data_hun[2] = 0x0200200;
			Temp_data_hun[3] = 0x0200200;
			break;

		case 7:
			Temp_data_hun[0] = 0x0200000;
			Temp_data_hun[1] = 0x0200200;
			Temp_data_hun[2] = 0x0200000;
			Temp_data_hun[3] = 0x0000000;
			break;

		case 8:
			Temp_data_hun[0] = 0x0200000;
			Temp_data_hun[1] = 0x0200200;
			Temp_data_hun[2] = 0x0200200;
			Temp_data_hun[3] = 0x0200200;
			break;

		case 9:
			Temp_data_hun[0] = 0x0200000;
			Temp_data_hun[1] = 0x0200200;
			Temp_data_hun[2] = 0x0200200;
			Temp_data_hun[3] = 0x0200000;
			break;
    }
    switch (tens)
    {
    	case 0:
        	Temp_data_ten[0] = 0x0080000;
        	Temp_data_ten[1] = 0x0090000;
        	Temp_data_ten[2] = 0x0080000;
        	Temp_data_ten[3] = 0x0090000;
    		break;

    	case 1:
    		Temp_data_ten[0] = 0x0000000;
    		Temp_data_ten[1] = 0x0080000;
    		Temp_data_ten[2] = 0x0080000;
    		Temp_data_ten[3] = 0x0000000;
    		break;

		case 2:
			Temp_data_ten[0] = 0x0080000;
			Temp_data_ten[1] = 0x0080000;
			Temp_data_ten[2] = 0x0010000;
			Temp_data_ten[3] = 0x0090000;
			break;

		case 3:
			Temp_data_ten[0] = 0x0080000;
			Temp_data_ten[1] = 0x0080000;
			Temp_data_ten[2] = 0x0090000;
			Temp_data_ten[3] = 0x0080000;
			break;

		case 4:
			Temp_data_ten[0] = 0x0000000;
			Temp_data_ten[1] = 0x0090000;
			Temp_data_ten[2] = 0x0090000;
			Temp_data_ten[3] = 0x0000000;
			break;

		case 5:
			Temp_data_ten[0] = 0x0080000;
			Temp_data_ten[1] = 0x0010000;
			Temp_data_ten[2] = 0x0090000;
			Temp_data_ten[3] = 0x0080000;
			break;

		case 6:
			Temp_data_ten[0] = 0x0080000;
			Temp_data_ten[1] = 0x0010000;
			Temp_data_ten[2] = 0x0090000;
			Temp_data_ten[3] = 0x0090000;
			break;

		case 7:
			Temp_data_ten[0] = 0x0080000;
			Temp_data_ten[1] = 0x0090000;
			Temp_data_ten[2] = 0x0080000;
			Temp_data_ten[3] = 0x0000000;
			break;

		case 8:
			Temp_data_ten[0] = 0x0080000;
			Temp_data_ten[1] = 0x0090000;
			Temp_data_ten[2] = 0x0090000;
			Temp_data_ten[3] = 0x0090000;
			break;

		case 9:
			Temp_data_ten[0] = 0x0080000;
			Temp_data_ten[1] = 0x0090000;
			Temp_data_ten[2] = 0x0090000;
			Temp_data_ten[3] = 0x0080000;
			break;
    }
    switch (ones)
    {
    	case 0:
        	Temp_data_one[0] = 0x0000002;
        	Temp_data_one[1] = 0x0000003;
        	Temp_data_one[2] = 0x0000002;
        	Temp_data_one[3] = 0x0000003;
    		break;

    	case 1:
    		Temp_data_one[0] = 0x0000000;
    		Temp_data_one[1] = 0x0000002;
    		Temp_data_one[2] = 0x0000002;
    		Temp_data_one[3] = 0x0000000;
    		break;

		case 2:
			Temp_data_one[0] = 0x0000002;
			Temp_data_one[1] = 0x0000002;
			Temp_data_one[2] = 0x0000001;
			Temp_data_one[3] = 0x0000003;
			break;

		case 3:
			Temp_data_one[0] = 0x0000002;
			Temp_data_one[1] = 0x0000002;
			Temp_data_one[2] = 0x0000003;
			Temp_data_one[3] = 0x0000002;
			break;

		case 4:
			Temp_data_one[0] = 0x0000000;
			Temp_data_one[1] = 0x0000003;
			Temp_data_one[2] = 0x0000003;
			Temp_data_one[3] = 0x0000000;
			break;

		case 5:
			Temp_data_one[0] = 0x0000002;
			Temp_data_one[1] = 0x0000001;
			Temp_data_one[2] = 0x0000003;
			Temp_data_one[3] = 0x0000002;
			break;

		case 6:
			Temp_data_one[0] = 0x0000002;
			Temp_data_one[1] = 0x0000001;
			Temp_data_one[2] = 0x0000003;
			Temp_data_one[3] = 0x0000003;
			break;

		case 7:
			Temp_data_one[0] = 0x0000002;
			Temp_data_one[1] = 0x0000003;
			Temp_data_one[2] = 0x0000002;
			Temp_data_one[3] = 0x0000000;
			break;

		case 8:
			Temp_data_one[0] = 0x0000002;
			Temp_data_one[1] = 0x0000003;
			Temp_data_one[2] = 0x0000003;
			Temp_data_one[3] = 0x0000003;
			break;

		case 9:
			Temp_data_one[0] = 0x0000002;
			Temp_data_one[1] = 0x0000003;
			Temp_data_one[2] = 0x0000003;
			Temp_data_one[3] = 0x0000002;
			break;
    }
    for(int i = 0; i<4; i++)
    {
    	data[i] = Temp_data_tho[i] | Temp_data_hun[i] | Temp_data_ten[i] | Temp_data_one[i];
    }
}
////////////////////////////////////////////시간_date//////////////////////////////////////////
void LCD_Display_date(uint32_t *data)
{

	HAL_LCD_Clear(&hlcd);
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    // 1) RTC에서 현재 날짜/시간 읽기 (BCD)
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BCD);

    // 2) BCD → 10진 변환
    uint8_t year  = BCD2BIN(sDate.Year);
    uint8_t month = BCD2BIN(sDate.Month);
    uint8_t day   = BCD2BIN(sDate.Date);


	uint32_t data_year[4] = {0};
	uint32_t data_month[4] = {0};
	uint32_t data_day[4] = {0};

	LCD_DigitNumber_Year_Hour_Calculate(year, data_year);
	LCD_DigitNumber_Month_Min_Calculate(month, data_month);
	LCD_DigitNumber_Day_Sec_Calculate(day, data_day);
	for(int i = 0; i<4; i++)
	{
		data[i] = data_year[i] | data_month[i] | data_day[i];
	}

	data[0] = data[0] | 0x00140C0;	//'date글자, :'표시
	data[1] = data[1] | 0x100E020;
	data[2] = data[2] | 0x300E020;
	data[3] = data[3] | 0x300B060;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}

void LCD_Display_Time(uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    // 1) RTC에서 현재 시간 읽기 (BCD)
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BCD);

    // 2) BCD → 10진 변환
    uint8_t hour   = BCD2BIN(sTime.Hours);
    uint8_t minute = BCD2BIN(sTime.Minutes);
    uint8_t second = BCD2BIN(sTime.Seconds);

	uint32_t data_hour[4] = {0};
	uint32_t data_min[4] = {0};
	uint32_t data_sec[4] = {0};

	LCD_DigitNumber_Year_Hour_Calculate(hour, data_hour);
	LCD_DigitNumber_Month_Min_Calculate(minute, data_min);
	LCD_DigitNumber_Day_Sec_Calculate(second, data_sec);
	for(int i = 0; i<4; i++)
	{
		data[i] = data_hour[i] | data_min[i] | data_sec[i];
	}

	data[0] = data[0] | 0x00100C0;	//'H--S글자, :'표시
	data[1] = data[1] | 0x3000020;
	data[2] = data[2] | 0x300A060;
	data[3] = data[3] | 0x2000040;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}
void LCD_DigitNumber_Year_Hour_Calculate(uint16_t number, uint32_t *data)
{
	uint8_t tens= 0;
	uint8_t ones = 0;
    tens      = number / 10;
    ones      = number % 10;
    LCD_Display_Year_Hour_Number(tens, ones, data);
}

void LCD_DigitNumber_Month_Min_Calculate(uint16_t number, uint32_t *data)
{
	uint8_t tens= 0;
	uint8_t ones = 0;
    tens      = number / 10;
    ones      = number % 10;
    LCD_Display_Month_Min_Number(tens, ones, data);
}

void LCD_DigitNumber_Day_Sec_Calculate(uint16_t number, uint32_t *data)
{
	uint8_t tens= 0;
	uint8_t ones = 0;
    tens      = number / 10;
    ones      = number % 10;
    LCD_Display_Day_Sec_Number(tens, ones, data);
}

void LCD_Display_Year_Hour_Number(uint8_t tens, uint8_t ones, uint32_t *data)
{
	uint32_t Year_Hour_data_ten[4] = {0};
	uint32_t Year_Hour_data_one[4] = {0};

    switch (tens)
    {
    	case 0:
        	Year_Hour_data_ten[0] = 0x0000000;
        	Year_Hour_data_ten[1] = 0x0000008;
        	Year_Hour_data_ten[2] = 0x0000000;
        	Year_Hour_data_ten[3] = 0x0000008;
        	HAL_LCD_Write(&hlcd, LCD_COM0_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM1_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM2_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM3_1 , LCD_SEG44, LCD_SEG44);
    		break;

    	case 1:
    		Year_Hour_data_ten[0] = 0x0000000;
    		Year_Hour_data_ten[1] = 0x0000000;
    		Year_Hour_data_ten[2] = 0x0000000;
    		Year_Hour_data_ten[3] = 0x0000000;
        	HAL_LCD_Write(&hlcd, LCD_COM1_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM2_1 , LCD_SEG44, LCD_SEG44);
    		break;

		case 2:
			Year_Hour_data_ten[0] = 0x0000000;
			Year_Hour_data_ten[1] = 0x0000000;
			Year_Hour_data_ten[2] = 0x0000008;
			Year_Hour_data_ten[3] = 0x0000008;
        	HAL_LCD_Write(&hlcd, LCD_COM0_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM1_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM3_1 , LCD_SEG44, LCD_SEG44);
			break;

		case 3:
			Year_Hour_data_ten[0] = 0x0000000;
			Year_Hour_data_ten[1] = 0x0000000;
			Year_Hour_data_ten[2] = 0x0000008;
			Year_Hour_data_ten[3] = 0x0000000;
        	HAL_LCD_Write(&hlcd, LCD_COM0_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM1_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM2_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM3_1 , LCD_SEG44, LCD_SEG44);
			break;

		case 4:
			Year_Hour_data_ten[0] = 0x0000000;
			Year_Hour_data_ten[1] = 0x0000008;
			Year_Hour_data_ten[2] = 0x0000008;
			Year_Hour_data_ten[3] = 0x0000000;
        	HAL_LCD_Write(&hlcd, LCD_COM1_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM2_1 , LCD_SEG44, LCD_SEG44);
			break;

		case 5:
			Year_Hour_data_ten[0] = 0x0000000;
			Year_Hour_data_ten[1] = 0x0000008;
			Year_Hour_data_ten[2] = 0x0000008;
			Year_Hour_data_ten[3] = 0x0000000;
			HAL_LCD_Write(&hlcd, LCD_COM0_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM2_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM3_1 , LCD_SEG44, LCD_SEG44);
			break;

		case 6:
			Year_Hour_data_ten[0] = 0x0000000;
			Year_Hour_data_ten[1] = 0x0000008;
			Year_Hour_data_ten[2] = 0x0000008;
			Year_Hour_data_ten[3] = 0x0000008;
        	HAL_LCD_Write(&hlcd, LCD_COM0_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM2_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM3_1 , LCD_SEG44, LCD_SEG44);
			break;

		case 7:
			Year_Hour_data_ten[0] = 0x0000000;
			Year_Hour_data_ten[1] = 0x0000008;
			Year_Hour_data_ten[2] = 0x0000000;
			Year_Hour_data_ten[3] = 0x0000000;
        	HAL_LCD_Write(&hlcd, LCD_COM0_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM1_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM2_1 , LCD_SEG44, LCD_SEG44);
			break;

		case 8:
			Year_Hour_data_ten[0] = 0x0000000;
			Year_Hour_data_ten[1] = 0x0000008;
			Year_Hour_data_ten[2] = 0x0000008;
			Year_Hour_data_ten[3] = 0x0000008;
        	HAL_LCD_Write(&hlcd, LCD_COM0_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM1_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM2_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM3_1 , LCD_SEG44, LCD_SEG44);
			break;

		case 9:
			Year_Hour_data_ten[0] = 0x0000000;
			Year_Hour_data_ten[1] = 0x0000008;
			Year_Hour_data_ten[2] = 0x0000008;
			Year_Hour_data_ten[3] = 0x0000000;
        	HAL_LCD_Write(&hlcd, LCD_COM0_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM1_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM2_1 , LCD_SEG44, LCD_SEG44);
        	HAL_LCD_Write(&hlcd, LCD_COM3_1 , LCD_SEG44, LCD_SEG44);
			break;
    }
    switch (ones)
    {
    	case 0:
        	Year_Hour_data_one[0] = 0x0020000;
        	Year_Hour_data_one[1] = 0x0020004;
        	Year_Hour_data_one[2] = 0x0020000;
        	Year_Hour_data_one[3] = 0x0020004;
    		break;

    	case 1:
    		Year_Hour_data_one[0] = 0x0000000;
    		Year_Hour_data_one[1] = 0x0020000;
    		Year_Hour_data_one[2] = 0x0020000;
    		Year_Hour_data_one[3] = 0x0000000;
    		break;

		case 2:
			Year_Hour_data_one[0] = 0x0020000;
			Year_Hour_data_one[1] = 0x0020000;
			Year_Hour_data_one[2] = 0x0000004;
			Year_Hour_data_one[3] = 0x0020004;
			break;

		case 3:
			Year_Hour_data_one[0] = 0x0020000;
			Year_Hour_data_one[1] = 0x0020000;
			Year_Hour_data_one[2] = 0x0020004;
			Year_Hour_data_one[3] = 0x0020000;
			break;

		case 4:
			Year_Hour_data_one[0] = 0x0000000;
			Year_Hour_data_one[1] = 0x0020004;
			Year_Hour_data_one[2] = 0x0020004;
			Year_Hour_data_one[3] = 0x0000000;
			break;

		case 5:
			Year_Hour_data_one[0] = 0x0020000;
			Year_Hour_data_one[1] = 0x0000004;
			Year_Hour_data_one[2] = 0x0020004;
			Year_Hour_data_one[3] = 0x0020000;
			break;

		case 6:
			Year_Hour_data_one[0] = 0x0020000;
			Year_Hour_data_one[1] = 0x0000004;
			Year_Hour_data_one[2] = 0x0020004;
			Year_Hour_data_one[3] = 0x0020004;
			break;

		case 7:
			Year_Hour_data_one[0] = 0x0020000;
			Year_Hour_data_one[1] = 0x0020004;
			Year_Hour_data_one[2] = 0x0020000;
			Year_Hour_data_one[3] = 0x0000000;
			break;

		case 8:
			Year_Hour_data_one[0] = 0x0020000;
			Year_Hour_data_one[1] = 0x0020004;
			Year_Hour_data_one[2] = 0x0020004;
			Year_Hour_data_one[3] = 0x0020004;
			break;

		case 9:
			Year_Hour_data_one[0] = 0x0020000;
			Year_Hour_data_one[1] = 0x0020004;
			Year_Hour_data_one[2] = 0x0020004;
			Year_Hour_data_one[3] = 0x0020000;
			break;
    }
    for(int i = 0; i<4; i++)
    {
    	data[i] = Year_Hour_data_ten[i] | Year_Hour_data_one[i];
    }
}

void LCD_Display_Month_Min_Number(uint8_t tens, uint8_t ones, uint32_t *data)
{
	uint32_t Month_Min_data_ten[4] = {0};
	uint32_t Month_Min_data_one[4] = {0};

    switch (tens)
    {
    	case 0:
        	Month_Min_data_ten[0] = 0x0000100;
        	Month_Min_data_ten[1] = 0x0000180;
        	Month_Min_data_ten[2] = 0x0000100;
        	Month_Min_data_ten[3] = 0x0000180;
    		break;

    	case 1:
    		Month_Min_data_ten[0] = 0x0000000;
    		Month_Min_data_ten[1] = 0x0000100;
    		Month_Min_data_ten[2] = 0x0000100;
    		Month_Min_data_ten[3] = 0x0000000;
    		break;

		case 2:
			Month_Min_data_ten[0] = 0x0000100;
			Month_Min_data_ten[1] = 0x0000100;
			Month_Min_data_ten[2] = 0x0000080;
			Month_Min_data_ten[3] = 0x0000180;
			break;

		case 3:
			Month_Min_data_ten[0] = 0x0000100;
			Month_Min_data_ten[1] = 0x0000100;
			Month_Min_data_ten[2] = 0x0000180;
			Month_Min_data_ten[3] = 0x0000100;
			break;

		case 4:
			Month_Min_data_ten[0] = 0x0000000;
			Month_Min_data_ten[1] = 0x0000180;
			Month_Min_data_ten[2] = 0x0000180;
			Month_Min_data_ten[3] = 0x0000000;
			break;

		case 5:
			Month_Min_data_ten[0] = 0x0000100;
			Month_Min_data_ten[1] = 0x0000080;
			Month_Min_data_ten[2] = 0x0000180;
			Month_Min_data_ten[3] = 0x0000100;
			break;

		case 6:
			Month_Min_data_ten[0] = 0x0000100;
			Month_Min_data_ten[1] = 0x0000080;
			Month_Min_data_ten[2] = 0x0000180;
			Month_Min_data_ten[3] = 0x0000180;
			break;

		case 7:
			Month_Min_data_ten[0] = 0x0000100;
			Month_Min_data_ten[1] = 0x0000180;
			Month_Min_data_ten[2] = 0x0000100;
			Month_Min_data_ten[3] = 0x0000000;
			break;

		case 8:
			Month_Min_data_ten[0] = 0x0000100;
			Month_Min_data_ten[1] = 0x0000180;
			Month_Min_data_ten[2] = 0x0000180;
			Month_Min_data_ten[3] = 0x0000180;
			break;

		case 9:
			Month_Min_data_ten[0] = 0x0000100;
			Month_Min_data_ten[1] = 0x0000180;
			Month_Min_data_ten[2] = 0x0000180;
			Month_Min_data_ten[3] = 0x0000100;
			break;
    }
    switch (ones)
    {
    	case 0:
        	Month_Min_data_one[0] = 0x0200000;
        	Month_Min_data_one[1] = 0x0200200;
        	Month_Min_data_one[2] = 0x0200000;
        	Month_Min_data_one[3] = 0x0200200;
    		break;

    	case 1:
    		Month_Min_data_one[0] = 0x0000000;
    		Month_Min_data_one[1] = 0x0200000;
    		Month_Min_data_one[2] = 0x0200000;
    		Month_Min_data_one[3] = 0x0000000;
    		break;

		case 2:
			Month_Min_data_one[0] = 0x0200000;
			Month_Min_data_one[1] = 0x0200000;
			Month_Min_data_one[2] = 0x0000200;
			Month_Min_data_one[3] = 0x0200200;
			break;

		case 3:
			Month_Min_data_one[0] = 0x0200000;
			Month_Min_data_one[1] = 0x0200000;
			Month_Min_data_one[2] = 0x0200200;
			Month_Min_data_one[3] = 0x0200000;
			break;

		case 4:
			Month_Min_data_one[0] = 0x0000000;
			Month_Min_data_one[1] = 0x0200200;
			Month_Min_data_one[2] = 0x0200200;
			Month_Min_data_one[3] = 0x0000000;
			break;

		case 5:
			Month_Min_data_one[0] = 0x0200000;
			Month_Min_data_one[1] = 0x0000200;
			Month_Min_data_one[2] = 0x0200200;
			Month_Min_data_one[3] = 0x0200000;
			break;

		case 6:
			Month_Min_data_one[0] = 0x0200000;
			Month_Min_data_one[1] = 0x0000200;
			Month_Min_data_one[2] = 0x0200200;
			Month_Min_data_one[3] = 0x0200200;
			break;

		case 7:
			Month_Min_data_one[0] = 0x0200000;
			Month_Min_data_one[1] = 0x0200200;
			Month_Min_data_one[2] = 0x0200000;
			Month_Min_data_one[3] = 0x0000000;
			break;

		case 8:
			Month_Min_data_one[0] = 0x0200000;
			Month_Min_data_one[1] = 0x0200200;
			Month_Min_data_one[2] = 0x0200200;
			Month_Min_data_one[3] = 0x0200200;
			break;

		case 9:
			Month_Min_data_one[0] = 0x0200000;
			Month_Min_data_one[1] = 0x0200200;
			Month_Min_data_one[2] = 0x0200200;
			Month_Min_data_one[3] = 0x0200000;
			break;
    }
    for(int i = 0; i<4; i++)
    {
    	data[i] = Month_Min_data_ten[i] | Month_Min_data_one[i];
    }
}

void LCD_Display_Day_Sec_Number(uint8_t tens, uint8_t ones, uint32_t *data)
{
	uint32_t Day_Sec_data_ten[4] = {0};
	uint32_t Day_Sec_data_one[4] = {0};

    switch (tens)
    {
    	case 0:
        	Day_Sec_data_ten[0] = 0x0080000;
        	Day_Sec_data_ten[1] = 0x0090000;
        	Day_Sec_data_ten[2] = 0x0080000;
        	Day_Sec_data_ten[3] = 0x0090000;
    		break;

    	case 1:
    		Day_Sec_data_ten[0] = 0x0000000;
    		Day_Sec_data_ten[1] = 0x0080000;
    		Day_Sec_data_ten[2] = 0x0080000;
    		Day_Sec_data_ten[3] = 0x0000000;
    		break;

		case 2:
			Day_Sec_data_ten[0] = 0x0080000;
			Day_Sec_data_ten[1] = 0x0080000;
			Day_Sec_data_ten[2] = 0x0010000;
			Day_Sec_data_ten[3] = 0x0090000;
			break;

		case 3:
			Day_Sec_data_ten[0] = 0x0080000;
			Day_Sec_data_ten[1] = 0x0080000;
			Day_Sec_data_ten[2] = 0x0090000;
			Day_Sec_data_ten[3] = 0x0080000;
			break;

		case 4:
			Day_Sec_data_ten[0] = 0x0000000;
			Day_Sec_data_ten[1] = 0x0090000;
			Day_Sec_data_ten[2] = 0x0090000;
			Day_Sec_data_ten[3] = 0x0000000;
			break;

		case 5:
			Day_Sec_data_ten[0] = 0x0080000;
			Day_Sec_data_ten[1] = 0x0010000;
			Day_Sec_data_ten[2] = 0x0090000;
			Day_Sec_data_ten[3] = 0x0080000;
			break;

		case 6:
			Day_Sec_data_ten[0] = 0x0080000;
			Day_Sec_data_ten[1] = 0x0010000;
			Day_Sec_data_ten[2] = 0x0090000;
			Day_Sec_data_ten[3] = 0x0090000;
			break;

		case 7:
			Day_Sec_data_ten[0] = 0x0080000;
			Day_Sec_data_ten[1] = 0x0090000;
			Day_Sec_data_ten[2] = 0x0080000;
			Day_Sec_data_ten[3] = 0x0000000;
			break;

		case 8:
			Day_Sec_data_ten[0] = 0x0080000;
			Day_Sec_data_ten[1] = 0x0090000;
			Day_Sec_data_ten[2] = 0x0090000;
			Day_Sec_data_ten[3] = 0x0090000;
			break;

		case 9:
			Day_Sec_data_ten[0] = 0x0080000;
			Day_Sec_data_ten[1] = 0x0090000;
			Day_Sec_data_ten[2] = 0x0090000;
			Day_Sec_data_ten[3] = 0x0080000;
			break;
    }
    switch (ones)
    {
    	case 0:
        	Day_Sec_data_one[0] = 0x0000002;
        	Day_Sec_data_one[1] = 0x0000003;
        	Day_Sec_data_one[2] = 0x0000002;
        	Day_Sec_data_one[3] = 0x0000003;
    		break;

    	case 1:
    		Day_Sec_data_one[0] = 0x0000000;
    		Day_Sec_data_one[1] = 0x0000002;
    		Day_Sec_data_one[2] = 0x0000002;
    		Day_Sec_data_one[3] = 0x0000000;
    		break;

		case 2:
			Day_Sec_data_one[0] = 0x0000002;
			Day_Sec_data_one[1] = 0x0000002;
			Day_Sec_data_one[2] = 0x0000001;
			Day_Sec_data_one[3] = 0x0000003;
			break;

		case 3:
			Day_Sec_data_one[0] = 0x0000002;
			Day_Sec_data_one[1] = 0x0000002;
			Day_Sec_data_one[2] = 0x0000003;
			Day_Sec_data_one[3] = 0x0000002;
			break;

		case 4:
			Day_Sec_data_one[0] = 0x0000000;
			Day_Sec_data_one[1] = 0x0000003;
			Day_Sec_data_one[2] = 0x0000003;
			Day_Sec_data_one[3] = 0x0000000;
			break;

		case 5:
			Day_Sec_data_one[0] = 0x0000002;
			Day_Sec_data_one[1] = 0x0000001;
			Day_Sec_data_one[2] = 0x0000003;
			Day_Sec_data_one[3] = 0x0000002;
			break;

		case 6:
			Day_Sec_data_one[0] = 0x0000002;
			Day_Sec_data_one[1] = 0x0000001;
			Day_Sec_data_one[2] = 0x0000003;
			Day_Sec_data_one[3] = 0x0000003;
			break;

		case 7:
			Day_Sec_data_one[0] = 0x0000002;
			Day_Sec_data_one[1] = 0x0000003;
			Day_Sec_data_one[2] = 0x0000002;
			Day_Sec_data_one[3] = 0x0000000;
			break;

		case 8:
			Day_Sec_data_one[0] = 0x0000002;
			Day_Sec_data_one[1] = 0x0000003;
			Day_Sec_data_one[2] = 0x0000003;
			Day_Sec_data_one[3] = 0x0000003;
			break;

		case 9:
			Day_Sec_data_one[0] = 0x0000002;
			Day_Sec_data_one[1] = 0x0000003;
			Day_Sec_data_one[2] = 0x0000003;
			Day_Sec_data_one[3] = 0x0000002;
			break;
    }
    for(int i = 0; i<4; i++)
    {
    	data[i] = Day_Sec_data_ten[i] | Day_Sec_data_one[i];
    }
}
////////////////////////////////////////////배터리//////////////////////////////////////////
void LCD_Display_Battery(uint8_t battery_status, uint32_t *data)
{
	if(battery_status == 0)
	{
		data[0] = data[0] | 0x0000800;
		data[1] = data[1] | 0x0000800;
		data[2] = data[2] | 0x0000800;
		data[3] = data[3] | 0x0000800;
		//배터리 3칸 + 배터리박스 on
	}
	else if(battery_status == 1)
	{
		data[0] = data[0] | 0x0000000;
		data[1] = data[1] | 0x0000800;
		data[2] = data[2] | 0x0000800;
		data[3] = data[3] | 0x0000800;
		//배터리 2칸 + 배터리박스 on
	}
	else if(battery_status == 2)
	{
		data[0] = data[0] | 0x0000000;
		data[1] = data[1] | 0x0000800;
		data[2] = data[2] | 0x0000000;
		data[3] = data[3] | 0x0000800;
		//배터리 1칸 + 배터리박스 on
	}
	else if(battery_status == 3)
	{
		data[0] = data[0] | 0x0000000;
		data[1] = data[1] | 0x0000000;
		data[2] = data[2] | 0x0000000;
		data[3] = data[3] | 0x0000800;
		//배터리 0칸 + 배터리박스 on
	}
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}

////////////////////////////////////////////usb//////////////////////////////////////////
void LCD_Display_USB(uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
	memset(data, 0, sizeof(data));
	data[0] = data[0] | 0x0004000;
	data[1] = data[1] | 0x300A000;
	data[2] = data[2] | 0x100F000;
	data[3] = data[3] | 0x3007000;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
    HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	HAL_LCD_UpdateDisplayRequest(& hlcd);
	//Usb 글자 on
}

void LCD_Display_USB_Load(uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
	memset(data, 0, sizeof(data));
	data[0] = data[0] | 0x0004100;
	data[1] = data[1] | 0x320A188;
	data[2] = data[2] | 0x122F384;
	data[3] = data[3] | 0x322728C;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
    HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	HAL_LCD_Write(&hlcd, LCD_COM3_1 , LCD_SEG44, LCD_SEG44);
	HAL_LCD_UpdateDisplayRequest(& hlcd);
	//Usb LoAd글자 on
}
////////////////////////////////////////////나머지 글자 및 알람//////////////////////////////////////////
void LCD_Display_Boot(uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
	memset(data, 0, sizeof(data));
	data[1] = data[1] | 0x2000020;
	data[2] = data[2] | 0x300F020;
	data[3] = data[3] | 0x300F060;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
	//boot 글자 on
}

void Blank_Dose_Display(uint32_t *data)
{
	data[2] = data[2] | 0x200A020;
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
	//dose값 나오기전 '----' 표시
}

void LCD_Display_StartMode(uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
	memset(data, 0, sizeof(data));
	data[0] = data[0] | 0x0000100;
	data[1] = data[1] | 0x001038C;
	data[2] = data[2] | 0x001038C;
	data[3] = data[3] | 0x00B0294;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
    HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	HAL_LCD_Write(&hlcd, LCD_COM0_1 , LCD_SEG44, LCD_SEG44);
	HAL_LCD_Write(&hlcd, LCD_COM2_1 , LCD_SEG44, LCD_SEG44);
	HAL_LCD_Write(&hlcd, LCD_COM3_1 , LCD_SEG44, LCD_SEG44);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
	//start 글자, REC 표시
}

void LCD_Display_StopMode(uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
	memset(data, 0, sizeof(data));
	data[0] = data[0] | 0x0200100;
	data[1] = data[1] | 0x020038C;
	data[2] = data[2] | 0x000031C;
	data[3] = data[3] | 0x0020384;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
    HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	HAL_LCD_Write(&hlcd, LCD_COM0_1 , LCD_SEG44, LCD_SEG44);
	HAL_LCD_Write(&hlcd, LCD_COM2_1 , LCD_SEG44, LCD_SEG44);
	HAL_LCD_Write(&hlcd, LCD_COM3_1 , LCD_SEG44, LCD_SEG44);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
	//stop 글자, stop 표시
}

void LCD_Display_PauseMode(uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
	memset(data, 0, sizeof(data));
	data[0] = data[0] | 0x02A0000;
	data[1] = data[1] | 0x003038C;
	data[2] = data[2] | 0x023031C;
	data[3] = data[3] | 0x029018C;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
    HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	HAL_LCD_Write(&hlcd, LCD_COM0_1 , LCD_SEG44, LCD_SEG44);
	HAL_LCD_Write(&hlcd, LCD_COM1_1 , LCD_SEG44, LCD_SEG44);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
	//pause 글자, stop 표시
}

void LCD_Display_Etc(uint32_t *data)
{
	data[1] = data[1] | 0x0000010; //L-R on
	data[0] = data[0] | 0x0000010; //L-T on
	data[3] = data[3] | 0x4000000; //Delay on
	data[3] = data[3] | 0x0000010; //Rec on
	data[2] = data[2] | 0x0000010; //Stop on
	data[2] = data[2] | 0x4000000; //R-ring on
	data[1] = data[1] | 0x4000000; //T-ring on
	data[0] = data[0] | 0x8000000; //RH1
	data[1] = data[1] | 0x8000000; //RH2
	data[2] = data[2] | 0x8000000; //TH1
	data[3] = data[3] | 0x8000000; //TH2
	data[0] = data[0] | 0x0008000; //TL1
	data[0] = data[0] | 0x0002000; //TL2
	data[0] = data[0] | 0x4000000;//Mark
	data[0] = data[0] | 0x2000000; //MAX
	data[0] = data[0] | 0x0000020; //MIN
}


void Small_Stop_Display(uint32_t *data)
{
	data[2] = data[2] | 0x0000010; //Stop on
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);

}

void Small_Start_Display(uint32_t *data)
{
	data[3] = data[3] | 0x0000010; //Rec on
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}

void Small_Delay_Display(uint32_t *data)
{
	data[3] = data[3] | 0x4000000; //Delay on
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}
void Mark_Display(uint32_t *data){
	if(device_config.mark == 1) {
	data[0] = data[0] | 0x4000000;//Mark
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
	}
}
void Log_Mark_Display(uint32_t *data){
	HAL_LCD_Clear(&hlcd);
	data[0] = data[0] | 0x0000010; //L-R on
	data[1] = data[1] | 0x0000010; //L-T on
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}

void Max_Display(uint32_t *data){
	data[0] = data[0] | 0x2000000; //MAX
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}

void Min_Display(uint32_t *data){
	data[0] = data[0] | 0x0000020; //MIN
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}

void LCD_Clear(void)
{
	HAL_LCD_Clear(&hlcd);
}
void LCD_Clear_Display(uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
	data[0] = 0x0000000;
	data[1] = 0x0000000;
	data[2] = 0x0000000;
	data[3] = 0x0000000;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);

	HAL_LCD_UpdateDisplayRequest(& hlcd);
}
////////////////////////////////////////////Logging point//////////////////////////////////////////
void LCD_Display_LP(uint32_t index, uint32_t *data, bool Packet_dose_unit)
{
	HAL_LCD_Clear(&hlcd);
	LCD_DigitNumber_LP_Calculate(index, data);
	if (Packet_dose_unit)
	data[1] = data[1] | 0x0000010; //L-R on
	else
	data[0] = data[0] | 0x0000010; //L-T on

	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}

void LCD_Display_DelayMode(uint32_t sec, uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
	memset(data, 0, sizeof(data));
	HAL_LCD_Clear(&hlcd);
	LCD_DigitNumber_InTime_Calculate(sec, data);
	data[0] = data[0] | 0x0010080; // 'Delay, :' 표시
	data[3] = data[3] | 0x4000000;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
    HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
	//stop 글자, stop 표시
}
void LCD_DigitNumber_LP_Calculate(uint16_t number, uint32_t *data)
{
	uint8_t ten_thousands = 0;
	uint8_t thousands= 0;
	uint8_t hundreds= 0;
	uint8_t tens= 0;
	uint8_t ones = 0;
	ten_thousands = number / 10000;
	number %= 10000;
    thousands = number / 1000;
    number %= 1000;
    hundreds  = number / 100;
    number %= 100;
    tens      = number / 10;
    ones      = number % 10;
    LCD_Display_LP_Number(ten_thousands, thousands, hundreds, tens, ones, data);
}

void LCD_Display_LP_Number(uint8_t ten_thousands, uint8_t thousands, uint8_t hundreds, uint8_t tens, uint8_t ones, uint32_t *data)
{
	uint32_t LP_data_tentho[4] = {0};
	uint32_t LP_data_tho[4] = {0};
	uint32_t LP_data_hun[4] = {0};
	uint32_t LP_data_ten[4] = {0};
	uint32_t LP_data_one[4] = {0};

	switch (ten_thousands)
		    {
		    	case 0:
		    		LP_data_tentho[0] = 0x0000000;
		    		LP_data_tentho[1] = 0x0000000;
		    		LP_data_tentho[2] = 0x0000000;
		    		LP_data_tentho[3] = 0x0000000;
		    		break;

		    	case 1:
		    		LP_data_tentho[0] = 0x0000000;
		    		LP_data_tentho[1] = 0x0020000;
		    		LP_data_tentho[2] = 0x0020000;
		    		LP_data_tentho[3] = 0x0000000;
		    		break;

				case 2:
					LP_data_tentho[0] = 0x0020000;
					LP_data_tentho[1] = 0x0020000;
					LP_data_tentho[2] = 0x0000004;
					LP_data_tentho[3] = 0x0020004;
					break;

				case 3:
					LP_data_tentho[0] = 0x0020000;
					LP_data_tentho[1] = 0x0020000;
					LP_data_tentho[2] = 0x0020004;
					LP_data_tentho[3] = 0x0020000;
					break;

				case 4:
					LP_data_tentho[0] = 0x0000000;
					LP_data_tentho[1] = 0x0020004;
					LP_data_tentho[2] = 0x0020004;
					LP_data_tentho[3] = 0x0000000;
					break;

				case 5:
					LP_data_tentho[0] = 0x0020000;
					LP_data_tentho[1] = 0x0000004;
					LP_data_tentho[2] = 0x0020004;
					LP_data_tentho[3] = 0x0020000;
					break;

				case 6:
					LP_data_tentho[0] = 0x0020000;
					LP_data_tentho[1] = 0x0000004;
					LP_data_tentho[2] = 0x0020004;
					LP_data_tentho[3] = 0x0020004;
					break;

				case 7:
					LP_data_tentho[0] = 0x0020000;
					LP_data_tentho[1] = 0x0020004;
					LP_data_tentho[2] = 0x0020000;
					LP_data_tentho[3] = 0x0000000;
					break;

				case 8:
					LP_data_tentho[0] = 0x0020000;
					LP_data_tentho[1] = 0x0020004;
					LP_data_tentho[2] = 0x0020004;
					LP_data_tentho[3] = 0x0020004;
					break;

				case 9:
					LP_data_tentho[0] = 0x0020000;
					LP_data_tentho[1] = 0x0020004;
					LP_data_tentho[2] = 0x0020004;
					LP_data_tentho[3] = 0x0020000;
					break;
		    }


	switch (thousands)
	    {
			case 0:
				if(ten_thousands == 0)
				{
					LP_data_tho[0] = 0x0000000;
					LP_data_tho[1] = 0x0000000;
					LP_data_tho[2] = 0x0000000;
					LP_data_tho[3] = 0x0000000;
	    			break;
				}
				else
				{
					LP_data_tho[0] = 0x0000100;
					LP_data_tho[1] = 0x0000180;
					LP_data_tho[2] = 0x0000100;
					LP_data_tho[3] = 0x0000180;
				}
	    	case 1:
	    		LP_data_tho[0] = 0x0000000;
	    		LP_data_tho[1] = 0x0000100;
	    		LP_data_tho[2] = 0x0000100;
	    		LP_data_tho[3] = 0x0000000;
	    		break;

			case 2:
				LP_data_tho[0] = 0x0000100;
				LP_data_tho[1] = 0x0000100;
				LP_data_tho[2] = 0x0000080;
				LP_data_tho[3] = 0x0000180;
				break;

			case 3:
				LP_data_tho[0] = 0x0000100;
				LP_data_tho[1] = 0x0000100;
				LP_data_tho[2] = 0x0000180;
				LP_data_tho[3] = 0x0000100;
				break;

			case 4:
				LP_data_tho[0] = 0x0000000;
				LP_data_tho[1] = 0x0000180;
				LP_data_tho[2] = 0x0000180;
				LP_data_tho[3] = 0x0000000;
				break;

			case 5:
				LP_data_tho[0] = 0x0000100;
				LP_data_tho[1] = 0x0000080;
				LP_data_tho[2] = 0x0000180;
				LP_data_tho[3] = 0x0000100;
				break;

			case 6:
				LP_data_tho[0] = 0x0000100;
				LP_data_tho[1] = 0x0000080;
				LP_data_tho[2] = 0x0000180;
				LP_data_tho[3] = 0x0000180;
				break;

			case 7:
				LP_data_tho[0] = 0x0000100;
				LP_data_tho[1] = 0x0000180;
				LP_data_tho[2] = 0x0000100;
				LP_data_tho[3] = 0x0000000;
				break;

			case 8:
				LP_data_tho[0] = 0x0000100;
				LP_data_tho[1] = 0x0000180;
				LP_data_tho[2] = 0x0000180;
				LP_data_tho[3] = 0x0000180;
				break;

			case 9:
				LP_data_tho[0] = 0x0000100;
				LP_data_tho[1] = 0x0000180;
				LP_data_tho[2] = 0x0000180;
				LP_data_tho[3] = 0x0000100;
				break;
	    }

    switch (hundreds)
    {
    	case 0:
    		if(ten_thousands == 0 && thousands == 0)
    		{
    			LP_data_hun[0] = 0x0000000;
    			LP_data_hun[1] = 0x0000000;
    			LP_data_hun[2] = 0x0000000;
    			LP_data_hun[3] = 0x0000000;
    		}
    		else
    		{
    			LP_data_hun[0] = 0x0200000;
    			LP_data_hun[1] = 0x0200200;
    			LP_data_hun[2] = 0x0200000;
    			LP_data_hun[3] = 0x0200200;
    		}
    		break;

    	case 1:
    		LP_data_hun[0] = 0x0000000;
    		LP_data_hun[1] = 0x0200000;
    		LP_data_hun[2] = 0x0200000;
    		LP_data_hun[3] = 0x0000000;
    		break;

		case 2:
			LP_data_hun[0] = 0x0200000;
			LP_data_hun[1] = 0x0200000;
			LP_data_hun[2] = 0x0000200;
			LP_data_hun[3] = 0x0200200;
			break;

		case 3:
			LP_data_hun[0] = 0x0200000;
			LP_data_hun[1] = 0x0200000;
			LP_data_hun[2] = 0x0200200;
			LP_data_hun[3] = 0x0200000;
			break;

		case 4:
			LP_data_hun[0] = 0x0000000;
			LP_data_hun[1] = 0x0200200;
			LP_data_hun[2] = 0x0200200;
			LP_data_hun[3] = 0x0000000;
			break;

		case 5:
			LP_data_hun[0] = 0x0200000;
			LP_data_hun[1] = 0x0000200;
			LP_data_hun[2] = 0x0200200;
			LP_data_hun[3] = 0x0200000;
			break;

		case 6:
			LP_data_hun[0] = 0x0200000;
			LP_data_hun[1] = 0x0000200;
			LP_data_hun[2] = 0x0200200;
			LP_data_hun[3] = 0x0200200;
			break;

		case 7:
			LP_data_hun[0] = 0x0200000;
			LP_data_hun[1] = 0x0200200;
			LP_data_hun[2] = 0x0200000;
			LP_data_hun[3] = 0x0000000;
			break;

		case 8:
			LP_data_hun[0] = 0x0200000;
			LP_data_hun[1] = 0x0200200;
			LP_data_hun[2] = 0x0200200;
			LP_data_hun[3] = 0x0200200;
			break;

		case 9:
			LP_data_hun[0] = 0x0200000;
			LP_data_hun[1] = 0x0200200;
			LP_data_hun[2] = 0x0200200;
			LP_data_hun[3] = 0x0200000;
			break;
    }
    switch (tens)
    {
    	case 0:
    		if(ten_thousands == 0 && thousands == 0 && hundreds == 0)
    		{
    			LP_data_ten[0] = 0x0000000;
    			LP_data_ten[1] = 0x0000000;
    			LP_data_ten[2] = 0x0000000;
    			LP_data_ten[3] = 0x0000000;
    			break;
    		}
    		else
    		{
    			LP_data_ten[0] = 0x0080000;
    			LP_data_ten[1] = 0x0090000;
    			LP_data_ten[2] = 0x0080000;
    			LP_data_ten[3] = 0x0090000;
    			break;
    		}

    	case 1:
    		LP_data_ten[0] = 0x0000000;
    		LP_data_ten[1] = 0x0080000;
    		LP_data_ten[2] = 0x0080000;
    		LP_data_ten[3] = 0x0000000;
    		break;

		case 2:
			LP_data_ten[0] = 0x0080000;
			LP_data_ten[1] = 0x0080000;
			LP_data_ten[2] = 0x0010000;
			LP_data_ten[3] = 0x0090000;
			break;

		case 3:
			LP_data_ten[0] = 0x0080000;
			LP_data_ten[1] = 0x0080000;
			LP_data_ten[2] = 0x0090000;
			LP_data_ten[3] = 0x0080000;
			break;

		case 4:
			LP_data_ten[0] = 0x0000000;
			LP_data_ten[1] = 0x0090000;
			LP_data_ten[2] = 0x0090000;
			LP_data_ten[3] = 0x0000000;
			break;

		case 5:
			LP_data_ten[0] = 0x0080000;
			LP_data_ten[1] = 0x0010000;
			LP_data_ten[2] = 0x0090000;
			LP_data_ten[3] = 0x0080000;
			break;

		case 6:
			LP_data_ten[0] = 0x0080000;
			LP_data_ten[1] = 0x0010000;
			LP_data_ten[2] = 0x0090000;
			LP_data_ten[3] = 0x0090000;
			break;

		case 7:
			LP_data_ten[0] = 0x0080000;
			LP_data_ten[1] = 0x0090000;
			LP_data_ten[2] = 0x0080000;
			LP_data_ten[3] = 0x0000000;
			break;

		case 8:
			LP_data_ten[0] = 0x0080000;
			LP_data_ten[1] = 0x0090000;
			LP_data_ten[2] = 0x0090000;
			LP_data_ten[3] = 0x0090000;
			break;

		case 9:
			LP_data_ten[0] = 0x0080000;
			LP_data_ten[1] = 0x0090000;
			LP_data_ten[2] = 0x0090000;
			LP_data_ten[3] = 0x0080000;
			break;
    }
    switch (ones)
    {
    	case 0:
        	LP_data_one[0] = 0x0000002;
        	LP_data_one[1] = 0x0000003;
        	LP_data_one[2] = 0x0000002;
        	LP_data_one[3] = 0x0000003;
    		break;

    	case 1:
    		LP_data_one[0] = 0x0000000;
    		LP_data_one[1] = 0x0000002;
    		LP_data_one[2] = 0x0000002;
    		LP_data_one[3] = 0x0000000;
    		break;

		case 2:
			LP_data_one[0] = 0x0000002;
			LP_data_one[1] = 0x0000002;
			LP_data_one[2] = 0x0000001;
			LP_data_one[3] = 0x0000003;
			break;

		case 3:
			LP_data_one[0] = 0x0000002;
			LP_data_one[1] = 0x0000002;
			LP_data_one[2] = 0x0000003;
			LP_data_one[3] = 0x0000002;
			break;

		case 4:
			LP_data_one[0] = 0x0000000;
			LP_data_one[1] = 0x0000003;
			LP_data_one[2] = 0x0000003;
			LP_data_one[3] = 0x0000000;
			break;

		case 5:
			LP_data_one[0] = 0x0000002;
			LP_data_one[1] = 0x0000001;
			LP_data_one[2] = 0x0000003;
			LP_data_one[3] = 0x0000002;
			break;

		case 6:
			LP_data_one[0] = 0x0000002;
			LP_data_one[1] = 0x0000001;
			LP_data_one[2] = 0x0000003;
			LP_data_one[3] = 0x0000003;
			break;

		case 7:
			LP_data_one[0] = 0x0000002;
			LP_data_one[1] = 0x0000003;
			LP_data_one[2] = 0x0000002;
			LP_data_one[3] = 0x0000000;
			break;

		case 8:
			LP_data_one[0] = 0x0000002;
			LP_data_one[1] = 0x0000003;
			LP_data_one[2] = 0x0000003;
			LP_data_one[3] = 0x0000003;
			break;

		case 9:
			LP_data_one[0] = 0x0000002;
			LP_data_one[1] = 0x0000003;
			LP_data_one[2] = 0x0000003;
			LP_data_one[3] = 0x0000002;
			break;
    }
    for(int i = 0; i<4; i++)
    {
    	data[i] = LP_data_tentho[i] | LP_data_tho[i] | LP_data_hun[i] | LP_data_ten[i] | LP_data_one[i];
    }
}

////////////////////////////////////////////interval time//////////////////////////////////////////
void LCD_Display_InTime_LT(uint32_t sec, uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
	LCD_DigitNumber_InTime_Calculate(sec, data);
	data[0] = data[0] | 0x1011090;//'REC, :, StAr,LT'표시
	data[1] = data[1] | 0x200B000;
	data[2] = data[2] | 0x300B020;
	data[3] = data[3] | 0x100E030;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}
void LCD_Display_InTime_LR(uint32_t sec, uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
	LCD_DigitNumber_InTime_Calculate(sec, data);
	data[0] = data[0] | 0x1011080;//'REC, :, StAr,LR'표시
	data[1] = data[1] | 0x200B010;
	data[2] = data[2] | 0x300B020;
	data[3] = data[3] | 0x100E030;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
}
void LCD_DigitNumber_InTime_Calculate(uint16_t number, uint32_t *data)
{
	uint8_t h1= 0;
	uint8_t h0= 0;
	uint8_t m1= 0;
	uint8_t m0= 0;
	uint8_t s1= 0;
	uint8_t s0= 0;
	uint8_t hours = 0;
	uint8_t minutes = 0;
	uint8_t seconds = 0;
	uint32_t data_hour[4] = {0};
	uint32_t data_min[4] = {0};
	uint32_t data_sec[4] = {0};

    hours   = number / 3600;
    number %= 3600;
    minutes = number / 60;
    seconds = number % 60;

    h1 = hours / 10;
    h0 = hours % 10;
    m1 = minutes / 10;
    m0 = minutes % 10;
    s1 = seconds / 10;
    s0 = seconds % 10;

    LCD_Display_Year_Hour_Number(h1, h0, data_hour);
    LCD_Display_Month_Min_Number(m1, m0, data_min);
    LCD_Display_Day_Sec_Number(s1, s0, data_sec);
    for(int i=0; i<4; i++)
    {
    	data[i] = data_hour[i] | data_min[i] | data_sec[i];
    }
}

void LCD_Display_EndMode(uint32_t *data)
{
	HAL_LCD_Clear(&hlcd);
	memset(data, 0, sizeof(data));
	data[0] = data[0] | 0x1000000; // 'End' 표시
	data[1] = data[1] | 0x2001000;
	data[2] = data[2] | 0x200F000;
	data[3] = data[3] | 0x300B000;
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
    HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
	//HAL_LCD_UpdateDisplayRequest(& hlcd);
	//stop 글자, stop 표시
}
void LCD_Alarm_RH1_Display(uint32_t *data){
	data[3] = data[3] | 0x8000000; // 'RH1' 표시
	HAL_LCD_Write(& hlcd, LCD_COM3 , ~(LCD_SEG0 | LCD_SEG27),data[3]);
}

void LCD_Alarm_RH2_Display(uint32_t *data){
	data[2] = data[2] | 0x8000000; // 'RH2' 표시
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
}

void LCD_Alarm_TH1_Display(uint32_t *data){
	data[1] = data[1] | 0x8000000; // 'TH1' 표시
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
}

void LCD_Alarm_TH2_Display(uint32_t *data){
	data[0] = data[0] | 0x8000000;// 'TH2' 표시
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
}

void LCD_Alarm_TL1_Display(uint32_t *data){
	data[0] = data[0] | 0x0008000;// 'TL1' 표시
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
}

void LCD_Alarm_TL2_Display(uint32_t *data){
	data[0] = data[0] | 0x0002000;// 'TL2' 표시
	HAL_LCD_Write(& hlcd, LCD_COM0 , ~(LCD_SEG0 | LCD_SEG27),data[0]);
}

void LCD_Display_Ring_R(uint32_t *data){
	data[2] = data[2] | 0x4000000;// 'Ring R' 표시
	HAL_LCD_Write(& hlcd, LCD_COM2 , ~(LCD_SEG0 | LCD_SEG27),data[2]);
}

void LCD_Display_Ring_T(uint32_t *data){
	data[1] = data[1] | 0x4000000;// 'Ring T' 표시
	HAL_LCD_Write(& hlcd, LCD_COM1 , ~(LCD_SEG0 | LCD_SEG27),data[1]);
}
void LCD_Display_Alarm(uint32_t *data){
	AlarmState RAD_ALARM  = (GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1) == ALARM_ON) ||
	               	  	  	(GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2) == ALARM_ON);

	AlarmState TEMP_ALARM = (GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1) == ALARM_ON) ||
	               	  	  	(GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2) == ALARM_ON) ||
							(GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1) == ALARM_ON) ||
							(GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2) == ALARM_ON);

	if(GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1) == ALARM_ON)
	{
		LCD_Alarm_RH1_Display(data);
	}
	if(GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2) == ALARM_ON)
	{
		LCD_Alarm_RH2_Display(data);
	}
	if(GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1) == ALARM_ON)
	{
		LCD_Alarm_TH1_Display(data);
	}
	if(GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2) == ALARM_ON)
	{
		LCD_Alarm_TH2_Display(data);
	}
	if(GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1) == ALARM_ON)
	{
		LCD_Alarm_TL1_Display(data);
	}
	if(GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2) == ALARM_ON)
	{
		LCD_Alarm_TL2_Display(data);
	}

	if(RAD_ALARM != ALARM_DISABLE)
	{
		LCD_Display_Ring_R(data);
	}
	if(TEMP_ALARM != ALARM_DISABLE)
	{
		LCD_Display_Ring_T(data);
	}
}
