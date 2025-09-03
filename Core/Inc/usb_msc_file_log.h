/*
 * usb_msc_file_log.h
 *
 *  Created on: Jul 10, 2025
 *      Author: dongo
 */

#ifndef INC_USB_MSC_FILE_LOG_H_
#define INC_USB_MSC_FILE_LOG_H_


#include "main.h"
#include "ff.h"
#include "sensor_log.h"

typedef struct csv_log_times {
    uint8_t  year;         //  1B: 2000년 기준 오프셋 (예: 25→2025)
    uint8_t  month;        //  1B: 1~12
    uint8_t  day;          //  1B: 1~31
    uint8_t  hour;         //  1B: 0~23
    uint8_t  minute;       //  1B: 0~59
    uint8_t  second;       //  1B: 0~59
}file_log_time_t;

typedef struct {
    float         highest_temp;
    float         lowest_temp;
    float         average_temp;
    float         highest_radiation;
    float         average_radiation;
    float         mean_kinetic_temp; // MKT
    file_log_time_t          start_time;
    file_log_time_t          stop_time;
    unsigned long elapsed_time_sec;
    unsigned int  data_points_temp_count; // Count of temperature data points
    unsigned int  data_points_radiation_count; // Count of radiation data points
    uint16_t  RH1_alarm_count;
    uint16_t  RH2_alarm_count;
    uint16_t  TH1_alarm_count;
    uint16_t  TH2_alarm_count;
    uint16_t  TL1_alarm_count;
    uint16_t  TL2_alarm_count;

} logging_summary_t;

/* Exported types ------------------------------------------------------------*/
// An enum for the status of an alarm zone
typedef enum {
    ALARM_STATUS_OK = 0,          // Zone is within normal limits
    ALARM_STATUS_ALARM
} AlarmStatus;

// A struct to hold information for a single alarm zone
typedef struct {
    char        zone_name[16];                   // e.g., "H3"
    int16_t       threshold;          // e.g., 25.0
    unsigned long alarm_delay_sec;          // e.g., 00D 00H 30M 00S -> 1800
    char        delay_type[8];                  // e.g., "(Sin)"
    unsigned long total_time_in_violation_minutes; // e.g., 30D 16H 36M 00S -> 2651760
    unsigned int  violation_count;              // e.g., 31
    AlarmStatus   status;                       // e.g., ALARM_STATUS_ALARM
} alarm_zone_info_t;

// A struct for a single marked event
typedef struct {
    int         index;
    char      timestamp[32];
    float       temperature;
} MarkedEventInfo;

// The main struct to hold all parsed log information
typedef struct {
    // Device Information
    struct {
        const char *device_code;
        const char *probe_type;
        const char *firmware_version;
        const char *model_code;
        const char *serial_number;
    } device_info;

    // Trip Information
    struct {
        unsigned long trip_id;
        const char    *description;
    } trip_info;

    // Configuration Information
    struct {
        unsigned long temp_interval_sec;
        unsigned long rad_interval_sec;
        const char          *start_mode;
        const char          *stop_mode;
        unsigned long start_delay_sec;
    } config_info;

    // Alarm Configuration/Summary
    struct {
        file_log_time_t   alarm_triggered_at;
        // Array to hold all 6 alarm zones from the log
        alarm_zone_info_t zones[6];
    } alarm_summary;

    // Logging Summary
    logging_summary_t logging_summary;
    // Marked Events
    // A pointer is used here since the number of marked events can vary.
    MarkedEventInfo* marked_events;
    int              num_marked_events;

    // Make a utility struct to hold offsets for CSV fields
    // This will help in writing the CSV file at the fixed offsets
    struct
    {
        unsigned int device_info_offset;
        unsigned int trip_info_offset;
        unsigned int config_info_offset;
        unsigned int alarm_summary_offset;
        unsigned int alarm_summary_fill_offset;
        unsigned int logging_summary_offset;
        unsigned int marked_events_offset;
        unsigned int logging_summary_avg_offset;

    }csv_fiels_offset;

    // The actual recorded data points would also be dynamically allocated
    // DataPoint* recorded_data;
    // int             num_data_points; // Should match logging_summary.data_points_count

} log_csv_t;

/*---------------------------------------------------------------------------*/


/* Exported define ------------------------------------------------------------*/

#define MAX_NUMBER_RECORDS 12000

#define DEVICE_INFO_DEVICE_CODE "LogNC 1"
#define DEVICE_INFO_PROBE_TYPE "Temperature and Radiation (interval)"

#define DEVICE_TEMPERATURE_UINIT    "C"
#define DEVICE_RADIATION_UINIT      "uSv/h"

#define TRIP_DESCRIPTION "Temperature and Radiation recording"


#define ALARM_ZONE_TH2_THRESHOLD 25.0f
#define ALARM_ZONE_TH1_THRESHOLD 15.0f
#define ALARM_ZONE_TL1_THRESHOLD 0.0f
#define ALARM_ZONE_TL2_THRESHOLD 2.0f

#define ALARM_ZONE_RH1_THRESHOLD 550.0f

/* Export variables ---------------------------------------------------------*/



void minutes_to_hms_string(uint16_t total_mins, char *buffer, size_t buffer_size);
void time_to_dmyhms_string(const file_log_time_t *time, char *buffer, size_t buffer_size);
const char* convert_month_to_string(const uint8_t  month);

/**
 * @brief Converts total minutes to a "DDH HH M SSS" string.
 * @param total_mins The total time in minutes.
 * @param buffer A pointer to the output buffer.
 * @param buffer_size The size of the output buffer.
 */
void minutes_to_dhms_string_long(uint32_t total_mins, char *buffer, size_t buffer_size);


/*-----------------------------------CSV-------------------------------------*/
UINT csv_gen_template_file(const DeviceSettings *dev_setting);

void csv_append_new_record(const file_log_time_t *time,
                           float temperature,
                           float radiation);
void csv_gen_completed_report(const logging_summary_t *log);

const logging_summary_t* csv_get_logging_summary(void);

/*-----------------------------------PDF-------------------------------------*/
// This function only generates a raw PDF file.
// This must be called at the beginning of the
// This file can't not be open by pc due to it has wrong pdf format.
UINT pdf_gen_template_file(void);

// This function corrects the PDF format by updating kids, xref, and drawing the chart.
// This is only called when stopping measurement (e.g., connect to USB, press stop
void pdf_gen_completed_report(const logging_summary_t *log,
                            const DeviceSettings *dev_setting);

// Insert data point to table
void pdf_append_new_record(const UINT record_num,
                            const file_log_time_t *time,
                           float temperature,
                           float radiation);

#endif /* INC_USB_MSC_FILE_LOG_H_ */

