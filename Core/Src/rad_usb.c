/*
 * rad_usb.c
 *
 *  Created on: Jun 18, 2025
 *      Author: dongo
 */
#include <string.h>
#include "rad_usb.h"
#include "main.h"
#include "ux_dcd_stm32.h"
#include "spi_flash.h"
#include "meas_data_log.h"
#include "app_usbx_device.h"
#include "ff.h"
#include "usb_media_common.h"
#include "log_system.h"
#include "sensor_log.h"
//void Set_EndTarget_FromNow_Days(uint16_t days);
#define DEBUG_LOG 1

//FIXME: why should not define in headers?
#define BCD2BIN(x)  ((((x) >> 4) * 10) + ((x) & 0x0F))

FATFS fs;      // File system object
FRESULT res;   // Result code
BYTE work[FF_MAX_SS]; /* Work area (larger is better for processing time) */

//FIXME: Create USB state machine?
volatile int usbx_host_req_records =0;



extern PCD_HandleTypeDef hpcd_USB_DRD_FS;
extern uint16_t idx_rad;
extern uint16_t idx_temp;
extern UINT cur_record_num;

extern DeviceConfig device_config;

#define CRC8_SMBUS_POLYNOMIAL 0x07

/**
 * Calculates the standard CRC-8 checksum (Polynomial 0x07).
 * @param data Pointer to the data buffer.
 * @param length The number of bytes in the buffer.
 * @return The 8-bit CRC checksum.
 */
//FIXME: Create crc table for speed up.
uint8_t crc8_SMBUS_calculate(const uint8_t *data, size_t length) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x80) { // If MSB is 1
                crc = (crc << 1) ^ CRC8_SMBUS_POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

static UINT Rad_USBX_HID_ACK_response(const uint8_t cmd_id, const uint8_t parm_id, const uint8_t error_code) {
    uint8_t tx_buf[64] = {0};

    // 1. start
    tx_buf[0] = DEVICE_PACKET_START_0;
    tx_buf[1] = DEVICE_PACKET_START_1;
    // 2. cmd_id
    tx_buf[2] = DEVICE_CID_ACKNOWLEDGE_RESP;
    // 3. len
    tx_buf[3] = DEVICE_PACKET_ACKNOWLEDGE_LEN;
    // 4. data
    tx_buf[4] = cmd_id; // Command ID
    tx_buf[5] = parm_id; // Parameter ID
    tx_buf[6] = error_code; // Error code
    // 5. checksum
    tx_buf[7] = crc8_SMBUS_calculate(&tx_buf[4], DEVICE_PACKET_ACKNOWLEDGE_LEN);

    return USB_Send_HidReport(tx_buf, 64); // HID는 항상 64바이트
}

static void Rad_USBX_HID_Set_Command(const uint8_t parm_id);
static void Rad_USBX_HID_Get_Parameters(const uint8_t parm_id);
static void Rad_USBX_HID_Get_Records(const uint8_t parm_id);
static void Rad_USBX_HID_Set_Parameters(const uint8_t *data, const uint8_t len);

UINT RAD_USBX_Device_Init(void){
	UINT status  = 0;


	//Sometimes it fails to initiate device due to memory insufficient
	// add this flag to ensure initiate device successfully.
	status = MX_USBX_Device_Init();
	if (status != 0) {
//FIXME: add debug level.
#if DEBUG_LOG ==1
		printf("Failed: MX_USBX_Device_Init status %x\n",status);
#endif /* DEBUG_LOG */

		return status;
	}
	MX_USB_PCD_Init();

	// config endpoint 9 in,out, check theirs size
	HAL_PCDEx_PMAConfig(&hpcd_USB_DRD_FS, 0x00, PCD_SNG_BUF, 0x40);
	HAL_PCDEx_PMAConfig(&hpcd_USB_DRD_FS, 0x80, PCD_SNG_BUF, 0x80);

	// config endpoint 1 in,out, check theirs size
	HAL_PCDEx_PMAConfig(&hpcd_USB_DRD_FS, 0x01, PCD_SNG_BUF, 0xC0);
	HAL_PCDEx_PMAConfig(&hpcd_USB_DRD_FS, 0x81, PCD_SNG_BUF, 0x100);

	//enpoind 2 for MSC class.
	HAL_PCDEx_PMAConfig(&hpcd_USB_DRD_FS, 0x02, PCD_SNG_BUF, 0x140);
	HAL_PCDEx_PMAConfig(&hpcd_USB_DRD_FS, 0x82, PCD_SNG_BUF, 0x180);

	ux_dcd_stm32_initialize((ULONG)0, (ULONG)&hpcd_USB_DRD_FS);

	HAL_PCD_Start(&hpcd_USB_DRD_FS);

	return status;
}

void RAD_USBX_Device_Process(void){

	 ux_device_stack_tasks_run();

}

//FIXME: Should be use CRC8 standard polynomial 0x07
uint8_t DeviceSetting_CalcChecksum(const DeviceSettingPacket_t* pkt) {
    uint16_t sum = 0x82; // 규격상 0x82 + payload
    sum += pkt->cmd_id;
    sum += pkt->len;
    for (uint8_t i = 0; i < pkt->len; ++i)
        sum += pkt->data[i];
    return (uint8_t)(sum % 256);
}

int DeviceSetting_Send(const DeviceSettingPacket_t* pkt) {
    uint8_t tx_buf[64] = {0};
    size_t idx = 0;

    // 1. start
    tx_buf[idx++] = pkt->start[0];
    tx_buf[idx++] = pkt->start[1];
    // 2. cmd_id
    tx_buf[idx++] = pkt->cmd_id;
    // 3. parm_id
    tx_buf[idx++] = pkt->parm_id;
    // 4. len
    tx_buf[idx++] = pkt->len;
    // 5. data
    memcpy(&tx_buf[idx], pkt->data, pkt->len); idx += pkt->len;
    // 5. checksum
    tx_buf[idx++] = pkt->checksum;

    return USB_Send_HidReport(tx_buf, 64); // HID는 항상 64바이트
}

UINT USB_HID_Send_Record(const uint16_t record_idx)
{
    // Read entry
    log_entry_t e = {0};
    meas_data_log_read_entry(record_idx, &e);

    if (e.year == 0xFF) {
        printf("[USB] Record %u has invalid year (0xFF), stop sending.\r\n", record_idx);
        usbx_host_req_records = 0;  // 전송 종료 신호
        cur_record_num = 0;
        return UX_SUCCESS;
    }

    DeviceSettingPacket_t pkt = {0};
    pkt.start[0] = DEVICE_PACKET_START_0;
    pkt.start[1] = DEVICE_PACKET_START_1;
    pkt.cmd_id   = DEVICE_CID_GET_RECRORDS_RESP;
    pkt.parm_id  = 0x00; // No specific parameter ID for records
    pkt.len      = DEVICE_CID_GET_RECRORDS_RESP_LEN;

    // pkt.data[0] = e.index & 0xFF;
    // pkt.data[1] = (e.index >> 8) & 0xFF;

    // FIXME: For testing only:
    pkt.data[0] = record_idx & 0xFF;
    pkt.data[1] = (record_idx >> 8) & 0xFF;

    // [Timestamp (6 bytes)]
    pkt.data[2] = e.year;
    pkt.data[3] = e.month;
    pkt.data[4] = e.day;
    pkt.data[5] = e.hour;
    pkt.data[6] = e.minute;
    pkt.data[7] = e.second;
    // [Temp (2 bytes)] (LSB, MSB)
    pkt.data[8]  = e.temperature & 0xFF;
    pkt.data[9]  = (e.temperature >> 8) & 0xFF;
    // [CPS (2 bytes)] (LSB, MSB)
    pkt.data[10] = e.count & 0xFF;
    pkt.data[11] = (e.count >> 8) & 0xFF;
    // [Dose (4 bytes)] (LSB, ..., MSB)
    pkt.data[12] = e.dose & 0xFF;
    pkt.data[13] = (e.dose >> 8) & 0xFF;
    pkt.data[14] = (e.dose >> 16) & 0xFF;
    pkt.data[15] = (e.dose >> 24) & 0xFF;
    // [Mark (1 byte)]
    pkt.data[16] = e.mark;

    //FIXME: Change to CRC8_STANDARD func.
    pkt.checksum = crc8_SMBUS_calculate(pkt.data, pkt.len);

    return DeviceSetting_Send(&pkt);
}


void USB_HID_Receive(uint8_t* data, ULONG* len)
{
    uint8_t cmd_id =0;
    uint8_t parm_id = 0;
    uint8_t checksum = 0;
    uint8_t payload_len = 0;

    if (USB_Get_HidReport(data, len) == 1) {
        // No new data available
        return;
    }
    if (*len == 0)  {
        // It should never happen, but just in case.
        return;
    }

    if (data[0] != 0x55 || data[1] != 0x55) {
        printf("[USB] Invalid Start Byte: %02X %02X\n", data[0], data[1]);
        return;
    }

    cmd_id = data[2];
    parm_id = data[3];
    payload_len = data[4];
    checksum = data[5 + payload_len];
    // [4] 명령 해석 및 로그
    printf("[USB][RX] ");
    for (int i = 0; i < *len; i++)
        printf("%02X ", data[i]);
    printf("\n");

    printf("[USB][PARSE] cmd_id=0x%02X, len=%u, payload=0x%02X, checksum=0x%02X\n",
           cmd_id, payload_len, parm_id, checksum);

    // Check sum validation
//    int crc_check = crc8_SMBUS_calculate(&data[5], payload_len);
//    if (checksum != crc_check) {
//        LOG_APP("Checksum mismatch: expected %02X, got %02X\n", crc_check, checksum);
//        Rad_USBX_HID_ACK_response(cmd_id, parm_id, DEVICE_PACKET_ERROR_CRC_MISMATCH);
//        return;
//    }

    switch (cmd_id) {
    case DEVICE_CID_GET_PARAMETERS_REQ:
        Rad_USBX_HID_Get_Parameters(parm_id);
        break;
    case DEVICE_CID_GET_RECRORDS_REQ:
        Rad_USBX_HID_Get_Records(parm_id);
        break;
    case DEVICE_CID_SET_PARAMETERS_REQ:
        Rad_USBX_HID_Set_Parameters(data, *len);
        break;
    case DEVICE_CID_SET_COMMAND_REQ:
        Rad_USBX_HID_Set_Command(parm_id);
        break;

     case 0x05:
         uint8_t yy = data[4];
         uint8_t mm = data[5];
         uint8_t dd = data[6];
         uint8_t hh = data[7];
         uint8_t mi = data[8];
         uint8_t ss = data[9];
         Set_RTC_TimeOnly(yy, mm, dd, hh, mi, ss);
         printf("[CMD] Set RTC to 20%02u-%02u-%02u %02u:%02u:%02u\n", yy, mm, dd, hh, mi, ss);
         break;
     case 0x06:
    	 RAD_USBX_Clean_Vol();
//    	 RAD_USBX_Fatfs_format_disk();
    	 printf("Erase CSV & PDF");
         break;
//     case 0x07:
//    	 RTC_Disable_All_Wakeup();
//    	 printf("Reset RTC Alarm");
//         break;
     case 0x08:
    	 RTC_Disable_All_Wakeup();
    	 printf("Reset RTC Alarm");
         break;

    default:
        LOG_APP("[USB] Unknown command ID: 0x%02X\n", cmd_id);
        break;
    }

    memset(data, 0, *len);
    *len = 0;
}


void RAD_USBX_Fatfs_format_disk(void) {


  MKFS_PARM format_options = {
      .fmt = FM_FAT,      // Format type: FAT12/FAT16/FAT32
      .n_fat = MEDIA_NUMBER_OF_FATS,         // Number of FATs: 1 is fine for flash
      .align = 4,         // Data area alignment (Cluster size in sector unit.)
      .n_root = MEDIA_DIRECTORY_ENTRIES,      // Number of root directory entries
      .au_size = MEDIA_LOGICAL_CLUSTER_SIZE        // Cluster size (0=default, determined by volume size)
  };

  res = f_mount(&fs, "", 1); // The "1" forces mounting now

  if (res == FR_NO_FILESYSTEM) {
      LOG_USB_MSC("No filesystem found, formatting...\r\n");
      res = f_mkfs("", &format_options , work, sizeof(work));
      if (res != FR_OK) {
          LOG_USB_MSC("Error formatting drive! ret:%02x\r\n",res);
      }
  }
  res = f_mount(&fs, "", 1);
  if (res != FR_OK) {
        LOG_USB_MSC("Error mount drive! ret:%02x\r\n",res);
  }
  res = f_setlabel("0:RAD_TAG_LOG");
    if (res != FR_OK) {
        LOG_USB_MSC("Error setlabel volume! ret:%02x\r\n",res);
  }
}

void RAD_USBX_Clean_Vol(void) {
    int num_blk_64k = (NOR_FLASH_TOTAL_SIZE / NOR_FLASH_BLOCK_64K_SIZE);
    uint32_t addr = 0;

    for (int i = 0; i < num_blk_64k; i++)
    {
        addr = NOR_FLASH_BASE_ADDRESS + (i * NOR_FLASH_BLOCK_64K_SIZE);
        SPI_FLASH_EraseBlock64k(addr);
    }
}

static void Rad_USBX_HID_Set_Command(const uint8_t command_code) {

    switch (command_code) {
        case 0x01: // Start
            printf("[CMD] Start logging\n");
            Switch_Backup_reg(MODE_Start);
            break;
        case 0x02: // Stop
            printf("[CMD] Stop logging\n");
            Switch_Backup_reg(MODE_Stop);
            break;
        case 0x03: // Pause
            printf("[CMD] Pause\n");
            Switch_Backup_reg(MODE_Pause);
            break;
        case 0x04: // Resume
            printf("[CMD] Resume\n");
            Switch_Backup_reg(MODE_Start);
            break;
        case 0x05: // EraseData
            printf("[CMD] Erase Data\n");
            meas_data_log_erase();
            break;
        // Move to get records command id
        // case 0x06: // SendData
        //     printf("[CMD] Send All Data\n");
        //     //FIXME: Trigger a flag and all records will be sent at main loop
        //     // send_all_log_entries_usb();
        //     break;
        default:
            printf("[CMD] Unknown code: 0x%02X\n", command_code);
            Rad_USBX_HID_ACK_response(DEVICE_CID_SET_COMMAND_REQ, command_code, DEVICE_PACKET_PARAMETER_NOT_SUPPORTED);
            return;
    }
    Rad_USBX_HID_ACK_response(DEVICE_CID_SET_COMMAND_REQ, command_code, DEVICE_PACKET_SUCCESS);
}

// ---- helpers: epoch(2000-01-01 기준 초) -> YY/MM/DD hh:mm:ss
static inline int is_leap_y2000(uint16_t y2000) {
    uint16_t y = (uint16_t)(2000 + y2000);
    return ((y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0)));
}
static inline uint8_t days_in_month_y2000(uint8_t m, uint16_t y2000) {
    static const uint8_t dm[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 0 || m > 12) return 30;
    if (m == 2) return dm[1] + (is_leap_y2000(y2000) ? 1 : 0);
    return dm[m-1];
}
static void epoch_to_ymd_y2000(uint32_t epoch,
                               uint8_t *yy, uint8_t *mm, uint8_t *dd,
                               uint8_t *hh, uint8_t *mi, uint8_t *ss)
{
    uint32_t days = epoch / 86400U;
    uint32_t rem  = epoch % 86400U;

    *hh = (uint8_t)(rem / 3600U); rem %= 3600U;
    *mi = (uint8_t)(rem / 60U);
    *ss = (uint8_t)(rem % 60U);

    uint16_t y = 0; // 2000년 기준 오프셋
    for (;;) {
        uint16_t diy = (uint16_t)(is_leap_y2000(y) ? 366 : 365);
        if (days < diy) break;
        days -= diy; ++y;
    }
    *yy = (uint8_t)(y % 100);

    uint8_t m = 1;
    for (;;) {
        uint8_t dim = days_in_month_y2000(m, y);
        if (days < dim) break;
        days -= dim; ++m;
    }
    *mm = m;
    *dd = (uint8_t)(days + 1);
}


static void Rad_USBX_HID_Get_Parameters(const uint8_t parm_id) {
    DeviceSettingPacket_t response_pkt = {0};

    printf("[USB][GET_PARAM] parm_id=0x%02X\n", parm_id);

    switch (parm_id)
    {
    case DEVICE_PID_DEVICE_CODE:
        printf("[USB][GET_PARAM] DEVICE_CODE -> %s\n", current_settings.device_code);
        memcpy(response_pkt.data, current_settings.device_code, strlen((const char*)current_settings.device_code));
        response_pkt.len = strlen((const char*)current_settings.device_code);
        break;

    case DEVICE_PID_SERIAL_NUMBER:
        printf("[USB][GET_PARAM] SERIAL -> %s\n", current_settings.serial);
        memcpy(response_pkt.data, current_settings.serial, strlen((const char*)current_settings.serial));
        response_pkt.len = strlen((const char*)current_settings.serial);
        break;

    case DEVICE_PID_FIRMWARE_VER:
        printf("[USB][GET_PARAM] FW_VER -> %s\n", current_settings.firmware_ver);
        memcpy(response_pkt.data, current_settings.firmware_ver, strlen((const char*)current_settings.firmware_ver));
        response_pkt.len = strlen((const char*)current_settings.firmware_ver);
        break;

    case DEVICE_PID_RECORDING_TYPE:
        printf("[USB][GET_PARAM] RECORDING_TYPE -> %u\n", current_settings.sensor_type);
        response_pkt.data[0] = current_settings.sensor_type;
        response_pkt.len = 1;
        break;

    case DEVICE_PID_BATTERY_LEVEL:
        printf("[USB][GET_PARAM] BATTERY_LEVEL not supported\n");
        break;

    case DEVICE_PID_DEVICE_STATUS:
        printf("[USB][GET_PARAM] DEVICE_STATUS -> %u\n", current_settings.mode_status);
        response_pkt.data[0] = current_settings.mode_status;
        response_pkt.len = 1;
        break;

    case DEVICE_PID_TRIP_CODE:
        printf("[USB][GET_PARAM] TRIP_CODE -> %u\n", current_settings.trip_code);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.trip_code, response_pkt.data[0], response_pkt.data[1]);
        response_pkt.len = 2;
        break;

    case DEVICE_PID_TRIP_DESCRIPTION:
        printf("[USB][GET_PARAM] TRIP_DESCRIPTION -> %s\n", current_settings.trip_desc);
        memcpy(response_pkt.data, current_settings.trip_desc, strlen((const char*)current_settings.trip_desc));
        response_pkt.len = strlen((const char*)current_settings.trip_desc);
        break;

    case DEVICE_PID_START_MODE:
        printf("[USB][GET_PARAM] START_MODE -> %u\n", current_settings.start_mode);
        response_pkt.data[0] = current_settings.start_mode;
        response_pkt.len = 1;
        break;

    case DEVICE_PID_START_DELAY: {
        uint32_t v = (uint32_t)current_settings.start_delay;
        response_pkt.data[0] = (uint8_t)(v & 0xFF);
        response_pkt.data[1] = (uint8_t)((v >> 8) & 0xFF);
        response_pkt.data[2] = (uint8_t)((v >> 16) & 0xFF);
        response_pkt.data[3] = (uint8_t)((v >> 24) & 0xFF);
        response_pkt.len = 4;
        printf("[USB][GET_PARAM] START_DELAY -> %lu (len=4)\n", (unsigned long)v);
        break;
    }

    case DEVICE_PID_START_TIME: {
        uint32_t epoch = current_settings.start_reservation_time;

        uint8_t yy=0, mm=0, dd=0, hh=0, mi=0, ss=0;
        epoch_to_ymd_y2000(epoch, &yy, &mm, &dd, &hh, &mi, &ss);

        response_pkt.data[0] = yy; // 2000년 기준(00~99)
        response_pkt.data[1] = mm;
        response_pkt.data[2] = dd;
        response_pkt.data[3] = hh;
        response_pkt.data[4] = mi;
        response_pkt.data[5] = ss;
        response_pkt.len     = 6;

        printf("[USB][GET_PARAM] START_TIME -> 20%02u-%02u-%02u %02u:%02u:%02u (epoch=%lu)\n",
               yy, mm, dd, hh, mi, ss, (unsigned long)epoch);
        break;
    }


    case DEVICE_PID_PAUSE:
        printf("[USB][GET_PARAM] PAUSE -> %u\n", current_settings.pause_enable);
        response_pkt.data[0] = current_settings.pause_enable;
        response_pkt.len = 1;
        break;

    case DEVICE_PID_DEVICE_TIME: {
        RTC_TimeTypeDef sTime;
        RTC_DateTypeDef sDate;
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BCD);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BCD);
        response_pkt.data[0] = BCD2BIN(sDate.Year);
        response_pkt.data[1] = BCD2BIN(sDate.Month);
        response_pkt.data[2] = BCD2BIN(sDate.Date);
        response_pkt.data[3] = BCD2BIN(sTime.Hours);
        response_pkt.data[4] = BCD2BIN(sTime.Minutes);
        response_pkt.data[5] = BCD2BIN(sTime.Seconds);
        response_pkt.len = 6;
        printf("[USB][GET_PARAM] DEVICE_TIME -> 20%02u-%02u-%02u %02u:%02u:%02u\n",
               response_pkt.data[0], response_pkt.data[1], response_pkt.data[2],
               response_pkt.data[3], response_pkt.data[4], response_pkt.data[5]);
        break;
    }

    case DEVICE_PID_TEMP_LOGGING_INTERVAL: {
        // current_settings.temp_interval 가 아직 uint16_t 라면 상위 바이트는 0이 됩니다.
        uint32_t v = (uint32_t)current_settings.temp_interval;

        response_pkt.data[0] = (uint8_t)(v & 0xFF);
        response_pkt.data[1] = (uint8_t)((v >> 8) & 0xFF);
        response_pkt.data[2] = (uint8_t)((v >> 16) & 0xFF);
        response_pkt.data[3] = (uint8_t)((v >> 24) & 0xFF);
        response_pkt.len = 4;

        printf("[USB][GET_PARAM] TEMP_LOGGING_INTERVAL -> %lu sec (len=4)\n", (unsigned long)v);
        break;
    }


    case DEVICE_PID_RAD_LOGGING_INTERVAL: {
        uint32_t v = (uint32_t)current_settings.rad_interval; // 구조체가 u16이면 상위는 0
        response_pkt.data[0] = (uint8_t)(v & 0xFF);
        response_pkt.data[1] = (uint8_t)((v >> 8) & 0xFF);
        response_pkt.data[2] = (uint8_t)((v >> 16) & 0xFF);
        response_pkt.data[3] = (uint8_t)((v >> 24) & 0xFF);
        response_pkt.len = 4;
        printf("[USB][GET_PARAM] RAD_LOGGING_INTERVAL -> %lu sec (len=4)\n", (unsigned long)v);
        break;
    }

    // rad_usb.c
    case DEVICE_PID_LOGGING_DURATION: {
        uint16_t days = current_settings.interval_duration_day;  // 0이면 미설정
        response_pkt.data[0] = (uint8_t)(days & 0xFF);
        response_pkt.data[1] = (uint8_t)((days >> 8) & 0xFF);
        response_pkt.len = 2;
        printf("[USB][GET_PARAM] LOGGING_DURATION -> %u day(s)\n", days);
        break;
    }


    case DEVICE_PID_REPORT_FORMAT:
        printf("[USB][GET_PARAM] REPORT_FORMAT -> %u\n", current_settings.report_format);
        response_pkt.data[0] = current_settings.report_format;
        response_pkt.len = 1;
        break;

    case DEVICE_PID_TEMP_UNIT:
        printf("[USB][GET_PARAM] TEMP_UNIT -> %u\n", current_settings.display_temp_unit);
        response_pkt.data[0] = current_settings.display_temp_unit;
        response_pkt.len = 1;
        break;

    case DEVICE_PID_RAD_UNIT:
        printf("[USB][GET_PARAM] RAD_UNIT -> %u\n", current_settings.display_dose_unit);
        response_pkt.data[0] = current_settings.display_dose_unit;
        response_pkt.len = 1;
        break;

    case DEVICE_PID_RAD_HIGH_ALARM_1:
        printf("[USB][GET_PARAM] RAD_HIGH_ALARM_1 -> state=%u th=%u delay=%u\n",
               GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1),
               current_settings.alarm_rh1, current_settings.alarm_delay_rh1);
        response_pkt.data[0] = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_rh1, response_pkt.data[1], response_pkt.data[2]);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_delay_rh1, response_pkt.data[3], response_pkt.data[4]);
        response_pkt.len = 5;
        break;

    case DEVICE_PID_RAD_HIGH_ALARM_2:
        printf("[USB][GET_PARAM] RAD_HIGH_ALARM_2 -> state=%u th=%u delay=%u\n",
               GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2),
               current_settings.alarm_rh2, current_settings.alarm_delay_rh2);
        response_pkt.data[0] = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_rh2, response_pkt.data[1], response_pkt.data[2]);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_delay_rh2, response_pkt.data[3], response_pkt.data[4]);
        response_pkt.len = 5;
        break;

    case DEVICE_PID_TEMP_HIGH_ALARM_1:
        printf("[USB][GET_PARAM] TEMP_HIGH_ALARM_1 -> state=%u th=%d delay=%u\n",
               GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1),
               current_settings.alarm_th1, current_settings.alarm_delay_th1);
        response_pkt.data[0] = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_th1, response_pkt.data[1], response_pkt.data[2]);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_delay_th1, response_pkt.data[3], response_pkt.data[4]);
        response_pkt.len = 5;
        break;

    case DEVICE_PID_TEMP_HIGH_ALARM_2:
        printf("[USB][GET_PARAM] TEMP_HIGH_ALARM_2 -> state=%u th=%d delay=%u\n",
               GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2),
               current_settings.alarm_th2, current_settings.alarm_delay_th2);
        response_pkt.data[0] = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_th2, response_pkt.data[1], response_pkt.data[2]);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_delay_th2, response_pkt.data[3], response_pkt.data[4]);
        response_pkt.len = 5;
        break;

    case DEVICE_PID_TEMP_LOW_ALARM_1:
        printf("[USB][GET_PARAM] TEMP_LOW_ALARM_1 -> state=%u th=%d delay=%u\n",
               GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1),
               current_settings.alarm_tl1, current_settings.alarm_delay_tl1);
        response_pkt.data[0] = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_tl1, response_pkt.data[1], response_pkt.data[2]);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_delay_tl1, response_pkt.data[3], response_pkt.data[4]);
        response_pkt.len = 5;
        break;

    case DEVICE_PID_TEMP_LOW_ALARM_2:
        printf("[USB][GET_PARAM] TEMP_LOW_ALARM_2 -> state=%u th=%d delay=%u\n",
               GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2),
               current_settings.alarm_tl2, current_settings.alarm_delay_tl2);
        response_pkt.data[0] = GET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_tl2, response_pkt.data[1], response_pkt.data[2]);
        U16_TO_LITTLE_ENDIAN_BYTES(current_settings.alarm_delay_tl2, response_pkt.data[3], response_pkt.data[4]);
        response_pkt.len = 5;
        break;

    case DEVICE_PID_CURRENT_INDEX_READINGS:
        printf("[USB][GET_PARAM] CURRENT_INDEX_READINGS -> temp=%u rad=%u\n", idx_temp, idx_rad);
        U16_TO_LITTLE_ENDIAN_BYTES(idx_temp, response_pkt.data[0], response_pkt.data[1]);
        U16_TO_LITTLE_ENDIAN_BYTES(idx_rad, response_pkt.data[2], response_pkt.data[3]);
        response_pkt.len = 4;
        break;

    default:
        printf("[USB][GET_PARAM] Unknown parameter ID: 0x%02X\n", parm_id);
        Rad_USBX_HID_ACK_response(DEVICE_CID_GET_PARAMETERS_REQ, parm_id, DEVICE_PACKET_PARAMETER_NOT_SUPPORTED);
        return;
    }

    response_pkt.start[0] = DEVICE_PACKET_START_0;
    response_pkt.start[1] = DEVICE_PACKET_START_1;
    response_pkt.cmd_id   = DEVICE_CID_GET_PARAMETERS_RESP;
    response_pkt.parm_id  = parm_id;
    response_pkt.checksum = crc8_SMBUS_calculate(response_pkt.data, response_pkt.len);

    printf("[USB][GET_PARAM] Sending response (len=%u, checksum=0x%02X)\n",
           response_pkt.len, response_pkt.checksum);

    DeviceSetting_Send(&response_pkt);
}


static void Rad_USBX_HID_Set_Parameters(const uint8_t *data, const uint8_t len) {

    uint8_t parm_id = data[3];
    uint8_t payload_len = data[4];
    uint8_t *payload = (uint8_t *)&data[5];

    printf("[USB][SET_PARAM] parm_id=0x%02X, payload_len=%u\n", parm_id, payload_len);

    switch (parm_id)
    {
    case DEVICE_PID_DEVICE_CODE:
        memset(current_settings.device_code, 0, sizeof(current_settings.device_code));
        memcpy(current_settings.device_code, payload, payload_len);
        printf("[USB][SET_PARAM] DEVICE_CODE <- %.*s\n", payload_len, payload);
        break;

    case DEVICE_PID_SERIAL_NUMBER:
        memset(current_settings.serial, 0, sizeof(current_settings.serial));
        memcpy(current_settings.serial, payload, payload_len);
        printf("[USB][SET_PARAM] SERIAL <- %.*s\n", payload_len, payload);
        break;

    case DEVICE_PID_FIRMWARE_VER:
        memset(current_settings.firmware_ver, 0, sizeof(current_settings.firmware_ver));
        memcpy(current_settings.firmware_ver, payload, payload_len);
        printf("[USB][SET_PARAM] FIRMWARE_VER <- %.*s\n", payload_len, payload);
        break;

    case DEVICE_PID_RECORDING_TYPE:
        current_settings.sensor_type = payload[0];
        printf("[USB][SET_PARAM] RECORDING_TYPE <- %u\n", current_settings.sensor_type);
        break;

    case DEVICE_PID_BATTERY_LEVEL:
        printf("[USB][SET_PARAM] BATTERY_LEVEL not supported\n");
        break;

    case DEVICE_PID_TRIP_CODE:
        current_settings.trip_code = LITTLE_ENDIAN_BYTES_TO_U16(payload[0], payload[1]);
        printf("[USB][SET_PARAM] TRIP_CODE <- %u\n", current_settings.trip_code);
        break;

    case DEVICE_PID_TRIP_DESCRIPTION:
        memset(current_settings.trip_desc, 0, sizeof(current_settings.trip_desc));
        memcpy(current_settings.trip_desc, payload, payload_len);
        printf("[USB][SET_PARAM] TRIP_DESCRIPTION <- %.*s\n", payload_len, payload);
        break;

    case DEVICE_PID_START_MODE:
        current_settings.start_mode = payload[0];
        printf("[USB][SET_PARAM] START_MODE <- %u\n", current_settings.start_mode);
        break;

    case DEVICE_PID_LOGGING_DURATION:
        current_settings.interval_duration_day = LITTLE_ENDIAN_BYTES_TO_U16(payload[0], payload[1]);
        Save_CurrentSettings();
        printf("[USB][SET_PARAM] LOGGING_DURATION <- %u day(s)\n", current_settings.interval_duration_day);
        break;

    case DEVICE_PID_START_DELAY: {
        uint32_t v32 = ((uint32_t)payload[0])
                     | (((uint32_t)payload[1]) << 8)
                     | (((uint32_t)payload[2]) << 16)
                     | (((uint32_t)payload[3]) << 24);
        current_settings.start_delay = v32;
//        Save_CurrentSettings();  // 저장까지 수행
        printf("[USB][SET_PARAM] START_DELAY <- %lu (len=4)\n", (unsigned long)v32);
        break;
    }


    case DEVICE_PID_START_TIME: {
        uint8_t yy = payload[0];  // 2000년 기준
        uint8_t mm = payload[1];
        uint8_t dd = payload[2];
        uint8_t hh = payload[3];
        uint8_t mi = payload[4];
        uint8_t ss = payload[5];

        uint32_t epoch = ymd_to_epoch(yy, mm, dd, hh, mi, ss);
        current_settings.start_reservation_time = epoch;

        printf("[USB][SET_PARAM] START_TIME <- 20%02u-%02u-%02u %02u:%02u:%02u (epoch=%lu)\n",
               yy, mm, dd, hh, mi, ss, (unsigned long)epoch);
        break;
    }


    case DEVICE_PID_PAUSE:
        current_settings.pause_enable = payload[0];
        printf("[USB][SET_PARAM] PAUSE <- %u\n", current_settings.pause_enable);
        break;

    case DEVICE_PID_DEVICE_TIME: {
        uint8_t yy = payload[0];
        uint8_t mm = payload[1];
        uint8_t dd = payload[2];
        uint8_t hh = payload[3];
        uint8_t mi = payload[4];
        uint8_t ss = payload[5];
        Set_RTC_TimeOnly(yy, mm, dd, hh, mi, ss);
        printf("[USB][SET_PARAM] DEVICE_TIME <- 20%02u-%02u-%02u %02u:%02u:%02u\n",
               yy, mm, dd, hh, mi, ss);
        break;
    }

    case DEVICE_PID_TEMP_LOGGING_INTERVAL: {
        uint32_t v32 = 0;

        if (payload_len == 4) {
            v32  =  ((uint32_t)payload[0])
                 | (((uint32_t)payload[1]) << 8)
                 | (((uint32_t)payload[2]) << 16)
                 | (((uint32_t)payload[3]) << 24);
        } else if (payload_len == 2) {
            // 하위 호환 (구버전: 16-bit 초)
            v32 = LITTLE_ENDIAN_BYTES_TO_U16(payload[0], payload[1]);
        } else {
            // 길이 오류 → 거부
            Rad_USBX_HID_ACK_response(DEVICE_CID_SET_PARAMETERS_REQ, parm_id,
                                      DEVICE_PACKET_PARAMETER_NOT_SUPPORTED);
            printf("[USB][SET_PARAM][ERR] TEMP_LOGGING_INTERVAL invalid len=%u\n", payload_len);
            return;
        }

        // (구조체 필드 타입) 현재 프로젝트 구조에 맞게 할당
        // 만약 아직 uint16_t 라면, 임시로 clamp 하거나(권장X) 구조체를 uint32_t로 변경하세요.
        current_settings.temp_interval = (typeof(current_settings.temp_interval))v32;

        printf("[USB][SET_PARAM] TEMP_LOGGING_INTERVAL <- %lu sec (len=%u)\n",
               (unsigned long)v32, payload_len);
        break;
    }

    case DEVICE_PID_RAD_LOGGING_INTERVAL: {
        uint32_t v32 = 0;

        if (payload_len == 4) {
            v32  =  ((uint32_t)payload[0])
                 | (((uint32_t)payload[1]) << 8)
                 | (((uint32_t)payload[2]) << 16)
                 | (((uint32_t)payload[3]) << 24);
        } else if (payload_len == 2) {
            v32 = LITTLE_ENDIAN_BYTES_TO_U16(payload[0], payload[1]); // 하위호환
        } else {
            Rad_USBX_HID_ACK_response(DEVICE_CID_SET_PARAMETERS_REQ, parm_id,
                                      DEVICE_PACKET_PARAMETER_NOT_SUPPORTED);
            printf("[USB][SET_PARAM][ERR] RAD_LOGGING_INTERVAL invalid len=%u\n", payload_len);
            return;
        }

        current_settings.rad_interval = (typeof(current_settings.rad_interval))v32;

        printf("[USB][SET_PARAM] RAD_LOGGING_INTERVAL <- %lu sec (len=%u)\n",
               (unsigned long)v32, payload_len);
        break;
    }

//    case DEVICE_PID_LOGGING_DURATION:
//        printf("[USB][SET_PARAM] LOGGING_DURATION not supported\n");
//        break;

    case DEVICE_PID_REPORT_FORMAT:
        current_settings.report_format = payload[0];
        printf("[USB][SET_PARAM] REPORT_FORMAT <- %u\n", current_settings.report_format);
        break;

    case DEVICE_PID_TEMP_UNIT:
        current_settings.display_temp_unit = payload[0];
        printf("[USB][SET_PARAM] TEMP_UNIT <- %u\n", current_settings.display_temp_unit);
        break;

    case DEVICE_PID_RAD_UNIT:
        current_settings.display_dose_unit = payload[0];
        printf("[USB][SET_PARAM] RAD_UNIT <- %u\n", current_settings.display_dose_unit);
        break;

    case DEVICE_PID_RAD_HIGH_ALARM_1:
        SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH1, payload[0]);
        current_settings.alarm_rh1 = LITTLE_ENDIAN_BYTES_TO_U16(payload[1], payload[2]);
        current_settings.alarm_delay_rh1 = LITTLE_ENDIAN_BYTES_TO_U16(payload[3], payload[4]);
        printf("[USB][SET_PARAM] RAD_HIGH_ALARM_1 <- state=%u th=%u delay=%u\n",
               payload[0], current_settings.alarm_rh1, current_settings.alarm_delay_rh1);
        break;

    case DEVICE_PID_RAD_HIGH_ALARM_2:
        SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_RH2, payload[0]);
        current_settings.alarm_rh2 = LITTLE_ENDIAN_BYTES_TO_U16(payload[1], payload[2]);
        current_settings.alarm_delay_rh2 = LITTLE_ENDIAN_BYTES_TO_U16(payload[3], payload[4]);
        printf("[USB][SET_PARAM] RAD_HIGH_ALARM_2 <- state=%u th=%u delay=%u\n",
               payload[0], current_settings.alarm_rh2, current_settings.alarm_delay_rh2);
        break;

    case DEVICE_PID_TEMP_HIGH_ALARM_1:
        SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH1, payload[0]);
        current_settings.alarm_th1 = LITTLE_ENDIAN_BYTES_TO_I16(payload[1], payload[2]);
        current_settings.alarm_delay_th1 = LITTLE_ENDIAN_BYTES_TO_U16(payload[3],payload[4]);
        printf("[USB][SET_PARAM] TEMP_HIGH_ALARM_1 <- state=%u th=%d delay=%u\n",
               payload[0], current_settings.alarm_th1, current_settings.alarm_delay_th1);
        break;

    case DEVICE_PID_TEMP_HIGH_ALARM_2:
        SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TH2, payload[0]);
        current_settings.alarm_th2 = LITTLE_ENDIAN_BYTES_TO_I16(payload[1], payload[2]);
        current_settings.alarm_delay_th2 = LITTLE_ENDIAN_BYTES_TO_U16(payload[3],payload[4]);
        printf("[USB][SET_PARAM] TEMP_HIGH_ALARM_2 <- state=%u th=%d delay=%u\n",
               payload[0], current_settings.alarm_th2, current_settings.alarm_delay_th2);
        break;

    case DEVICE_PID_TEMP_LOW_ALARM_1:
        SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL1, payload[0]);
        current_settings.alarm_tl1 = LITTLE_ENDIAN_BYTES_TO_I16(payload[1], payload[2]);
        current_settings.alarm_delay_tl1 = LITTLE_ENDIAN_BYTES_TO_U16(payload[3], payload[4]);
        printf("[USB][SET_PARAM] TEMP_LOW_ALARM_1 <- state=%u th=%d delay=%u\n",
               payload[0], current_settings.alarm_tl1, current_settings.alarm_delay_tl1);
        break;

    case DEVICE_PID_TEMP_LOW_ALARM_2:
        SET_ALARM_STATE(device_config.alarm_state, ALARM_STATE_POS_TL2, payload[0]);
        current_settings.alarm_tl2 = LITTLE_ENDIAN_BYTES_TO_I16(payload[1], payload[2]);
        current_settings.alarm_delay_tl2 = LITTLE_ENDIAN_BYTES_TO_U16(payload[3], payload[4]);
        printf("[USB][SET_PARAM] TEMP_LOW_ALARM_2 <- state=%u th=%d delay=%u\n",
               payload[0], current_settings.alarm_tl2, current_settings.alarm_delay_tl2);
        break;

    default:
        printf("[USB][SET_PARAM] Unknown parameter ID: 0x%02X\n", parm_id);
        Rad_USBX_HID_ACK_response(DEVICE_CID_SET_PARAMETERS_REQ, parm_id, DEVICE_PACKET_PARAMETER_NOT_SUPPORTED);
        return;
    }

    // 성공 ACK
    Rad_USBX_HID_ACK_response(DEVICE_CID_SET_PARAMETERS_REQ, parm_id, DEVICE_PACKET_SUCCESS);
    printf("[USB][SET_PARAM] ACK sent for parm_id=0x%02X\n", parm_id);
}

static void Rad_USBX_HID_Get_Records(const uint8_t parm_id)
{
    UNUSED(parm_id);
    usbx_host_req_records = 1; // Set flag to send all records at main loop
}


FRESULT RAD_Fatfs_MountOnly(void)
{
    // 안전하게 언마운트 후 재마운트 (선택)
    f_mount(0, "", 0);

    FRESULT r = f_mount(&fs, "", 1);  // 즉시 마운트
    if (r == FR_OK) {
        printf("[FATFS] remount OK\r\n");
    } else if (r == FR_NO_FILESYSTEM) {
        printf("[FATFS] remount FAIL: no filesystem (FR_NO_FILESYSTEM)\r\n");
        // 필요 시 포맷:
        // RAD_USBX_Fatfs_format_disk();
    } else {
        printf("[FATFS] remount FAIL: %d\r\n", r);
    }
    return r;
}

