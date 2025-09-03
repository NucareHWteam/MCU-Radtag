/*
 * usb_msc_file_log.c
 *
 *  Created on: Jul 10, 2025
 *      Author: dongo
 */


/*
 * usb_msc_csv_log.c
 *
 *  Created on: Jul 2, 2025
 *      Author: dongo
 */
#include "sensor_log.h"
#include "usb_msc_file_log.h"
#include "meas_data_log.h"
#include "log_system.h"
#include "ff.h"
#include <stdbool.h>
extern DeviceConfig device_config;
/* Private macro -------------------------------------------------------------*/

// Muximum number of records in the log file
#define MAX_NUMBER_RECORDS 12000
// FIXME: should be set to the real size of the csv file?
#define CSV_PRE_ALLOCATE_FILE_SIZE (650*1024) //650KB for csv log.

#define CSV_LOG_FILE_NAME "Data_Log.csv"

// Time format for CSV file
#define CSV_YEAR_OFFSET 2000 // Year offset for CSV file, e.g., 25 means 2025

// Don't change these defines
// Fix offset for each section in the CSV file
#define CSV_DEVICE_INFO_OFFSET                      0xff
#define CSV_TRIP_INFO_OFFSET                        0xff
#define CSV_CONFIG_INFO_OFFSET                      546
#define CSV_ALARM_SUMMARY_OFFSET                    725
#define CSV_LOGGING_SUMMARY_OFFSET                  1498

#define fatfs_write_str(file_ptr,text)      f_puts(text,file_ptr)

#define ALARM_SUMMARY_PAD_BYTES 1024   // 권장



static log_csv_t log_csv = {
    //FIXME: should be fill with real device info
    .device_info = {
        .device_code = DEVICE_INFO_DEVICE_CODE,
        .probe_type = DEVICE_INFO_PROBE_TYPE,
        .firmware_version = " ",
        .model_code = " ",
        .serial_number = " "
    },

    .trip_info = {
        .trip_id = 0,
        .description = TRIP_DESCRIPTION
    },

    .config_info = {
        //FIXME: should be replaced by real value seting from host.
        .temp_interval_sec = 0,             //10mins
		.rad_interval_sec = 0,             //10mins
        .start_mode = "Manual",
        .stop_mode = "Manual & Software",
        .start_delay_sec = 0
    },

    /*Alarm Configuration/Summary
    / ********************************/
    .alarm_summary = {
        .alarm_triggered_at = {
            .year = 0, //
            .month = 1, //
            .day = 0,  //
            .hour = 0,  //
            .minute = 0,
            .second = 0
        },
        .zones[0] = {
            .zone_name = "RH1",
            .threshold = 0,
            .alarm_delay_sec = 0,
            .total_time_in_violation_minutes = 0,
            .violation_count = 0,
            .status = ALARM_STATUS_ALARM
        },
        .zones[1] = {
            .zone_name = "RH2",
            .threshold = 0,
            .alarm_delay_sec = 0,
            .total_time_in_violation_minutes = 0,
            .violation_count = 0,
            .status = ALARM_STATUS_ALARM
        },
        .zones[2] = {
            .zone_name = "TH1",
            .threshold = 0,
            .alarm_delay_sec = 0,
            .total_time_in_violation_minutes = 0,
            .violation_count = 0,
            .status = ALARM_STATUS_ALARM
        },
        .zones[3] = {
            .zone_name = "TH2",
            .threshold = 0,
            .alarm_delay_sec = 0,
            .total_time_in_violation_minutes = 0,
            .violation_count = 0,
            .status = ALARM_STATUS_ALARM
        },
        .zones[4] = {
            .zone_name = "TL1",
            .threshold = 0,
            .alarm_delay_sec = 0,
            .total_time_in_violation_minutes = 0,
            .violation_count = 0,
            .status = ALARM_STATUS_OK
        },
        .zones[5] = {
            .zone_name = "TL2",
            .threshold =0,
            .alarm_delay_sec = 0,
            .total_time_in_violation_minutes = 0,
            .violation_count = 0,
            .status = ALARM_STATUS_OK
        }
    },
    // Logging Summary
    // *******************************
    .logging_summary  = {
        .highest_temp = 0.0,
        .lowest_temp = 0.0,
        .average_temp = 0.0,
        .mean_kinetic_temp = 0.0,
        .highest_radiation = 0,
        .average_radiation = 0,
        .start_time  = {
            .year = 0,
            .month = 0,
            .day = 0,
            .hour = 0,
            .minute = 0,
            .second = 0
        },
        .stop_time   = {0},
        .elapsed_time_sec = 0,
        .data_points_temp_count = 0,
		.data_points_radiation_count =0
    },
    /*Marked Events
     ********************************/
//    .marked_events = {
//        .index =0,
//        .timestamp="20-Jun-25 07:32:35",
//        .temperature = 27.8
//    }

    //FIXME: bug at the moment, should be checked again.
    .csv_fiels_offset = {
        .device_info_offset = 0,
        .trip_info_offset = 0,
        .config_info_offset = 0,
        .alarm_summary_offset = 0,
		.alarm_summary_fill_offset = 0,
        .logging_summary_offset = 0,
        .marked_events_offset = 0,
		.logging_summary_avg_offset = 0,
    }
};

/* Private variables -----------------------------------------------*/
FIL csv_file;
char * csv_file_name = CSV_LOG_FILE_NAME;

/* Private function prototypes -----------------------------------------------*/
static void csv_header(void);
static void csv_device_info(const DeviceSettings *dev_setting);
static void csv_trip_info(const DeviceSettings *dev_setting);
static void csv_conf_info(const DeviceSettings *dev_setting);
static void csv_alarm_summary(const DeviceSettings *dev_setting);
static void csv_log_summary(void);
static void csv_dump_log_entries_with_summary_seek(void);
static void csv_recorded_data(void);

static void csv_alarm_summary_prealloc(const DeviceSettings *dev_setting);
static void csv_alarm_summary_fill(const DeviceSettings *dev_setting);
//FIXME: TBD
//static void csv_update_alarm_config_summary(void);


/* Public function prototypes -----------------------------------------------*/

/**
 * @brief
 * Generate a template CSV file with the necessary headers and initial data.
 * This function should be called once to create the CSV file structure.
 * @return FX_SUCCESS on success, or an error code on failure.
 */
UINT csv_gen_template_file(const DeviceSettings *dev_setting) {
    FRESULT res;
    FILINFO fno;

    // Check the file existance:
    res = f_stat (csv_file_name,&fno);
    switch (res)
    {
    case FR_OK:
        // File exists. Open it to append data.
        LOG_APP("File '%s' exists \r\n", csv_file_name);
        break;

    case FR_NO_FILE:
        // Create new file and generate template
        // File does not exist. Create it and write the header first.
        LOG_APP("File '%s' not found. Creating new file with header.\n", csv_file_name);
        res = f_open(&csv_file, csv_file_name, FA_CREATE_NEW | FA_WRITE);
        if (res != FR_OK)
        {
            LOG_APP("Open csv log file failed,Unmount...\r\n");
            f_mount(NULL, "", 0);
            return res;
        }
        //FIXME: pre-allocated here.
        csv_header();
        csv_device_info(dev_setting);
        csv_trip_info(dev_setting);
        csv_conf_info(dev_setting);
        csv_alarm_summary_prealloc(dev_setting);
        csv_dump_log_entries_with_summary_seek();
        csv_alarm_summary_fill(dev_setting);
//         csv_marked_event()
//        csv_recorded_data();
        LOG_APP("SUCCESS: Created template csv file\r\n");
        LOG_APP("Closing file, Flush...\r\n");
        res = f_close(&csv_file);
        if (res != FR_OK)
        {
            LOG_APP("Close CSV failed\r\n");
            return res;
        }

        break;

    default:
        LOG_APP("f_stat unknown error, ret:%02d\r\n",res);
        break;
    }
    return res;
}

/**
 * @brief
 * To append a new record to the CSV file.
 * This function should be called periodically to log new data points.
 * @param date The date of the record in "DD-MMM-YY HH:MM:SS" format.
 * @param time The time of the record in "HH:MM:SS" format.
 * @param temperature The temperature value to log.
 * @param radiation The radiation value to log.
 */
void csv_append_new_record(const file_log_time_t *time,
                           float temperature,
                           float radiation)
{
    char tmp_time[64];
    char tmp_buff[128] = {0};
    FRESULT status  =0;

    //OPEN log file:
    status = f_open(&csv_file, csv_file_name, FA_OPEN_APPEND | FA_WRITE);
    if (status != FR_OK)
    {
        LOG_APP("FATAL: f_open FAILED with status: 0x%02X\r\n", status);
        return ;
    }
    time_to_dmyhms_string(time,tmp_time,sizeof(tmp_time));
    snprintf(tmp_buff,
            sizeof(tmp_buff),
            " %s,  %.2f,  %.2f\n",
			tmp_time,temperature,radiation);

    LOG_APP("Inserting to CSV, size: %u\r\n",(unsigned int)(f_size(&csv_file)));

    status = f_lseek(&csv_file, f_size(&csv_file));
    if (status != FR_OK)
    {
        LOG_APP("FATAL: f_lseek FAILED with status: 0x%02X\r\n", status);
        return ;
    }

    fatfs_write_str(&csv_file,tmp_buff);

    status = f_close(&csv_file);
    if (status != FR_OK)
    {
        LOG_APP("FATAL: f_close FAILED with status: 0x%02X\r\n", status);
        return ;
    }

}

/**
 * @brief
 * Updates the logging summary section in the CSV file with the latest statistics.
 * This function sets temperature and radiation statistics, start/stop times, elapsed time,
 * and data point counts, then writes the updated summary to the CSV at the correct offset.
 *
 * @param highest_temp        Highest recorded temperature.
 * @param lowest_temp         Lowest recorded temperature.
 * @param average_temp        Average temperature.
 * @param mean_kinetic_temp   Mean kinetic temperature (MKT).
 * @param highest_radiation   Highest recorded radiation.
 * @param average_radiation   Average radiation.
 * @param start_time          Measurement start time (string).
 * @param stop_time           Measurement stop time (string).
 * @param elapsed_time_sec Total elapsed time in minutes.
 * @param count               Number of data points.
 */
void csv_gen_completed_report(const logging_summary_t *log)
{
    FRESULT status = 0;

    log_csv.logging_summary.highest_temp =log-> highest_temp;
    log_csv.logging_summary.lowest_temp =log-> lowest_temp;
    log_csv.logging_summary.average_temp =log-> average_temp;
    log_csv.logging_summary.mean_kinetic_temp =log-> mean_kinetic_temp;
    log_csv.logging_summary.highest_radiation =log-> highest_radiation;
    log_csv.logging_summary.average_radiation =log-> average_radiation;

    log_csv.logging_summary.start_time =log-> start_time;
    log_csv.logging_summary.stop_time =log-> stop_time;
    log_csv.logging_summary.elapsed_time_sec =log-> elapsed_time_sec;

    log_csv.logging_summary.data_points_temp_count =log-> data_points_temp_count;
    log_csv.logging_summary.data_points_radiation_count =log-> data_points_radiation_count;
    // seek to the offset of the logging summary
    status = f_open(&csv_file, csv_file_name, FA_OPEN_APPEND | FA_WRITE);
    if (status != FR_OK)
    {
        LOG_APP("FATAL: f_open FAILED with status: 0x%02X\r\n", status);
        return ;
    }
    //FIXME: HARDCODE INDEX??
    status = f_lseek(&csv_file, CSV_LOGGING_SUMMARY_OFFSET);
    if (status != FR_OK)
    {
        LOG_APP("FATAL: f_open FAILED with status: 0x%02X\r\n", status);
        return ;
    }
//    csv_dump_log_entries_with_summary_seek();

    // change attr to readonly
    if (f_chmod(csv_file_name,AM_RDO,AM_RDO) != FR_OK)
    {
        LOG_APP("f_chmod %s failed\r\n",csv_file_name);
    }
    if (f_close(&csv_file) != FR_OK)
    {
        LOG_APP("f_close %s failed\r\n",csv_file_name);
    }
    LOG_APP("Close CSV log file\r\n");
    printf("[DEBUG][csv_gen_completed_report] elapsed_time_sec = %lu\r\n",
           log_csv.logging_summary.elapsed_time_sec);
}

/*---------------------------------------------------------------------------*/
/* Utility functions */

// static void fatfs_write_str(FIL *file, const char *s) {
//      if (f_puts(s, file) < 0) {
//         LOG_FX("f_puts() failed to write string.\n");
//     }
// }

//convert seconds to a string in the format "DD HH MM SS"
/**
 * @brief Converts total minutes to a "HHH MM M SSS" string.
 * @param total_mins The total time in minutes.
 * @param buffer A pointer to the output buffer.
 * @param buffer_size The size of the output buffer.
 */
void minutes_to_hms_string(uint16_t total_mins, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    uint16_t hours = total_mins / 60;
    uint16_t minutes = total_mins % 60;

    snprintf(buffer, buffer_size, "%02uH %02uM %02uS", hours, minutes, 0);
}

void seconds_to_hms_string(uint32_t total_secs, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    uint32_t hours = total_secs / 3600;
    uint32_t minutes = (total_secs % 3600) / 60;
    uint32_t seconds = total_secs % 60;

    snprintf(buffer, buffer_size, "%02luH %02luM %02luS", hours, minutes, seconds);
}


/**
 * @brief Converts total minutes to a "DDH HH M SSS" string.
 * @param total_mins The total time in minutes.
 * @param buffer A pointer to the output buffer.
 * @param buffer_size The size of the output buffer.
 */
void minutes_to_dhms_string_long(uint32_t total_mins, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    unsigned int days = total_mins / 1440; // 1440 minutes in a day
    unsigned int hours = (total_mins % 1440) / 60;
    unsigned int minutes = total_mins % 60;

    snprintf(buffer, buffer_size, "%02uD %02uH %02uM %02uS", days, hours, minutes, 0);
}

void seconds_to_dhms_string_long(uint32_t total_seconds, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    unsigned int days    = total_seconds / 86400;              // 86400초 = 1일
    unsigned int hours   = (total_seconds % 86400) / 3600;     // 나머지에서 시간 추출
    unsigned int minutes = (total_seconds % 3600) / 60;        // 나머지에서 분 추출
    unsigned int seconds = total_seconds % 60;                 // 남은 초

    snprintf(buffer, buffer_size, "%02uD %02uH %02uM %02uS", days, hours, minutes, seconds);
}

/**
 * @brief Converts a file_log_time_t struct to a "dd-Mon-yyyy HH:MM:SS" string.
 * @param time A pointer to the time structure.
 * @param buffer A pointer to the output buffer.
 * @param buffer_size The size of the output buffer.
 */
void time_to_dmyhms_string(const file_log_time_t *time, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0 || time == NULL) {
        if(buffer != NULL && buffer_size > 0) buffer[0] = '\0';
        return;
    }

    static const char* months[] = {
        "???", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    const char* month_str = (time->month >= 1 && time->month <= 12) ? months[time->month] : months[0];

    snprintf(buffer, buffer_size,
             "%02d-%s-%02d, %02d:%02d:%02d",
             time->day,
             month_str,
             time->year, // Adjust year with offset
             time->hour,
             time->minute,
             time->second);
}
// Convert 1 -> Jan, etc
const char* convert_month_to_string(const uint8_t  month)
{
    static const char* months[] = {
        "???", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    return ((month >= 1 && month <= 12) ? months[month] : months[0]);
}

void time_to_hhmmss_string(const file_log_time_t *time, char *buffer, size_t buffer_size) {

}

/* CSV file functions*/
static void csv_header(void) {

    const char* header_static[] = {
        "sep=,\n",
        "Note:\n",
        "All temperatures are in Degree Celsius.\n",
        "All Radation are in uSv/h.\n",
        "All times shown are based on UTC + 00:00 and 24-Hour clock [DD-MMM-YY HH:MM:SS]\n\n",
    };
    //device info offset + 1 (end of line)
    log_csv.csv_fiels_offset.device_info_offset = sizeof(header_static) + 1;
    for (int i = 0; i < sizeof(header_static)/sizeof(header_static[0]); ++i)
        fatfs_write_str(&csv_file, header_static[i]);
}

static void csv_device_info(const DeviceSettings *dev_setting) {

	log_csv.device_info.device_code = dev_setting->device_code;
	log_csv.device_info.model_code = dev_setting->model;
	log_csv.device_info.firmware_version = dev_setting->firmware_ver;
	log_csv.device_info.serial_number = dev_setting->serial;

	// sensor_type 값에 따라 probe_type 문자열 설정
	if (dev_setting->sensor_type == 1)
	    log_csv.device_info.probe_type = "Temperature and Radiation (interval)";
	else if (dev_setting->sensor_type == 2)
	    log_csv.device_info.probe_type = "Temperature Only";
	else
	    log_csv.device_info.probe_type = "Unknown Sensor Type";

    char tmp_buff[512] = {0};
    int offset = 0;
    const int buffer_size = sizeof(tmp_buff);

    const int label_width = 20;

    // 1. Add the static header to the buffer
    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "Device Information\n"
                       "************************************************\n");

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "%-*s: %-*s\n", label_width, "Device code", label_width, log_csv.device_info.device_code);

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "%-*s: %-*s\n", label_width, "Probe Type", 40, log_csv.device_info.probe_type);

    log_csv.device_info.firmware_version = dev_setting-> firmware_ver;
    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "%-*s: %-*s\n", label_width, "Firmware Version", label_width, log_csv.device_info.firmware_version);

    log_csv.device_info.model_code = dev_setting-> model;
    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "%-*s: %-*s\n", label_width, "Model Code", label_width, log_csv.device_info.model_code);

    log_csv.device_info.serial_number = dev_setting-> serial;
    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "%-*s: %-*s\n\n", label_width, "Serial Number", label_width, log_csv.device_info.serial_number);
    log_csv.csv_fiels_offset.trip_info_offset = log_csv.csv_fiels_offset.device_info_offset + offset + 1; // +1 for newline

    fatfs_write_str(&csv_file, tmp_buff);
}

static void csv_trip_info(const DeviceSettings *dev_setting) {
	log_csv.trip_info.trip_id = dev_setting->trip_code;
	log_csv.trip_info.description = dev_setting->trip_desc;

    char tmp_buff[256] = {0};
    int offset = 0;
    const int buffer_size = sizeof(tmp_buff);
    const int label_width = 20;

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                    "Trip Information\n"
                    "************************************************\n");
    log_csv.trip_info.trip_id = (unsigned long) (dev_setting -> trip_code);
    offset += snprintf(tmp_buff + offset, buffer_size - offset,
        "%-*s: %07ld\n", label_width, "Trip Id", log_csv.trip_info.trip_id);

    log_csv.trip_info.description = dev_setting -> trip_desc;
    offset += snprintf(tmp_buff + offset, buffer_size - offset,
        "%-*s: %-*s\n", label_width, "Description", label_width, log_csv.trip_info.description);
    log_csv.csv_fiels_offset.config_info_offset = log_csv.csv_fiels_offset.trip_info_offset + offset + 1; // +1 for newline

    fatfs_write_str(&csv_file, tmp_buff);
}

static void csv_conf_info(const DeviceSettings *dev_setting) {
	log_csv.config_info.temp_interval_sec = dev_setting->temp_interval;
	log_csv.config_info.rad_interval_sec = dev_setting->rad_interval;
	log_csv.config_info.start_delay_sec = dev_setting->start_delay;

	// start_mode, stop_mode는 현재 고정 문자열 사용 중이라면 아래처럼 유지
	log_csv.config_info.start_mode = "Manual";
	log_csv.config_info.stop_mode = "Manual & Software";

    char tmp_buff[256] = {0};
    int offset = 0;
    const int buffer_size = sizeof(tmp_buff);

    // Define the alignment width for all labels
    const int label_width = 15;

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "\nConfiguration Information\n"
                       "************************************************\n");
    char tmp[16];

    seconds_to_hms_string(log_csv.config_info.temp_interval_sec, tmp, sizeof(tmp));

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "%-*s: %-*s\n", label_width, "Temp Log Interval",
                       label_width , tmp);

    log_csv.config_info.rad_interval_sec = dev_setting->rad_interval;
    seconds_to_hms_string(log_csv.config_info.rad_interval_sec, tmp, sizeof(tmp));

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "%-*s: %-*s\n", label_width, "Rad Log Interval",
                       label_width , tmp);


    offset += snprintf(tmp_buff + offset, buffer_size - offset, // 추가
                       "%-*s: %-20s\n", label_width, "Start Mode",
                       log_csv.config_info.start_mode);

    offset += snprintf(tmp_buff + offset, buffer_size - offset,  // 추가
                       "%-*s: %-20s\n", label_width, "Stop Mode",
                       log_csv.config_info.stop_mode);

    log_csv.config_info.start_delay_sec = (unsigned long)(dev_setting ->start_delay);
    seconds_to_hms_string(log_csv.config_info.start_delay_sec,
                           tmp, sizeof(tmp));

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "%-*s: %-*s\n\n", label_width, "Start Delay",
                       label_width, tmp);

    log_csv.csv_fiels_offset.alarm_summary_offset = log_csv.csv_fiels_offset.config_info_offset + offset + 1; // +1 for newline
    fatfs_write_str(&csv_file, tmp_buff);
}

//FIXME: correct alarm format when update.
//static void csv_alarm_summary(const DeviceSettings *dev_setting) {
//    char tmp_buff[768] = {0};
//    int offset = 0;
//    const int fixed_block_size = sizeof(tmp_buff);
//
//    // --- Static Headers ---
//    offset += snprintf(tmp_buff + offset, fixed_block_size - offset,
//                       "Alarm Configuration/Summary\n"
//                       "************************************************\n");
//    char tmp_time[32];
//
//    //FiXME: trigger at?
//    time_to_dmyhms_string(&log_csv.alarm_summary.alarm_triggered_at, tmp_time, sizeof(tmp_time));
//
//    // Manually padded to align the colon
//    offset += snprintf(tmp_buff + offset, fixed_block_size - offset,
//                       "Alarm At(Te):            %s\n\n",
//                       tmp_time);
//
//    offset += snprintf(tmp_buff + offset, fixed_block_size - offset,
//        "Alarm Zone      Alarm Delay          Total Time           Violation     Status\n");
//
//    // --- Loop Through Zones ---
//    //RH1
//    log_csv.alarm_summary.zones[0].threshold = dev_setting -> alarm_rh1;
//    log_csv.alarm_summary.zones[1].threshold = dev_setting -> alarm_rh2;
//    log_csv.alarm_summary.zones[2].threshold = dev_setting -> alarm_th1;
//    log_csv.alarm_summary.zones[3].threshold = dev_setting -> alarm_th2;
//    log_csv.alarm_summary.zones[4].threshold = dev_setting -> alarm_tl1;
//    log_csv.alarm_summary.zones[5].threshold = dev_setting -> alarm_tl2;
//
//    for (int i = 0; i < 6; i++)
//    {
//        char tmp_alarm_delay[32];
//        char tmp_time_in_violation[32];
//        log_csv.alarm_summary.zones[i].alarm_delay_sec = dev_setting -> alarm_delay;
//        minutes_to_dhms_string_long(log_csv.alarm_summary.zones[i].alarm_delay_sec,
//                                      tmp_alarm_delay, sizeof(tmp_alarm_delay));
//
//        minutes_to_dhms_string_long(log_csv.alarm_summary.zones[i].total_time_in_violation_minutes,
//                                      tmp_time_in_violation, sizeof(tmp_time_in_violation));
//
//        const char *status = (log_csv.alarm_summary.zones[i].status == ALARM_STATUS_ALARM) ? "ALARM" : "OK";
//
//        if (i >= 2) { // TH1, TH2, TL1, TL2 (온도, 부호 있음)
//            offset += snprintf(tmp_buff + offset, fixed_block_size - offset,
//                               "%-4s: %6d   %-20s %-20s \t%-10u %-10s\n",
//                               log_csv.alarm_summary.zones[i].zone_name,
//                               (int16_t)log_csv.alarm_summary.zones[i].threshold,
//                               tmp_alarm_delay,
//                               tmp_time_in_violation,
//                               log_csv.alarm_summary.zones[i].violation_count,
//                               status);
//        } else { // RH1, RH2 (습도, 부호 없음)
//            offset += snprintf(tmp_buff + offset, fixed_block_size - offset,
//                               "%-4s: %6u   %-20s %-20s \t%-10u %-10s\n",
//                               log_csv.alarm_summary.zones[i].zone_name,
//                               (uint16_t)log_csv.alarm_summary.zones[i].threshold,
//                               tmp_alarm_delay,
//                               tmp_time_in_violation,
//                               log_csv.alarm_summary.zones[i].violation_count,
//                               status);
//        }
//
//    }
//
//    fatfs_write_str(&csv_file, tmp_buff);
//}

// 2-1) 헤더와 테이블 캡션만 먼저 쓰고, 채움 시작 오프셋 기록 + 패딩
static void csv_alarm_summary_prealloc(const DeviceSettings *dev)
{
    char buf[256] = {0};
    int off = 0;
    char tmp_time[32];

    // 섹션 제목/헤더
    off += snprintf(buf + off, sizeof(buf)-off,
        "Alarm Configuration/Summary\n"
        "************************************************\n");

    // 트리거 시간(초기엔 0 또는 N/A로 표기)
    time_to_dmyhms_string(&log_csv.alarm_summary.alarm_triggered_at, tmp_time, sizeof(tmp_time));
    off += snprintf(buf + off, sizeof(buf)-off,
        "Alarm At(Te):            %s\n\n", tmp_time);

    // 컬럼 헤더만 출력
    off += snprintf(buf + off, sizeof(buf)-off,
        "Alarm Zone      Alarm Delay          Total Time           Violation     Status\n");

    // 섹션 시작/고정 오프셋: (원한다면) 저장
    log_csv.csv_fiels_offset.alarm_summary_offset = f_tell(&csv_file);

    // 여기부터가 "되덮어쓸 본문" 시작점
    fatfs_write_str(&csv_file, buf);
    log_csv.csv_fiels_offset.alarm_summary_fill_offset = f_tell(&csv_file);

    // 나중에 덮어쓸 고정 길이 패딩
    char pad[ALARM_SUMMARY_PAD_BYTES];
    memset(pad, ' ', sizeof(pad));
    f_write(&csv_file, pad, sizeof(pad), NULL);

    // 실제 값들과는 분리: recorded data 등 다음 섹션으로 이어짐
}


static void csv_alarm_summary_fill(const DeviceSettings *dev_setting)
{
    char tmp_buff[768] = {0};
    int offset = 0;
    const int fixed_block_size = sizeof(tmp_buff);

    printf("[CSV][AlarmFill] 알람 요약 데이터 채우기 시작\r\n");

    // ★ 더 이상 zones[].threshold 에 /10.0f 로 저장하지 않습니다. (표시는 지역변수로 처리)

    // ── 본문(각 Zone 라인)만 tmp_buff에 누적 ──
    for (int i = 0; i < 6; i++) {
        char s_delay[32], s_tot[32];
        uint32_t delay_sec = 0;
        uint32_t violations = 0;

        switch (i) {
            case 0: delay_sec = dev_setting->alarm_delay_rh1; violations = log_csv.logging_summary.RH1_alarm_count; break;
            case 1: delay_sec = dev_setting->alarm_delay_rh2; violations = log_csv.logging_summary.RH2_alarm_count; break;
            case 2: delay_sec = dev_setting->alarm_delay_th1; violations = log_csv.logging_summary.TH1_alarm_count; break;
            case 3: delay_sec = dev_setting->alarm_delay_th2; violations = log_csv.logging_summary.TH2_alarm_count; break;
            case 4: delay_sec = dev_setting->alarm_delay_tl1; violations = log_csv.logging_summary.TL1_alarm_count; break;
            case 5: delay_sec = dev_setting->alarm_delay_tl2; violations = log_csv.logging_summary.TL2_alarm_count; break;
        }

        seconds_to_dhms_string_long(delay_sec, s_delay, sizeof(s_delay));

        // 예시 계산(실제 위반 누적 시간을 따로 관리한다면 그 값을 쓰세요)
        uint32_t tot_violation_sec = 0;
        if (log_csv.logging_summary.elapsed_time_sec > delay_sec) {
            tot_violation_sec = log_csv.logging_summary.elapsed_time_sec - delay_sec;
        }
        seconds_to_dhms_string_long(tot_violation_sec, s_tot, sizeof(s_tot));

        const char *status = (violations >= 1) ? "ALARM" : "OK";
        log_csv.alarm_summary.zones[i].status = (violations >= 1) ? ALARM_STATUS_ALARM : ALARM_STATUS_OK;
        log_csv.alarm_summary.zones[i].alarm_delay_sec = delay_sec;
        log_csv.alarm_summary.zones[i].total_time_in_violation_minutes = tot_violation_sec; // 이름과 달리 sec일 수 있음
        log_csv.alarm_summary.zones[i].violation_count = violations;

        // ★ 표시용 threshold 계산 & 올바른 포맷으로 출력
        if (i < 2) {
            // RH1/RH2: 원시단위가 0.01이므로 /10 해서 정수 표시 (예: 200000 → 20000)
            unsigned rh_disp = (i == 0) ? (unsigned)(dev_setting->alarm_rh1 / 10U)
                                        : (unsigned)(dev_setting->alarm_rh2 / 10U);
            offset += snprintf(tmp_buff + offset, fixed_block_size - offset,
                               "%-4s: %6u   %-20s %-20s \t%-10lu %-10s\n",
                               log_csv.alarm_summary.zones[i].zone_name,
                               rh_disp, s_delay, s_tot,
                               (unsigned long)violations, status);
        } else {
            // TH/TL: 원시단위가 0.1이므로 /10 해서 부호 있는 정수 표시 (예: -600 → -60)
            int thtl_disp = (i == 2) ? (int)(dev_setting->alarm_th1 / 10)
                            : (i == 3) ? (int)(dev_setting->alarm_th2 / 10)
                            : (i == 4) ? (int)(dev_setting->alarm_tl1 / 10)
                                       : (int)(dev_setting->alarm_tl2 / 10);
            offset += snprintf(tmp_buff + offset, fixed_block_size - offset,
                               "%-4s: %6d   %-20s %-20s \t%-10lu %-10s\n",
                               log_csv.alarm_summary.zones[i].zone_name,
                               thtl_disp, s_delay, s_tot,
                               (unsigned long)violations, status);
        }

        if (offset >= fixed_block_size) {
            printf("[CSV][AlarmFill][WARN] tmp_buff overflow, truncated\r\n");
            offset = fixed_block_size; // 안전
            break;
        }
    }

    // ── CSV에 덮어쓰기 ──
    FRESULT fr = f_lseek(&csv_file, log_csv.csv_fiels_offset.alarm_summary_fill_offset);
    if (fr != FR_OK) {
        printf("[CSV][AlarmFill][ERR] f_lseek fail: 0x%02X\r\n", fr);
        return;
    }

    UINT to_write = (offset <= ALARM_SUMMARY_PAD_BYTES) ? offset : ALARM_SUMMARY_PAD_BYTES;
    UINT bw = 0;
    fr = f_write(&csv_file, tmp_buff, to_write, &bw);
    if (fr != FR_OK || bw != to_write) {
        printf("[CSV][AlarmFill][ERR] f_write fail: 0x%02X, bw=%u/%u\r\n", fr, (unsigned)bw, (unsigned)to_write);
        return;
    }
    if (offset > ALARM_SUMMARY_PAD_BYTES) {
        printf("[CSV][AlarmFill][WARN] content truncated to %u bytes (pad=%u)\r\n",
               (unsigned)to_write, (unsigned)ALARM_SUMMARY_PAD_BYTES);
    }

    printf("[CSV][AlarmFill] 알람 요약 데이터 채움 완료: %u bytes at offset %lu\r\n",
           (unsigned)to_write, (unsigned long)log_csv.csv_fiels_offset.alarm_summary_fill_offset);
}




//Update the logging based on: DeviceConfig *cfg.
static void csv_log_summary() {
    char tmp_buff[512] = {0};
    int offset = 0;
    const int buffer_size = sizeof(tmp_buff);

    // Header section
    offset += snprintf(tmp_buff + offset, buffer_size - offset,
    		           "\r\n"
                       "\nLogging Summary\n"
                       "************************************************\n");

    // Data sections with manual space padding for perfect alignment
    // All colons will now line up vertically.
    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "Highest Temperature:   %6.1f\n",
                       log_csv.logging_summary.highest_temp);

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "Lowest Temperature:    %6.1f\n",
                       log_csv.logging_summary.lowest_temp);

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "Average Temperature:  .."
                       " %6.1f\n",
                       log_csv.logging_summary.average_temp);

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "Highest Radiation:     %6lu\n",
                       log_csv.logging_summary.highest_radiation);

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "Average Radiation:     %6lu\n",
                       log_csv.logging_summary.average_radiation);

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "MKT:                   %6.1f\n",
                       log_csv.logging_summary.mean_kinetic_temp);

    char time_str[32];
    time_to_dmyhms_string(&log_csv.logging_summary.start_time, time_str, sizeof(time_str));

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "Start Time:            %s\n",
                       time_str);
    time_to_dmyhms_string(&log_csv.logging_summary.stop_time, time_str, sizeof(time_str));

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "Stop Time:             %s\n",
                       time_str);

    char tmp[16];
    seconds_to_dhms_string_long(log_csv.logging_summary.elapsed_time_sec,
                               tmp, sizeof(tmp));

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "Elapsed Time:          %s\n",
                       tmp);

    // Fixed 6-space padding for data points
    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "Temp Data Points:      %6u\n",
                       log_csv.logging_summary.data_points_temp_count);

    offset += snprintf(tmp_buff + offset, buffer_size - offset,
                       "Rad Data Points:       %6u\n\n",
                       log_csv.logging_summary.data_points_radiation_count);

    // Remainder of the function
    log_csv.csv_fiels_offset.marked_events_offset = log_csv.csv_fiels_offset.logging_summary_offset + offset + 1;
    fatfs_write_str(&csv_file, tmp_buff);
}

static void csv_dump_log_entries_with_summary_seek(void) {
    log_entry_t entry;
    uint32_t max_idx = LOG_MAX_SIZE / ENTRY_SIZE;
    double sum_temp = 0.0, sum_dose = 0.0;
    uint32_t valid_count = 0;
    uint32_t valid_rad_count = 0;
    bool rh1_lat = false, rh2_lat = false, th1_lat = false, th2_lat = false, tl1_lat = false, tl2_lat = false;
    log_csv.logging_summary.RH1_alarm_count = 0;
    log_csv.logging_summary.RH2_alarm_count = 0;
    log_csv.logging_summary.TH1_alarm_count = 0;
    log_csv.logging_summary.TH2_alarm_count = 0;
    log_csv.logging_summary.TL1_alarm_count = 0;
    log_csv.logging_summary.TL2_alarm_count = 0;

    log_entry_t first = {0}, last = {0};
    bool first_found = false;
    char tmp_buff[256];

    extern uint16_t idx_rad;
    extern uint16_t idx_temp;
    log_csv.logging_summary.highest_temp   = device_config.temp_max / 10.0f;   // ex: 287 → 28.7℃
    log_csv.logging_summary.lowest_temp    = device_config.temp_min / 10.0f;
    log_csv.logging_summary.highest_radiation   = device_config.dose_max / 100.0f;  // ex: 1965 → 19.65uSv/h

    // 1. Logging Summary Header 및 자리 확보
    char summary_buff[512] = {0};
    int offset = 0;
    offset += snprintf(summary_buff + offset, sizeof(summary_buff) - offset,
        "\nLogging Summary\n"
        "************************************************\n"
        "Highest Temperature:   %6.1f\n"
        "Lowest Temperature:    %6.1f\n"
        "Highest Radiation Dose:%7.2f\n",
        log_csv.logging_summary.highest_temp,
        log_csv.logging_summary.lowest_temp,
        log_csv.logging_summary.highest_radiation);

    fatfs_write_str(&csv_file, summary_buff);
    log_csv.csv_fiels_offset.logging_summary_avg_offset = f_tell(&csv_file);

    char summary_padding[512];
    memset(summary_padding, ' ', sizeof(summary_padding));
    f_write(&csv_file, summary_padding, sizeof(summary_padding), NULL);
    // 2. 로그 헤더
    fatfs_write_str(&csv_file,
        "\nRecorded Data\n"
        "************************************************\n"
        "Index,Date,Time,Temperature,Rad,Mark\n");

    // 3. Flash 순회
    for (uint32_t i = 0; i < max_idx; i++) {
        meas_data_log_read_entry(i, &entry);
        if (entry.year == 0xFF || entry.index == 0xFFFF || entry.month == 0xFF) break;

        float temp = entry.temperature / 10.0f;
        float dose = entry.dose / 100.0f;
        char rad_str[16];

        // Rad 미측정 구간이면 "n/a"로
        if (entry.rad_measure_mark == 0) {
            strcpy(rad_str, "n/a");
        } else {
            snprintf(rad_str, sizeof(rad_str), "%.2f", dose);
        }

        if (!first_found) {
            first = entry;
            first_found = true;
        }
        last = entry;

        sum_temp += temp;

        if (entry.rad_measure_mark != 0) {
            sum_dose += dose;
            // rad valid count 별도
            valid_rad_count++;

        }
        valid_count++;

        // --- [추가] 루프 내부, sum/CSV 쓰기 전에 ---
        int16_t  t_x10  = (int16_t)entry.temperature;   // 0.1℃ 단위
        uint16_t d_x100 = (uint16_t)entry.dose;         // 0.01 단위

        uint32_t interval_sec = current_settings.temp_interval;
        uint32_t elapsed_sec  = (valid_count > 0) ? (valid_count - 1U) * interval_sec : 0U;

        bool gate_rh1 = (elapsed_sec >= current_settings.alarm_delay_rh1);
        bool gate_rh2 = (elapsed_sec >= current_settings.alarm_delay_rh2);
        bool gate_th1 = (elapsed_sec >= current_settings.alarm_delay_th1);
        bool gate_th2 = (elapsed_sec >= current_settings.alarm_delay_th2);
        bool gate_tl1 = (elapsed_sec >= current_settings.alarm_delay_tl1);
        bool gate_tl2 = (elapsed_sec >= current_settings.alarm_delay_tl2);

        // 조건(게이트 적용) — RH는 유효 방사선 구간에서만 판정
        bool rh1_cond = gate_rh1 && (entry.rad_measure_mark != 0) && (d_x100 >= current_settings.alarm_rh1);
        bool rh2_cond = gate_rh2 && (entry.rad_measure_mark != 0) && (d_x100 >= current_settings.alarm_rh2);
        bool th1_cond = gate_th1 && (t_x10 >= current_settings.alarm_th1);
        bool th2_cond = gate_th2 && (t_x10 >= current_settings.alarm_th2);
        bool tl1_cond = gate_tl1 && (t_x10 <= current_settings.alarm_tl1);
        bool tl2_cond = gate_tl2 && (t_x10 <= current_settings.alarm_tl2);

        // 카운트(현재 방식: 조건 만족 샘플 수 기반)
        if (rh1_cond) log_csv.logging_summary.RH1_alarm_count++;
        if (rh2_cond) log_csv.logging_summary.RH2_alarm_count++;
        if (th1_cond) log_csv.logging_summary.TH1_alarm_count++;
        if (th2_cond) log_csv.logging_summary.TH2_alarm_count++;
        if (tl1_cond) log_csv.logging_summary.TL1_alarm_count++;
        if (tl2_cond) log_csv.logging_summary.TL2_alarm_count++;

//        // 엣지 감지: false -> true 전환 시 1회만 증가
//        if (rh1_cond) { if (!rh1_lat) { log_csv.logging_summary.RH1_alarm_count++; rh1_lat = true; } }
//        else          { rh1_lat = false; }
//
//        if (rh2_cond) { if (!rh2_lat) { log_csv.logging_summary.RH2_alarm_count++; rh2_lat = true; } }
//        else          { rh2_lat = false; }
//
//        if (th1_cond) { if (!th1_lat) { log_csv.logging_summary.TH1_alarm_count++; th1_lat = true; } }
//        else          { th1_lat = false; }
//
//        if (th2_cond) { if (!th2_lat) { log_csv.logging_summary.TH2_alarm_count++; th2_lat = true; } }
//        else          { th2_lat = false; }
//
//        if (tl1_cond) { if (!tl1_lat) { log_csv.logging_summary.TL1_alarm_count++; tl1_lat = true; } }
//        else          { tl1_lat = false; }
//
//        if (tl2_cond) { if (!tl2_lat) { log_csv.logging_summary.TL2_alarm_count++; tl2_lat = true; } }
//        else          { tl2_lat = false; }


        snprintf(tmp_buff, sizeof(tmp_buff),
            "%u,%04u-%02u-%02u,%02u:%02u:%02u,%.1f,%s,0x%02X\n",
            entry.index,
            2000 + entry.year, entry.month, entry.day,
            entry.hour, entry.minute, entry.second,
            temp, rad_str, entry.mark);
        fatfs_write_str(&csv_file, tmp_buff);
    }

    if (!first_found || valid_count == 0) {
        LOG_APP("[ERROR] No valid entries found!\n");
        return;
    }

    // 4. 평균 계산 및 구조체 반영
    float avg_temp = sum_temp / valid_count;
    float avg_dose = (valid_rad_count ? (sum_dose / valid_rad_count) : 0.0f);
    float mkt = avg_temp;

    log_csv.logging_summary.average_temp = avg_temp;
    log_csv.logging_summary.average_radiation = avg_dose;
    log_csv.logging_summary.mean_kinetic_temp = mkt;

    log_csv.logging_summary.start_time = (file_log_time_t){
        .year = first.year, .month = first.month, .day = first.day,
        .hour = first.hour, .minute = first.minute, .second = first.second };

    log_csv.logging_summary.stop_time = (file_log_time_t){
        .year = last.year, .month = last.month, .day = last.day,
        .hour = last.hour, .minute = last.minute, .second = last.second };

    log_csv.logging_summary.elapsed_time_sec = (valid_count-1) * current_settings.temp_interval;
    log_csv.logging_summary.data_points_temp_count = idx_temp;
    log_csv.logging_summary.data_points_radiation_count = idx_rad;

    // 5. 평균 정보 문자열 생성 및 덮어쓰기
    char avg_text[320] = {0};
    int avg_offset = 0;
    char time_str[32], tmp[20];

    avg_offset += snprintf(avg_text + avg_offset, sizeof(avg_text) - avg_offset,
        "Average Temperature:   %6.1f\n", avg_temp);
    avg_offset += snprintf(avg_text + avg_offset, sizeof(avg_text) - avg_offset,
        "Average Radiation:     %6.1f\n", avg_dose);
    avg_offset += snprintf(avg_text + avg_offset, sizeof(avg_text) - avg_offset,
        "MKT:                   %6.1f\n", mkt);

    time_to_dmyhms_string(&log_csv.logging_summary.start_time, time_str, sizeof(time_str));
    avg_offset += snprintf(avg_text + avg_offset, sizeof(avg_text) - avg_offset,
        "Start Time:            %s\n", time_str);

    time_to_dmyhms_string(&log_csv.logging_summary.stop_time, time_str, sizeof(time_str));
    avg_offset += snprintf(avg_text + avg_offset, sizeof(avg_text) - avg_offset,
        "Stop Time:             %s\n", time_str);

    seconds_to_dhms_string_long(log_csv.logging_summary.elapsed_time_sec, tmp, sizeof(tmp));

    avg_offset += snprintf(avg_text + avg_offset, sizeof(avg_text) - avg_offset,
        "Elapsed Time:          %s\n", tmp);
    avg_offset += snprintf(avg_text + avg_offset, sizeof(avg_text) - avg_offset,
        "Temp Data Points:      %6u\n", idx_temp);
    avg_offset += snprintf(avg_text + avg_offset, sizeof(avg_text) - avg_offset,
        "Rad Data Points:       %6u\n", idx_rad);

    f_lseek(&csv_file, log_csv.csv_fiels_offset.logging_summary_avg_offset);

    f_write(&csv_file, avg_text, avg_offset, NULL);
}



// static void csv_marked_event(void) {
//     char tmp_buff[128] = {0};
//     snprintf(tmp_buff,
//             sizeof(tmp_buff),
//             "Marked Events\n"
//             "************************************************\n\n");
//     fatfs_write_str(&csv_file, tmp_buff);
// }


static void csv_recorded_data(void) {
    char tmp_buff[128] = {0};
    snprintf(tmp_buff,
            sizeof(tmp_buff),
            "Recorded Data\n"
            "************************************************\n"
            "Date,Time,Temperature,Rad\n");
    fatfs_write_str(&csv_file, tmp_buff);
}

const logging_summary_t* csv_get_logging_summary(void)
{
    return &log_csv.logging_summary;
}

//FIXME: TBD
//static void csv_update_alarm_config_summary(void) {
//
//    // csv_alarm_summary();
//}

