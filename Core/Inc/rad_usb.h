/*
 * rad_usb.h
 *
 *  Created on: Jun 18, 2025
 *      Author: dongo
 */

#ifndef INC_RAD_USB_H_
#define INC_RAD_USB_H_
#include <stdint.h>
#include <stdio.h>

#include "ux_port.h"
#include "ux_api.h"

#define USB_HID_SEND_RECORDS_SPEED_MS           3 //ms

#define DEVICE_PACKET_START_0   'U'
#define DEVICE_PACKET_START_1   'U'
#define DEVICE_PACKET_END_0     'Z'
#define DEVICE_PACKET_END_1     'Z'
#define DEVICE_PACKET_MAX_DATA  57

// Device Command IDs
#define DEVICE_CID_GET_PARAMETERS_REQ           0x01
#define DEVICE_CID_GET_RECRORDS_REQ             0x02
#define DEVICE_CID_SET_PARAMETERS_REQ           0x03
#define DEVICE_CID_SET_COMMAND_REQ              0x04

//Response Command IDs
#define DEVICE_CID_GET_PARAMETERS_RESP          0x11
#define DEVICE_CID_GET_RECRORDS_RESP            0x12
// records response length: idx:2 bytes, time: 6bytes,
// temperature: 2bytes, cps: 2bytes, dose: 4bytes, mark: 1byte
#define DEVICE_CID_GET_RECRORDS_RESP_LEN       17


#define DEVICE_CID_ACKNOWLEDGE_RESP             0xFE
// 3bytes: CID, PID and ERROR codes
#define DEVICE_PACKET_ACKNOWLEDGE_LEN           3


// Device Parameter IDs
#define DEVICE_PID_DEVICE_CODE                  0x01
#define DEVICE_PID_SERIAL_NUMBER                0x02
#define DEVICE_PID_FIRMWARE_VER                 0x03
#define DEVICE_PID_RECORDING_TYPE               0x04
#define DEVICE_PID_BATTERY_LEVEL                0x05
#define DEVICE_PID_DEVICE_STATUS                0x06
#define DEVICE_PID_TRIP_CODE                    0x07
#define DEVICE_PID_TRIP_DESCRIPTION             0x08
#define DEVICE_PID_START_MODE                   0x09
#define DEVICE_PID_START_DELAY                  0x0A
#define DEVICE_PID_START_TIME                   0x0B
#define DEVICE_PID_PAUSE                        0x0C
#define DEVICE_PID_DEVICE_TIME                  0x0D
#define DEVICE_PID_TEMP_LOGGING_INTERVAL        0x0E
#define DEVICE_PID_RAD_LOGGING_INTERVAL         0x0F

#define DEVICE_PID_REPORT_FORMAT                0x10
#define DEVICE_PID_TEMP_UNIT                    0x11
#define DEVICE_PID_RAD_UNIT                     0x12
#define DEVICE_PID_RAD_HIGH_ALARM_1             0x13
#define DEVICE_PID_RAD_HIGH_ALARM_2             0x14
#define DEVICE_PID_TEMP_HIGH_ALARM_1            0x15
#define DEVICE_PID_TEMP_HIGH_ALARM_2            0x16
#define DEVICE_PID_TEMP_LOW_ALARM_1             0x17
#define DEVICE_PID_TEMP_LOW_ALARM_2             0x18
#define DEVICE_PID_CURRENT_INDEX_READINGS       0x19
#define DEVICE_PID_LOGGING_DURATION             0x1A
//#define DEVICE_PID_LOGGING_DURATION             0x20

// Device packet error codes
#define DEVICE_PACKET_SUCCESS                   0x00
#define DEVICE_PACKET_ERROR_SET                 0x01

#define DEVICE_PACKET_PARAMETER_NOT_SUPPORTED   0xFE
#define DEVICE_PACKET_ERROR_CRC_MISMATCH        0xFF


/**
 * @brief Converts a 16-bit value into two 8-bit bytes in little-endian order.
 * Little-endian means the Least Significant Byte (LSB) comes first.
 *
 * @param u16_val The uint16_t value to convert.
 * @param byte0   The uint8_t variable to store the LSB.
 * @param byte1   The uint8_t variable to store the MSB.
 */
#define U16_TO_LITTLE_ENDIAN_BYTES(u16_val, byte0, byte1) \
    do {                                                 \
        (byte0) = (uint8_t)((u16_val) & 0xFF);            \
        (byte1) = (uint8_t)(((u16_val) >> 8) & 0xFF);     \
    } while (0)

/**
 * @brief Converts two 8-bit bytes (little-endian) into a single uint16_t.
 *
 * @param byte0 The LSB (Least Significant Byte).
 * @param byte1 The MSB (Most Significant Byte).
 * @return The resulting uint16_t value.
 */
#define LITTLE_ENDIAN_BYTES_TO_U16(byte0, byte1) \
    ((uint16_t)(((uint16_t)(byte1) << 8) | (byte0)))

#define LITTLE_ENDIAN_BYTES_TO_I16(byte0, byte1) \
    ((int16_t)(((uint16_t)(byte1) << 8) | (byte0)))


typedef struct {
    uint8_t start[2];                 // "UU"
    uint8_t cmd_id;
    uint8_t parm_id;
    uint8_t len;                       // data 길이
    uint8_t data[DEVICE_PACKET_MAX_DATA];
    uint8_t checksum;
} DeviceSettingPacket_t;


uint8_t DeviceSetting_CalcChecksum(const DeviceSettingPacket_t* pkt);
int DeviceSetting_Send(const DeviceSettingPacket_t* pkt);
int DeviceSetting_Receive(DeviceSettingPacket_t* pkt);
void USB_HID_Receive(uint8_t* data, ULONG* len);

/* Read entry at record_idx and send it.*/
//FIXME: HID MSP is 64 bytes, but record is 17 bytes. So Should append 2 more records?
UINT USB_HID_Send_Record(const uint16_t record_idx);

UINT RAD_USBX_Device_Init(void);
void RAD_USBX_Device_Process(void);

void RAD_USBX_Fatfs_format_disk(void);
void RAD_USBX_Clean_Vol(void);

#endif /* INC_RAD_USB_H_ */
