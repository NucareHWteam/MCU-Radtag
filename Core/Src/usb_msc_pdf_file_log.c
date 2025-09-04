/*
 * usb_msc_pdf_file_log.c
 *
 *  Created on: Jul 14, 2025
 *      Author: dongo
 */
#include <usb_msc_file_log.h>
#include "log_system.h"
#include "pdflib.h"
#include "main.h"
#include "ux_port.h"
#include <stdbool.h>
#include "meas_data_log.h"
#include "stm32u0xx_hal_rtc.h"
#include "stm32u0xx_hal_rtc_ex.h"
#include <string.h>
extern RTC_HandleTypeDef hrtc;
extern DeviceSettings current_settings;
extern DeviceConfig device_config;

/* Data Structures -------------------------------------------------------------*/
//FIXME: remove it?
typedef struct {
    const char* zone;
    const char* allow_time;
    const char* alarm_type;
    const char* total_time;
    const char* violations;
    const char* status;
    int is_alarm; // Flag to indicate if the status should be red
} AlarmRowData;

// NEW: Data structure for the logging summary section
//FIXME: remove it?
typedef struct {
    const char* highest_T;
    const char* lowest_T;
    const char* average_T;
    const char* mkt;
    const char* highest_R;
    const char* average_R;
    const char* alarm_at;
    const char* start_time;
    const char* stop_time;
    const char* elapsed_time;
    const char* data_points;
} LoggingSummaryData;




void seconds_to_dhms_string_long(uint32_t sec, char *out, size_t out_sz);

/* Private macro -------------------------------------------------------------*/
#define PDF_LOG_FILE_NAME "Data_Log.pdf"
#define PDF_COMP_HTTPS_LINK "https://gonucare.com/"

#define PDF_PAGE_WIDTH_A4   595
#define PDF_PAGE_HEIGHT_A4  842

// Page configuration as A4 size
#define PDF_PAGE_LEFT              50
#define PDF_PAGE_RIGHT             (PDF_PAGE_WIDTH_A4 - 50)
#define PDF_PAGE_TOP               800
#define PDF_PAGE_BOTTOM            50

//#define TOTAL_PDF_OBJECTS     58   // The number of objects we will create (1 through 6)
//#define TOTAL_PAGES           2    // The number of pages in the document

// Page Log Info format:
#define TEXT_START                          PDF_PAGE_LEFT + 5
#define TEXT_SECOND_COL_START               TEXT_START + 245
#define LINE_SPACING                        12
// The gap between last line of previous fields with new fields
#define FIELD_SPACING                       20
#define SECTION_TITLE_FONT_SIZE             10
#define SECTION_CONTENT_FONT_SIZE           8

// =================================================================================
// 1. CHART DEFINITIONS AND CONSTANTS
// =================================================================================
//chart size: 450 x 250
// --- Bounding box for the plotting area ---
#define CHART_X_START           PDF_PAGE_LEFT + 25
 #define CHART_Y_START          150
#define CHART_WIDTH             450
#define CHART_HEIGHT            250

// --- Y - Axis (Left) - Temperature ---
// Lowest temperature value that sensor can measure, this threshold is also use for scaling chart.
#define TEMP_MIN                -10.0f
static float g_temp_axis_min = -10.0f;
#define TEMP_LABLE_NUMS          5
#define TEMP_SCALE_UP           8.0f  // top chart: highest value plus 8.0

// --- Y-Axis (Right) - Radiation ---
// Lowest radiation value that sensor can measure, this threshold is also use for scaling chart.
#define RAD_MIN                0
#define RAD_LABEL_NUMS          7
//FIXME: Scale up to 100 is too large?
#define RAD_SCALE_UP            100



#if 0 // --- Sample Data ---
#define NUM_DATA_POINTS 400

float sample_temperature_data[NUM_DATA_POINTS] = {
    24.93, 23.84, 25.72, 24.38, 26.67, 23.20, 24.39, 25.16, 23.95, 26.12,
    24.10, 26.01, 24.57, 25.76, 24.29, 23.85, 25.49, 26.92, 24.47, 25.91,
    23.73, 25.08, 24.62, 24.77, 26.43, 23.91, 25.65, 24.37, 26.06, 24.05,
    24.84, 25.83, 23.20, 24.43, 26.78, 24.22, 25.32, 23.96, 26.55, 25.23,
    24.14, 26.33, 24.18, 23.60, 25.12, 26.24, 24.02, 24.63, 26.87, 25.43,
    24.17, 25.56, 23.57, 24.34, 23.67, 26.16, 24.93, 25.96, 24.48, 25.04,
    23.36, 24.74, 25.28, 26.38, 24.07, 24.97, 23.80, 26.19, 25.69, 24.10,
    25.61, 24.34, 26.59, 25.19, 24.15, 24.25, 26.00, 24.47, 23.88, 24.25,
    26.26, 25.45, 23.51, 24.55, 25.92, 24.08, 23.87, 26.72, 23.44, 25.35,
    24.30, 26.90, 24.13, 24.85, 25.07, 26.63, 24.00, 23.76, 24.11, 25.26,
    23.96, 24.50, 26.71, 24.79, 26.47, 23.31, 24.62, 25.86, 24.39, 24.21,
    26.80, 24.00, 25.73, 23.92, 24.82, 26.68, 25.39, 24.16, 23.59, 26.96,
    25.52, 24.71, 23.82, 26.43, 24.60, 24.39, 25.97, 24.27, 26.20, 25.14,
    24.09, 26.50, 24.90, 25.66, 24.33, 24.72, 23.55, 25.20, 24.45, 23.53,
    26.29, 24.36, 24.29, 25.80, 26.09, 24.05, 23.89, 23.98, 25.48, 26.65,
    24.13, 23.51, 25.10, 23.43, 26.93, 24.68, 24.91, 26.13, 23.65, 25.63,
    24.32, 26.35, 24.24, 25.16, 24.69, 26.75, 23.88, 24.56, 23.45, 25.36,
    26.51, 23.98, 23.71, 24.86, 24.31, 25.59, 26.85, 24.07, 23.76, 25.42,
    26.28, 24.19, 25.01, 26.10, 24.41, 24.42, 24.12, 25.88, 23.92, 24.20,
    25.95, 24.04, 26.84, 24.96, 25.77, 24.17, 24.55, 23.83, 26.36, 25.34,
    24.26, 26.04, 24.22, 25.30, 24.12, 26.45, 24.52, 25.67, 23.69, 24.71,
    23.83, 25.22, 26.60, 24.33, 24.03, 25.55, 23.47, 24.94, 25.81, 23.91,
    24.87, 23.46, 25.50, 26.34, 24.23, 23.70, 26.23, 25.68, 24.01, 24.19,
    25.05, 26.02, 24.49, 23.56, 24.60, 25.40, 26.66, 24.58, 25.90, 24.28,
    26.15, 24.76, 24.26, 25.44, 26.86, 24.14, 24.67, 25.00, 23.75, 25.93,
    24.56, 24.43, 26.91, 24.24, 25.09, 24.11, 26.21, 24.78, 24.64, 25.18,
    24.08, 26.14, 24.46, 24.16, 25.89, 26.82, 23.68, 25.11, 24.06, 26.08,
    25.41, 23.93, 23.87, 24.59, 24.61, 25.13, 26.67, 24.03, 23.54, 26.41,
    24.41, 25.72, 24.23, 26.32, 24.44, 25.38, 24.47, 25.84, 23.79, 24.65,
    25.25, 26.40, 24.27, 24.78, 23.50, 25.17, 26.39, 24.09, 25.03, 23.90,
    26.44, 24.36, 25.37, 24.15, 26.37, 24.66, 25.21, 24.18, 23.72, 25.94,
    26.06, 24.40, 24.38, 25.19, 26.17, 24.42, 24.70, 25.06, 24.01, 26.52,
    26.15, 24.76, 24.26, 25.44, 26.86, 24.14, 24.67, 25.00, 23.75, 25.93,
    24.56, 24.43, 26.91, 24.24, 25.09, 24.11, 26.21, 24.78, 24.64, 25.18,
    24.08, 26.14, 24.46, 24.16, 25.89, 26.82, 23.68, 25.11, 24.06, 26.08,
    25.41, 23.93, 23.87, 24.59, 24.61, 25.13, 26.67, 24.03, 23.54, 26.41,
    24.41, 25.72, 24.23, 26.32, 24.44, 25.38, 24.47, 25.84, 23.79, 24.65,
    25.25, 26.40, 24.27, 24.78, 23.50, 25.17, 26.39, 24.09, 25.03, 23.90,
    26.44, 24.36, 25.37, 24.15, 26.37, 24.66, 25.21, 24.18, 23.72, 25.94,
    26.06, 24.40, 24.38, 25.19, 26.17, 24.42, 24.70, 25.06, 24.01, 26.52
};

float sample_radiation_data[NUM_DATA_POINTS] = {
    // Stable baseline
    250.5, 251.2, 250.8, 251.5, 251.0, 251.8, 251.3, 252.0, 251.5, 251.2,
    251.8, 252.1, 251.7, 252.3, 252.0, 252.5, 252.2, 252.7, 252.4, 252.9,
    252.6, 253.0, 252.8, 253.1, 253.0, 253.3, 253.1, 253.4, 253.2, 253.5,
    253.3, 253.6, 253.4, 253.7, 253.5, 253.8, 253.6, 253.9, 253.7, 254.0,

    // --- First Radiation Peak Event (starts around index 40) ---
    255.0, 260.2, 268.3, 275.9, 285.1, 295.6, 305.2, 315.8, 320.1, 318.5,
    310.4, 300.3, 290.8, 280.6, 270.9, 265.4, 260.1, 255.8, 254.5, 254.2,

    // Return to stable baseline
    253.9, 253.6, 253.3, 253.0, 252.7, 252.4, 252.1, 251.8, 251.5, 251.2,
    250.9, 250.6, 250.3, 250.0, 249.7, 249.4, 249.1, 248.8, 248.5, 248.2,
    247.9, 247.6, 247.3, 247.0, 246.7, 246.4, 246.1, 245.8, 245.5, 245.2,
    244.9, 244.6, 244.3, 244.0, 243.7, 243.4, 243.1, 242.8, 242.5, 242.2,
    241.9, 241.6, 241.3, 241.0, 240.7, 240.4, 240.1, 240.0, 240.1, 240.2,
    240.3, 240.4, 240.5, 240.6, 240.7, 240.8, 240.9, 241.0, 241.1, 241.2,
    241.3, 241.4, 241.5, 241.6, 241.7, 241.8, 241.9, 242.0, 242.1, 242.2,
    242.3, 242.4, 242.5, 242.6, 242.7, 242.8, 242.9, 243.0, 243.1, 243.2,
    243.3, 243.4, 243.5, 243.6, 243.7, 243.8, 243.9, 244.0, 244.1, 244.2,
    244.3, 244.4, 244.5, 244.6, 244.7, 244.8, 244.9, 245.0, 245.1, 245.2,

    // --- Second Radiation Peak Event (starts around index 200) ---
    246.0, 250.0, 258.0, 269.0, 281.0, 295.0, 310.0, 325.0, 335.0, 340.0,
    338.0, 330.0, 320.0, 310.0, 300.0, 290.0, 280.0, 270.0, 260.0, 255.0,

    // Return to stable baseline
    254.0, 253.0, 252.0, 251.0, 250.0, 249.5, 249.0, 248.5, 248.0, 247.5,
    247.0, 246.5, 246.0, 245.5, 245.0, 244.5, 244.0, 243.5, 243.0, 242.5,
    242.0, 241.5, 241.0, 240.5, 240.0, 239.5, 239.0, 238.5, 238.0, 237.5,
    237.0, 236.5, 236.0, 235.5, 235.0, 234.5, 234.0, 233.5, 233.0, 232.5,
    232.0, 231.5, 231.0, 230.5, 230.0, 230.1, 230.2, 230.3, 230.4, 230.5,
    230.6, 230.7, 230.8, 230.9, 231.0, 231.1, 231.2, 231.3, 231.4, 231.5,
    231.6, 231.7, 231.8, 231.9, 232.0, 232.1, 232.2, 232.3, 232.4, 232.5,
    232.6, 232.7, 232.8, 232.9, 233.0, 233.1, 233.2, 233.3, 233.4, 233.5,
    233.6, 233.7, 233.8, 233.9, 234.0, 234.1, 234.2, 234.3, 234.4, 234.5,
    234.6, 234.7, 234.8, 234.9, 235.0, 235.1, 235.2, 235.3, 235.4, 235.5,

    // --- Third Radiation Peak Event (starts around index 340) ---
    236.0, 240.0, 248.0, 259.0, 271.0, 285.0, 300.0, 290.0, 280.0, 270.0,
    260.0, 255.0, 250.0, 248.0, 246.0, 244.0, 242.0, 240.0, 239.0, 238.0,

    // End on stable baseline
    237.5, 237.0, 236.5, 236.0, 235.5, 235.0, 234.5, 234.0, 233.5, 233.0,
    232.5, 232.0, 231.5, 231.0, 230.5, 230.0, 229.5, 229.0, 228.5, 228.0,
    244.9, 244.6, 244.3, 244.0, 243.7, 243.4, 243.1, 242.8, 242.5, 242.2,
    241.9, 241.6, 241.3, 241.0, 240.7, 240.4, 240.1, 240.0, 240.1, 240.2,
    240.3, 240.4, 240.5, 240.6, 240.7, 240.8, 240.9, 241.0, 241.1, 241.2,
    310.4, 300.3, 290.8, 280.6, 270.9, 265.4, 260.1, 255.8, 254.5, 254.2,
    241.3, 241.4, 241.5, 241.6, 241.7, 241.8, 241.9, 242.0, 242.1, 242.2,
    242.3, 242.4, 242.5, 242.6, 242.7, 242.8, 242.9, 243.0, 243.1, 243.2,
    243.3, 243.4, 243.5, 243.6, 243.7, 243.8, 243.9, 244.0, 244.1, 244.2,
    244.3, 244.4, 244.5, 244.6, 244.7, 244.8, 244.9, 245.0, 245.1, 245.2
};
#endif


//Table for data point records
#define TEXT_DATAPOINT_X_POS_START          PDF_PAGE_LEFT + 2
#define TEXT_DATAPOINT_Y_POS_START          750
#define TEXT_DATAPOINT_FONT_SIZE            5
#define CHILD_TABLE_SIZE                    99
#define CHILD_TABLE_NUMS                    5
#define DATAPOINTS_PER_CHILD_TABLE          96
//480 data points/page
#define DATAPOINTS_PER_PAGE                 (DATAPOINTS_PER_CHILD_TABLE * CHILD_TABLE_NUMS) 
#define DATAPOINTS_Y_MARGIN                 7

/* Export variables -----------------------------------------------*/
// Use
// extern log_csv_t log_csv;


/* Private variables -----------------------------------------------*/
FIL pdf_file;
char * pdf_file_name = PDF_LOG_FILE_NAME;

// Global to track the current byte offset in the file
ULONG           current_byte_offset = 0;

//For tracking pdf doc.
// FIXME: Due to enter shutdown mode, these variable will be loss
// Write it into flash memory?
static pdf_doc_t pdf_monitor;

/* Prototype -----------------------------------------------*/
/**
  * @brief  Helper function to write a string to the file and update the byte offset.
  * @param  file_ptr Pointer to the FIL object.
  * @param  text The null-terminated string to write.
  * @retval FX_SUCCESS or error code.
  */
//FIXME: unnecessary define
#define fatfs_write_string(file_ptr,text)      f_puts(text,file_ptr)

//================================================================================
// PDF DRAWING HELPER FUNCTIONS
//================================================================================
static int draw_title_background(FIL* file_ptr, int y_pos);
static int draw_section_title(FIL* file_ptr, int y_pos, const char* title);
static int draw_key_value_pair(FIL* file_ptr, int x_pos, int y_pos, const char* key, const char* value);

//================================================================================
// FUNCTIONS TO DRAW EACH PDF SECTION
//================================================================================
static int pdf_device_info(FIL* file_ptr, int *y_pos,const DeviceSettings *dev_setting);
static int pdf_trip_info(FIL* file_ptr, int *y_pos,const DeviceSettings *dev_setting);
static int pdf_config_info(FIL* file_ptr, int *y_pos,const DeviceSettings *dev_setting);
static int pdf_alarm_row(FIL* file_ptr, int y_pos, const alarm_zone_info_t* data, int alarm_pos);
static int pdf_alarm_table(FIL* file_ptr, int *y_pos, const DeviceSettings *dev_setting, const logging_summary_t *log);
static int pdf_logging_summary(FIL* file_ptr, int *y_pos,const logging_summary_t *log);
static void pdf_gen_chart(FIL* file_p, int *y_pos, const logging_summary_t *log,
                                        const DeviceSettings *dev_setting);


//================================================================================
// PDF GENERATION CORE FUNCTIONS
//================================================================================
static void pdf_write_page_structure(void);
static void pdf_write_footer_and_xref(void);
static void pdf_table_start_stop_time_header_obj_6(const logging_summary_t *log);
static void pdf_table_format_obj_8(void);
static inline void convert_record_num_to_x_y_position(const UINT record_num, UINT *x, UINT* y);
static int pdf_insert_datapoint_to_table(const UINT record_num,
                                         const file_log_time_t *time,
                                         float temperature,
                                         float radiation,
                                         const DeviceSettings *dev_setting);


static void generate_pdf_raw_report(void);
static void generate_pdf_report_1st_page(const logging_summary_t *log,
                                        const DeviceSettings *dev_setting);
static void generate_pdf_report_2nd_page(void);

// Close before stream, start new stream.
static void pdf_page_handle_for_new_datapoints(const UINT current_page);

//================================================================================
// EXPORT FUNCTIONS
//================================================================================

// Func create fx file and call generate_pdf_raw_report()
UINT pdf_gen_template_file(void) {

    FRESULT res = FR_OK;
    FILINFO fno;

    // Check the file existance:
    res = f_stat (pdf_file_name,&fno);
    switch (res)
    {
    case FR_OK:
        // File exists. Open it to append data.
        LOG_APP("File '%s' exists \r\n", pdf_file_name);
        break;

    case FR_NO_FILE:
        // Create new file and generate template
        // File does not exist. Create it and write the header first.
        LOG_APP("File '%s' not found. Creating new file with header.\n", pdf_file_name);
        res = f_open(&pdf_file, pdf_file_name, FA_CREATE_NEW | FA_WRITE);
        if (res != FR_OK)
        {
            LOG_APP("Open PDF log file failed,Unmount...\r\n");
            f_mount(NULL, "", 0);
            return res;
        }
        //FIXME: pre-allocated here.
        generate_pdf_raw_report();

        //FIXME: should close file here?
        LOG_APP("Closing file, Flush...\r\n");
        res = f_close(&pdf_file);
        if (res != FR_OK)
        {
            LOG_APP("f_close pdf file failed, ret=%d\r\n",res);
        }
        LOG_APP("SUCCESS: Created template %s file\r\n",pdf_file_name);

        break;

    default:
        LOG_APP("f_stat unknown error, ret:%02d\r\n",res);
        break;
    }

    return res;
}




void pdf_append_new_record(const UINT record_num,
                            const file_log_time_t *time,
                           float temperature,
                           float radiation) {
    FRESULT ret;

	LOG_APP("[%d] Inserting to PDF, pdf size: %d\r\n",record_num,(int)f_size(&pdf_file));
    UINT current_page = (UINT) (record_num / DATAPOINTS_PER_PAGE);
    UINT idx_offset = record_num % DATAPOINTS_PER_PAGE;

    ret = f_open(&pdf_file, pdf_file_name, FA_OPEN_APPEND | FA_WRITE);
    if (ret != FR_OK)
    {
        LOG_APP("FATAL: f_open pdf_file FAILED with status: 0x%02X\r\n", ret);
        return ;
    }

    ret = f_lseek(&pdf_file, f_size(&pdf_file));
    if (ret != FR_OK) {
        LOG_APP(" PDF: f_lseek failed, ret: %d\r\n",ret);
        return;
    }

    if (pdf_insert_datapoint_to_table(record_num, time, temperature, radiation, &current_settings) < 0) {
        LOG_APP("PDF: insert new data point failed\r\n");
        return;
    }
    // Is it last data points of the page?
    if (idx_offset == ( DATAPOINTS_PER_PAGE-1 )) {
        // end stream and create new obj page stream:
        pdf_page_handle_for_new_datapoints(current_page);
    }
    ret = f_close(&pdf_file);
    if (ret != FR_OK)
    {
        LOG_APP("FATAL: f_close pdf_file FAILED with status: 0x%02X\r\n", ret);
        return ;
    }
}

//void pdf_append_all_flash_log_entries(void)
//{
//    uint32_t max_idx = LOG_MAX_SIZE / ENTRY_SIZE; // 로그 최대 인덱스 계산
//    log_entry_t entry;
//
//    for (uint32_t i = 0; i < max_idx; i++) {
//        meas_data_log_read_entry(i, &entry);
//
//        if (entry.year == 0xFF || entry.index == 0xFFFF || entry.month == 0xFF)
//            break;
//
//        file_log_time_t time = {
//            .year   = entry.year,
//            .month  = entry.month,
//            .day    = entry.day,
//            .hour   = entry.hour,
//            .minute = entry.minute,
//            .second = entry.second
//        };
//
//        float temp = entry.temperature / 10.0f;
//        float dose = entry.dose / 100.0f;
//        float rad_for_print;
//
//        if (entry.rad_measure_mark == 0) {
//            rad_for_print = -1.0f;
//        } else {
//            rad_for_print = dose;
//        }
//
//        pdf_append_new_record(i, &time, temp, rad_for_print);
//    }
//}
void pdf_append_all_flash_log_entries(void)
{
    uint32_t max_idx = LOG_MAX_SIZE / ENTRY_SIZE;
    log_entry_t entry;

    // 1) 파일 한 번만 오픈 + 끝으로 이동
    if (f_open(&pdf_file, pdf_file_name, FA_OPEN_APPEND | FA_WRITE) != FR_OK) {
        LOG_APP("FATAL: f_open pdf_file FAILED\r\n");
        return;
    }
    f_lseek(&pdf_file, f_size(&pdf_file));

    for (uint32_t i = 0; i < max_idx; i++) {
        meas_data_log_read_entry(i, &entry);
        if (entry.year == 0xFF || entry.index == 0xFFFF || entry.month == 0xFF)
            break;

        file_log_time_t time = {
            .year   = entry.year,
            .month  = entry.month,
            .day    = entry.day,
            .hour   = entry.hour,
            .minute = entry.minute,
            .second = entry.second
        };

        float temp = entry.temperature / 10.0f;
        float dose = entry.dose / 100.0f;
        float rad_for_print = (entry.rad_measure_mark == 0) ? -1.0f : dose;

        // 2) 여기서 바로 쓰기 (열기/닫기 없음)
        if (pdf_insert_datapoint_to_table(i, &time, temp, rad_for_print, &current_settings) < 0) {
            LOG_APP("PDF: insert failed at %lu\r\n", (unsigned long)i);
            break;
        }

        // 3) 페이지 경계에서만 스트림 전환
        if ((i % DATAPOINTS_PER_PAGE) == (DATAPOINTS_PER_PAGE - 1)) {
            UINT current_page = (UINT)(i / DATAPOINTS_PER_PAGE);
            pdf_page_handle_for_new_datapoints(current_page);
        }

        // (선택) 너무 잦은 로그는 속도저하 → 간격 두고 찍기
        if ((i & 0xFF) == 0) {
            LOG_APP("PDF append progress: %lu\r\n", (unsigned long)i);
        }
    }

    f_close(&pdf_file);
}



void pdf_gen_completed_report(const logging_summary_t *log,
                            const DeviceSettings *dev_setting) {
	const logging_summary_t *summary = csv_get_logging_summary();
	printf("[DBG][PDF] temp_pts=%u, per_page=%u -> total_pages=%u(+1st)\r\n",
	       (unsigned)log->data_points_temp_count, (unsigned)DATAPOINTS_PER_PAGE,
	       (unsigned)(log->data_points_temp_count / DATAPOINTS_PER_PAGE + 1));
	printf("Report PDF File\r\n");
    // end current obj
    UINT current_page = (log->data_points_temp_count) / DATAPOINTS_PER_PAGE;
    UINT current_obj = TPDF_OBJ_NUM_PAGE_2ND + (current_page *10);
    const UINT total_pages = current_page +1;
    UINT page_obj = 0;

    if (f_open(&pdf_file, pdf_file_name, FA_OPEN_APPEND | FA_WRITE) != FR_OK)
    {
        LOG_APP("FATAL: f_open pdf FAILED\r\n");
        return ;
    }
    // End of stream of previous obj page
    // FIXME: how to get stream length of current page?
    tpdf_end_new_stream_obj(&pdf_file,current_obj + 4, 1234 ,&pdf_monitor );

    // update header start stop time
    pdf_table_start_stop_time_header_obj_6(summary);

    // Create 1st page
    generate_pdf_report_1st_page(log,dev_setting);

    // update kids page
    char content_buffer[512];
    char temp_ref[64];

    //First page.
    sprintf(content_buffer, "<<\n/Type /Pages\n/Count %d\n/Kids [ %d 0 R", total_pages +1 ,TPDF_OBJ_NUM_PAGE_1ST);

    // Loop through all page object IDs and append them directly to the content_buffer.
    for (int i = 0; i < total_pages; i++) {
        page_obj = TPDF_OBJ_NUM_PAGE_2ND + i*10;
        sprintf(temp_ref, " %d 0 R", page_obj);
        strcat(content_buffer, temp_ref);
    }
    // Append the closing bracket for the /Kids array and the closing dictionary brackets.
    sprintf(temp_ref,"]\n/MediaBox [0 0 %d %d]\n>>",PDF_PAGE_WIDTH_A4,PDF_PAGE_HEIGHT_A4);

    strcat(content_buffer, temp_ref);

    // Write the newly created object to the PDF file.
    tpdf_add_new_obj(&pdf_file, TPDF_OBJ_PAGES_KID, content_buffer, &pdf_monitor);

    // update xref.
    //number of obj?
    pdf_write_footer_and_xref();

    if (f_chmod(pdf_file_name,AM_RDO,AM_RDO) != FR_OK)
    {
        LOG_APP("f_chmod %s failed\r\n",pdf_file_name);
    }
    f_close(&pdf_file);
    LOG_APP("Close PDF log file\r\n");
}


//================================================================================
// PRIVATE FUNCTIONS DEFINITION
//================================================================================
static int draw_title_background(FIL* file_ptr, int y_pos) {
    char temp_buffer[64];
    //FIXME: HARD CODE??
    sprintf(temp_buffer, "q\n0.9 0.9 0.9 rg\n50 %d 495 15 re\nf\nQ\n", y_pos);

    if (file_ptr) {
        fatfs_write_string(file_ptr, temp_buffer);
        return 0;
    }
    return strlen(temp_buffer);
}

static int draw_section_title(FIL* file_ptr, int y_pos, const char* title) {
    int len = draw_title_background(file_ptr, y_pos - 12);
    // y_pos - 8
    len += tpdf_draw_colored_text(file_ptr,TEXT_START,y_pos-8,"F2",SECTION_TITLE_FONT_SIZE,0,0,0,title);
    return len;
}

//static int draw_key_value_pair(FIL* file_ptr, int x_pos, int y_pos, const char* key, const char* value) {
//    char temp_buffer[256];
//    // Draw the key using the bold font (/F1)
//    // snprintf(buff, size
//    sprintf(temp_buffer, "BT /F2 %d Tf 0 0 0 rg %d %d Td (%s) Tj ET\n",SECTION_CONTENT_FONT_SIZE, x_pos, y_pos, key);
//    int len = strlen(temp_buffer);
//    if(file_ptr) fatfs_write_string(file_ptr, temp_buffer);
//
//    // Draw the value using the regular font (/F2)
//    sprintf(temp_buffer, "BT /F1 %d Tf 0 0 0 rg %d %d Td (%s) Tj ET\n",SECTION_CONTENT_FONT_SIZE, x_pos + 125, y_pos, value);
//    len += strlen(temp_buffer);
//    if(file_ptr) fatfs_write_string(file_ptr, temp_buffer);
//
//    return file_ptr ? 0 : len;
//}

static int draw_key_value_pair(FIL* file_ptr, int x_pos, int y_pos, const char* key, const char* value) {
    char temp_buffer[256];

    // Draw the key using the bold font (/F1)
    int written = snprintf(
        temp_buffer, sizeof(temp_buffer),
        "BT /F2 %d Tf 0 0 0 rg %d %d Td (%s) Tj ET\n",
        SECTION_CONTENT_FONT_SIZE, x_pos, y_pos, key
    );
    if (written < 0 || written >= (int)sizeof(temp_buffer)) {
        // 에러 처리: 출력이 버퍼를 초과함
        return -1;
    }
    int len = written;
    if (file_ptr) fatfs_write_string(file_ptr, temp_buffer);

    // Draw the value using the regular font (/F2)
    written = snprintf(
        temp_buffer, sizeof(temp_buffer),
        "BT /F1 %d Tf 0 0 0 rg %d %d Td (%s) Tj ET\n",
        SECTION_CONTENT_FONT_SIZE, x_pos + 125, y_pos, value
    );
    if (written < 0 || written >= (int)sizeof(temp_buffer)) {
        return -1;
    }
    len += written;
    if (file_ptr) fatfs_write_string(file_ptr, temp_buffer);

    return file_ptr ? 0 : len;
}

static int pdf_device_info(FIL* file_ptr, int *y_pos, const DeviceSettings *dev_setting) {
    int len = draw_section_title(file_ptr, *y_pos, "Device Information");
    const char *probe_type_str;

    *y_pos -= FIELD_SPACING;
    len += draw_key_value_pair(file_ptr, TEXT_START, *y_pos, "Device Code :", dev_setting->device_code);
	if (dev_setting->sensor_type == 1)
		probe_type_str = "Temperature and Radiation (interval)";
	else if (dev_setting->sensor_type == 2)
		probe_type_str = "Temperature Only";
	else
		probe_type_str = "Unknown Sensor Type";
    len += draw_key_value_pair(file_ptr, TEXT_SECOND_COL_START, *y_pos, "Probe Type :", probe_type_str);
    *y_pos -= LINE_SPACING;

    len += draw_key_value_pair(file_ptr, TEXT_START, *y_pos, "Serial Number :", dev_setting->serial);
    len += draw_key_value_pair(file_ptr, TEXT_SECOND_COL_START, *y_pos, "Firmware Version :", dev_setting->firmware_ver);
    *y_pos -= LINE_SPACING;
    len += draw_key_value_pair(file_ptr, TEXT_START, *y_pos, "Mode Code :", dev_setting->model);
    *y_pos -= FIELD_SPACING;
    return len;
}

static int pdf_trip_info(FIL* file_ptr, int *y_pos, const DeviceSettings *dev_setting) {
    int len = draw_section_title(file_ptr, *y_pos, "Trip Information");
    *y_pos -= FIELD_SPACING;
    char tmp_buff[16];
    snprintf(tmp_buff,sizeof(tmp_buff),"%7u",dev_setting->trip_code);

    len += draw_key_value_pair(file_ptr, TEXT_START, *y_pos, "Trip Id :", tmp_buff);
    *y_pos -= LINE_SPACING;
    len += draw_key_value_pair(file_ptr, TEXT_START, *y_pos, "Description :", dev_setting->trip_desc);
    *y_pos -= FIELD_SPACING;
    return len;
}

//FIXME: start mode, stop mode, time based?
static int pdf_config_info(FIL* file_ptr, int *y_pos,const DeviceSettings *dev_setting) {
    char tmp[16];
    const char* start_mode_str;
    switch (dev_setting->start_mode) {
        case 0x00:
            start_mode_str = "Delay";
            break;
        case 0x01:
            start_mode_str = "Manual";
            break;
        case 0x02:
            start_mode_str = "Software";
            break;
        default:
            start_mode_str = "Unknown";
            break;
    }
    int len = draw_section_title(file_ptr, *y_pos, "Configuration Information");
    *y_pos -= FIELD_SPACING;
    len += draw_key_value_pair(file_ptr, TEXT_START, *y_pos, "Start Mode :",start_mode_str );

    seconds_to_hms_string(dev_setting ->temp_interval,tmp,sizeof(tmp));
    len += draw_key_value_pair(file_ptr, TEXT_SECOND_COL_START, *y_pos, "Temperature Log Interval :", tmp);
    *y_pos -= LINE_SPACING;

    seconds_to_hms_string(dev_setting ->start_delay,tmp,sizeof(tmp));
    len += draw_key_value_pair(file_ptr, TEXT_START, *y_pos, "Start Delay :", tmp);

    seconds_to_hms_string(dev_setting ->rad_interval,tmp,sizeof(tmp));
    len += draw_key_value_pair(file_ptr, TEXT_SECOND_COL_START, *y_pos, "Radiation Log Interval :", tmp);
    *y_pos -= LINE_SPACING;

    //FIXME: Is start/stop mode and Time based info fixed?
//    len += draw_key_value_pair(file_ptr, TEXT_SECOND_COL_START, *y_pos, "Stop Mode :", "Manual + Software");
//    *y_pos -= LINE_SPACING;

//    len += draw_key_value_pair(file_ptr, TEXT_START, *y_pos, "Time Base :", "UTC +00:00");
//    *y_pos -= FIELD_SPACING;

    len += draw_key_value_pair(file_ptr, TEXT_SECOND_COL_START, *y_pos, "Stop Mode :", "Manual + Software");
    *y_pos -= FIELD_SPACING;
    return len;
}

// FIXME: should hardcode the position and size?
//static int pdf_alarm_row(FIL* file_ptr, int y_pos, const alarm_zone_info_t* data) {
//    char temp_buffer[128];
//    char row_commands[512] = "";
//    char tmp_time_buff[32];
//
//    // Set text color to red if it's an alarm, otherwise black.
//    if (data->zone_name[0] == 'T') {
//        // 온도 알람 (TH, TL) → 부호 있는 정수 출력
//        sprintf(temp_buffer,
//                "BT %d 0 0 rg /F1 %d Tf 55 %d Td (%s %d (C)) Tj ET\n",
//                data->status, SECTION_CONTENT_FONT_SIZE, y_pos,
//                data->zone_name, (int16_t)data->threshold);
//    } else {
//        // 방사선 알람 (RH) → 부호 없는 정수 출력
//        sprintf(temp_buffer,
//                "BT %d 0 0 rg /F1 %d Tf 55 %d Td (%s %u (uSv/h)) Tj ET\n",
//                data->status, SECTION_CONTENT_FONT_SIZE, y_pos,
//                data->zone_name, (uint16_t)data->threshold);
//    }
//
//    strcat(row_commands, temp_buffer);
//
//    // Draw the rest of the columns in black
//    //FIXME: Allow time?
//    // Delay Time (초 -> DHMS)  ✅ dev_setting 접근 불가, data의 3번째 필드를 그대로 사용
//    seconds_to_dhms_string_long(data->alarm_delay_sec, tmp_time_buff, sizeof(tmp_time_buff));
//    sprintf(temp_buffer, "BT %d 0 0 rg /F1 %d Tf 155 %d Td (%s) Tj ET\n",
//            data->status, SECTION_CONTENT_FONT_SIZE, y_pos, tmp_time_buff);
//    strcat(row_commands, temp_buffer);
//
//    // Alarm Type
//    sprintf(temp_buffer, "BT %d 0 0 rg /F1 %d Tf 245 %d Td (%s) Tj ET\n",
//            data->status, SECTION_CONTENT_FONT_SIZE, y_pos, data->delay_type);
//    strcat(row_commands, temp_buffer);
//
//    // Total Time (초 -> DHMS)
//    seconds_to_dhms_string_long(data->total_time_in_violation_minutes, tmp_time_buff, sizeof(tmp_time_buff));
//    sprintf(temp_buffer, "BT %d 0 0 rg /F1 %d Tf 325 %d Td (%s) Tj ET\n",
//            data->status, SECTION_CONTENT_FONT_SIZE, y_pos, tmp_time_buff);
//    strcat(row_commands, temp_buffer);
//
//    sprintf(temp_buffer, "BT %d 0 0 rg /F1 %d Tf 435 %d Td (%d) Tj ET\n",data->status,SECTION_CONTENT_FONT_SIZE, y_pos, data->violation_count);
//    strcat(row_commands, temp_buffer);
//
//    const char *alr_status = (data->status == ALARM_STATUS_ALARM) ? "ALARM" : "OK";
//    sprintf(temp_buffer, "BT %d 0 0 rg /F1 %d Tf 500 %d Td (%s) Tj ET\n",data->status,SECTION_CONTENT_FONT_SIZE, y_pos, alr_status);
//    strcat(row_commands, temp_buffer);
//
//    if (file_ptr) {
//        fatfs_write_string(file_ptr, row_commands);
//        return 0;
//    }
//    return strlen(row_commands);
//}

#define ALARM_DISABLED(pos) (GET_ALARM_STATE(device_config.alarm_state, (pos)) == ALARM_DISABLE)

static int pdf_alarm_row(FIL* file_ptr,
                         int y_pos,
                         const alarm_zone_info_t* data,
                         int alarm_pos)
{
    char temp_buffer[128];
    char row_commands[512] = "";
    char tmp_time_buff[32];

    // 이 존의 현재 상태 확인(OFF/ON/DISABLE)
    const uint8_t state = GET_ALARM_STATE(device_config.alarm_state, alarm_pos);
    const int disabled = (state == ALARM_DISABLE);

    if (disabled) {
        // ── DISABLE: 이 줄(Alarm Zone/Delay/Type/Total/Violations/Status) 전부 공백 출력
        // Alarm Zone
        sprintf(temp_buffer, "BT 0 0 0 rg /F1 %d Tf 55  %d Td () Tj ET\n",
                SECTION_CONTENT_FONT_SIZE, y_pos);
        strcat(row_commands, temp_buffer);
        // Delay
        sprintf(temp_buffer, "BT 0 0 0 rg /F1 %d Tf 155 %d Td () Tj ET\n",
                SECTION_CONTENT_FONT_SIZE, y_pos);
        strcat(row_commands, temp_buffer);
        // Type
        sprintf(temp_buffer, "BT 0 0 0 rg /F1 %d Tf 245 %d Td () Tj ET\n",
                SECTION_CONTENT_FONT_SIZE, y_pos);
        strcat(row_commands, temp_buffer);
        // Total
        sprintf(temp_buffer, "BT 0 0 0 rg /F1 %d Tf 325 %d Td () Tj ET\n",
                SECTION_CONTENT_FONT_SIZE, y_pos);
        strcat(row_commands, temp_buffer);
        // Violations
        sprintf(temp_buffer, "BT 0 0 0 rg /F1 %d Tf 435 %d Td () Tj ET\n",
                SECTION_CONTENT_FONT_SIZE, y_pos);
        strcat(row_commands, temp_buffer);
        // Status
        sprintf(temp_buffer, "BT 0 0 0 rg /F1 %d Tf 500 %d Td () Tj ET\n",
                SECTION_CONTENT_FONT_SIZE, y_pos);
        strcat(row_commands, temp_buffer);

        if (file_ptr) { fatfs_write_string(file_ptr, row_commands); return 0; }
        return (int)strlen(row_commands);
    }

    // ── ON/OFF: 기존처럼 텍스트 출력 ──────────────────────────────────────────────
    // Alarm Zone 칼럼 (T*는 °C, R*는 uSv/h)
    if (data->zone_name[0] == 'T') {
        // threshold는 섭씨 기준으로 들어옴(예: th1/10)
        float t = (float)(int16_t)data->threshold;
        if (current_settings.display_temp_unit == 1) {
            t = t*9.0f/5.0f + 32.0f;
            sprintf(temp_buffer,
                "BT %d 0 0 rg /F1 %d Tf 55 %d Td (%s %.1f (F)) Tj ET\n",
                data->status, SECTION_CONTENT_FONT_SIZE, y_pos,
                data->zone_name, t);
        } else {
            sprintf(temp_buffer,
                "BT %d 0 0 rg /F1 %d Tf 55 %d Td (%s %.1f (C)) Tj ET\n",
                data->status, SECTION_CONTENT_FONT_SIZE, y_pos,
                data->zone_name, t);
        }
    } else {
        // RH 임계값은 uSv/h 기준(raw: rh/100)
        float r_usvh = (float)(uint16_t)data->threshold;
        if (current_settings.display_dose_unit == 1) {
            // uR/h: 정수 출력
            unsigned ru = (unsigned)(r_usvh * 100.0f + 0.5f);
            sprintf(temp_buffer,
                "BT %d 0 0 rg /F1 %d Tf 55 %d Td (%s %u (uR/h)) Tj ET\n",
                data->status, SECTION_CONTENT_FONT_SIZE, y_pos,
                data->zone_name, ru);
        } else {
            // uSv/h: 기존처럼 소수
            sprintf(temp_buffer,
                "BT %d 0 0 rg /F1 %d Tf 55 %d Td (%s %.1f (uSv/h)) Tj ET\n",
                data->status, SECTION_CONTENT_FONT_SIZE, y_pos,
                data->zone_name, r_usvh);
        }
    }


    strcat(row_commands, temp_buffer);

    // Delay Time
    seconds_to_dhms_string_long(data->alarm_delay_sec, tmp_time_buff, sizeof(tmp_time_buff));
    sprintf(temp_buffer,
            "BT %d 0 0 rg /F1 %d Tf 155 %d Td (%s) Tj ET\n",
            data->status, SECTION_CONTENT_FONT_SIZE, y_pos, tmp_time_buff);
    strcat(row_commands, temp_buffer);

    // Alarm Type
    sprintf(temp_buffer,
            "BT 0 0 0 rg /F1 %d Tf 245 %d Td (%s) Tj ET\n",
            SECTION_CONTENT_FONT_SIZE, y_pos, data->delay_type);
    strcat(row_commands, temp_buffer);

    // Total Time in Violation
    seconds_to_dhms_string_long(data->total_time_in_violation_minutes, tmp_time_buff, sizeof(tmp_time_buff));
    sprintf(temp_buffer,
            "BT %d 0 0 rg /F1 %d Tf 325 %d Td (%s) Tj ET\n",
            data->status, SECTION_CONTENT_FONT_SIZE, y_pos, tmp_time_buff);
    strcat(row_commands, temp_buffer);

    // Violations
    sprintf(temp_buffer,
            "BT %d 0 0 rg /F1 %d Tf 435 %d Td (%u) Tj ET\n",
            data->status, SECTION_CONTENT_FONT_SIZE, y_pos, (unsigned)data->violation_count);
    strcat(row_commands, temp_buffer);

    // Status
    {
        const char *alr_status = (data->status == ALARM_STATUS_ALARM) ? "ALARM" : "OK";
        sprintf(temp_buffer,
                "BT %d 0 0 rg /F1 %d Tf 500 %d Td (%s) Tj ET\n",
                data->status, SECTION_CONTENT_FONT_SIZE, y_pos, alr_status);
        strcat(row_commands, temp_buffer);
    }

    if (file_ptr) { fatfs_write_string(file_ptr, row_commands); return 0; }
    return (int)strlen(row_commands);
}





static int pdf_alarm_table(FIL* file_ptr, int *y_pos, const DeviceSettings *dev_setting, const logging_summary_t *log) {
    int len = draw_section_title(file_ptr, *y_pos, "");
    *y_pos -= 8; // Adjust Y position after title

    // CORRECTED: Each header item is now in its own text object (BT/ET) for correct positioning.
    len += tpdf_draw_colored_text(file_ptr,TEXT_START,*y_pos,"F2",SECTION_TITLE_FONT_SIZE,0,0,0,"Alarm Zone");

    len += tpdf_draw_colored_text(file_ptr,155,*y_pos,"F2",SECTION_TITLE_FONT_SIZE,0,0,0,"Delay Time");

    len += tpdf_draw_colored_text(file_ptr,245,*y_pos,"F2",SECTION_TITLE_FONT_SIZE,0,0,0,"Alarm Type");

    len += tpdf_draw_colored_text(file_ptr,325,*y_pos,"F2",SECTION_TITLE_FONT_SIZE,0,0,0,"Total Time");

    len += tpdf_draw_colored_text(file_ptr,435,*y_pos,"F2",SECTION_TITLE_FONT_SIZE,0,0,0,"Violations");

    len += tpdf_draw_colored_text(file_ptr,500,*y_pos,"F2",SECTION_TITLE_FONT_SIZE,0,0,0,"Status");

    *y_pos -= LINE_SPACING;
    //FIXME: lack of: total time, violation, status?
    // alarm_delay_* 가 "초" 단위라고 가정 (필요시 *60UL 하세요)
    unsigned long rh1_total = (log->elapsed_time_sec > dev_setting->alarm_delay_rh1)
                            ? (log->elapsed_time_sec - dev_setting->alarm_delay_rh1) : 0UL;
    unsigned long rh2_total = (log->elapsed_time_sec > dev_setting->alarm_delay_rh2)
                            ? (log->elapsed_time_sec - dev_setting->alarm_delay_rh2) : 0UL;
    unsigned long th1_total = (log->elapsed_time_sec > dev_setting->alarm_delay_th1)
                            ? (log->elapsed_time_sec - dev_setting->alarm_delay_th1) : 0UL;
    unsigned long th2_total = (log->elapsed_time_sec > dev_setting->alarm_delay_th2)
                            ? (log->elapsed_time_sec - dev_setting->alarm_delay_th2) : 0UL;
    unsigned long tl1_total = (log->elapsed_time_sec > dev_setting->alarm_delay_tl1)
                            ? (log->elapsed_time_sec - dev_setting->alarm_delay_tl1) : 0UL;
    unsigned long tl2_total = (log->elapsed_time_sec > dev_setting->alarm_delay_tl2)
                            ? (log->elapsed_time_sec - dev_setting->alarm_delay_tl2) : 0UL;

    const uint16_t c_rh1 = log->RH1_alarm_count;
    const uint16_t c_rh2 = log->RH2_alarm_count;
    const uint16_t c_th1 = log->TH1_alarm_count;
    const uint16_t c_th2 = log->TH2_alarm_count;
    const uint16_t c_tl1 = log->TL1_alarm_count;
    const uint16_t c_tl2 = log->TL2_alarm_count;

    alarm_zone_info_t tmp_alrm[6] = {
        // 1) 5번째=총시간(rh*_total), 2) 6번째=Count(c_*), 3) 7번째=0/1
        {"RH1: over",  dev_setting->alarm_rh1/100, dev_setting->alarm_delay_rh1, "Sin", rh1_total, c_rh1, (c_rh1 ? 1 : 0)},
        {"RH2: over",  dev_setting->alarm_rh2/100, dev_setting->alarm_delay_rh2, "Sin", rh2_total, c_rh2, (c_rh2 ? 1 : 0)},
        {"TH1: over",  dev_setting->alarm_th1/10,  dev_setting->alarm_delay_th1, "Sin", th1_total, c_th1, (c_th1 ? 1 : 0)},
        {"TH2: over",  dev_setting->alarm_th2/10,  dev_setting->alarm_delay_th2, "Sin", th2_total, c_th2, (c_th2 ? 1 : 0)},
        {"TL1: below", dev_setting->alarm_tl1/10,  dev_setting->alarm_delay_tl1, "Sin", tl1_total, c_tl1, (c_tl1 ? 1 : 0)},
        {"TL2: below", dev_setting->alarm_tl2/10,  dev_setting->alarm_delay_tl2, "Sin", tl2_total, c_tl2, (c_tl2 ? 1 : 0)},
    };


    // --- Draw Each Row ---
    len += pdf_alarm_row(file_ptr, *y_pos, &tmp_alrm[0], ALARM_STATE_POS_RH1);  *y_pos -= LINE_SPACING;
    len += pdf_alarm_row(file_ptr, *y_pos, &tmp_alrm[1], ALARM_STATE_POS_RH2);  *y_pos -= LINE_SPACING;
    len += pdf_alarm_row(file_ptr, *y_pos, &tmp_alrm[2], ALARM_STATE_POS_TH1);  *y_pos -= LINE_SPACING;
    len += pdf_alarm_row(file_ptr, *y_pos, &tmp_alrm[3], ALARM_STATE_POS_TH2);  *y_pos -= LINE_SPACING;
    len += pdf_alarm_row(file_ptr, *y_pos, &tmp_alrm[4], ALARM_STATE_POS_TL1);  *y_pos -= LINE_SPACING;
    len += pdf_alarm_row(file_ptr, *y_pos, &tmp_alrm[5], ALARM_STATE_POS_TL2);  *y_pos -= LINE_SPACING;

    *y_pos -= FIELD_SPACING;
    return len;
}

static int pdf_logging_summary(FIL* file_ptr, int *y_pos,const logging_summary_t *log) {
    int len = draw_section_title(file_ptr, *y_pos, "Logging Summary");
    int initial_y = *y_pos-FIELD_SPACING; // Y position after title
    int current_y = initial_y;
    char tmp_buff[64];

    // Draw left column
    // ---- display-unit conversion ----
    const int use_f = (current_settings.display_temp_unit == 1);
    const int use_uR = (current_settings.display_dose_unit == 1);
    const char *T = use_f ? "F" : "C";
    const char *R = use_uR ? "uR/h" : "uSv/h";

    float hiT = log->highest_temp;
    float loT = log->lowest_temp;
    float avT = log->average_temp;
    float mkt = log->mean_kinetic_temp;
    float hiR = log->highest_radiation;
    float avR = log->average_radiation;

    if (use_f) {
        hiT = hiT * 9.0f/5.0f + 32.0f;
        loT = loT * 9.0f/5.0f + 32.0f;
        avT = avT * 9.0f/5.0f + 32.0f;
        mkt = mkt * 9.0f/5.0f + 32.0f;
    }
    if (use_uR) {
        hiR *= 100.0f;
        avR *= 100.0f;
    }

    // Draw left column with units
    snprintf(tmp_buff,sizeof(tmp_buff),"%.1f (%s)",hiT,T);
    len += draw_key_value_pair(file_ptr, 55, current_y, "Highest Temperature :", tmp_buff);
    current_y -= LINE_SPACING;

    snprintf(tmp_buff,sizeof(tmp_buff),"%.1f (%s)",loT,T);
    len += draw_key_value_pair(file_ptr, 55, current_y, "Lowest Temperature :", tmp_buff);
    current_y -= LINE_SPACING;

    snprintf(tmp_buff,sizeof(tmp_buff),"%.1f (%s)",avT,T);
    len += draw_key_value_pair(file_ptr, 55, current_y, "Average Temperature :", tmp_buff);
    current_y -= LINE_SPACING;

    snprintf(tmp_buff,sizeof(tmp_buff),"%.1f (%s)",mkt,T);
    len += draw_key_value_pair(file_ptr, 55, current_y, "MKT :", tmp_buff);
    current_y -= LINE_SPACING;

    if (use_uR) {
        snprintf(tmp_buff,sizeof(tmp_buff),"%u (%s)", (unsigned)(hiR + 0.5f), R);
    } else {
        snprintf(tmp_buff,sizeof(tmp_buff),"%.2f (%s)", hiR, R);
    }
    len += draw_key_value_pair(file_ptr, 55, current_y, "Highest Rad :", tmp_buff);
    current_y -= LINE_SPACING;

    if (use_uR) {
        snprintf(tmp_buff,sizeof(tmp_buff),"%u (%s)", (unsigned)(avR + 0.5f), R);
    } else {
        snprintf(tmp_buff,sizeof(tmp_buff),"%.2f (%s)", avR, R);
    }
    len += draw_key_value_pair(file_ptr, 55, current_y, "Average Rad :", tmp_buff);




    // Draw right column, resetting Y position
    current_y = initial_y;

    //FIXME: Alarm at??
    file_log_time_t tmp_alrm_trg ={.year =0, .month=1, .day =0, .hour=0, .minute =0, .second =0};
    time_to_dmyhms_string(&tmp_alrm_trg,tmp_buff,sizeof(tmp_buff));
    len += draw_key_value_pair(file_ptr, 300, current_y, "Alarm At(Te) :", tmp_buff);
    current_y -= LINE_SPACING;

    time_to_dmyhms_string(&log->start_time,tmp_buff,sizeof(tmp_buff));
    len += draw_key_value_pair(file_ptr, 300, current_y, "Start Time :", tmp_buff);
    current_y -= LINE_SPACING;

    time_to_dmyhms_string(&log->stop_time,tmp_buff,sizeof(tmp_buff));
    len += draw_key_value_pair(file_ptr, 300, current_y, "Stop Time :", tmp_buff);
    current_y -= LINE_SPACING;


    // 시·분·초를 초 단위로 변환
    uint32_t start_sec = log->start_time.hour * 3600 +
                         log->start_time.minute  * 60 +
                         log->start_time.second;

    uint32_t stop_sec  = log->stop_time.hour  * 3600 +
                         log->stop_time.minute   * 60 +
                         log->stop_time.second;

    uint32_t elapsed_sec;
    if (stop_sec >= start_sec) {
        elapsed_sec = stop_sec - start_sec;
    } else {
        elapsed_sec = (86400 - start_sec) + stop_sec;
    }

    // 여기서 elapsed_sec 사용
    seconds_to_dhms_string_long(log->elapsed_time_sec, tmp_buff, sizeof(tmp_buff));
    len += draw_key_value_pair(file_ptr, 300, current_y, "Elapsed Time :", tmp_buff);
    current_y -= LINE_SPACING;


    snprintf(tmp_buff,sizeof(tmp_buff),"%d",log->data_points_temp_count);
    len += draw_key_value_pair(file_ptr, 300, current_y, "Temperature Data Points :", tmp_buff);
    current_y -= LINE_SPACING;

    snprintf(tmp_buff,sizeof(tmp_buff),"%d",log->data_points_radiation_count);
    len += draw_key_value_pair(file_ptr, 300, current_y, "Radiation Data Points :", tmp_buff);

    current_y -= FIELD_SPACING;

    *y_pos = current_y;

    return len;
}

// Map value to x,y axes
static float map_value(float value, float from_min, float from_max, float to_min, float to_max) {
    return to_min + (value - from_min) * (to_max - to_min) / (from_max - from_min);
}

// FIXME: Chart should be scaled from TEMP_MIN to highest_temp also from RAD_MIN to highest_rad?
static void pdf_chart_draw_axes(FIL* file_p, int y_pos, const float temp_highest_scale_up, \
                                const unsigned int rad_highest_scale_up, \
                                const file_log_time_t *start_time, \
                                const file_log_time_t *stop_time)
{
    char label_buffer[20];
    const int chart_y_start = y_pos - FIELD_SPACING;

    float temper_label_interval = (float)((temp_highest_scale_up - g_temp_axis_min ) / (TEMP_LABLE_NUMS-1));
    float rad_label_interval = (float) ((rad_highest_scale_up - RAD_MIN) / (RAD_LABEL_NUMS-1));

    // --- Draw Main Chart Box ---
    tpdf_draw_line(file_p, CHART_X_START, chart_y_start, CHART_X_START + CHART_WIDTH, chart_y_start, 1.5, 0, 0, 0); // Bottom
    tpdf_draw_line(file_p, CHART_X_START, chart_y_start, CHART_X_START, chart_y_start - CHART_HEIGHT, 1.5, 0, 0, 0); // Left
    tpdf_draw_line(file_p, CHART_X_START + CHART_WIDTH, chart_y_start, CHART_X_START + CHART_WIDTH, chart_y_start - CHART_HEIGHT, 1.5, 0, 0, 0);// Right
    tpdf_draw_line(file_p, CHART_X_START, chart_y_start - CHART_HEIGHT, CHART_X_START + CHART_WIDTH, chart_y_start - CHART_HEIGHT, 1.5, 0, 0, 0); // Top

    // --- Draw Y-Axis Labels (Temperature) ---
    const char *yl_T = (current_settings.display_temp_unit==0) ? "[C]" : "[F]";
    const char *yl_R = (current_settings.display_dose_unit==0) ? "[uSv/h]" : "[uR/h]";
    tpdf_draw_colored_text(file_p, CHART_X_START - 20, chart_y_start + 15, "F1", 11, 0, 0, 0, yl_T);
    //FIXME: Is it always 4 labels for tempe?
    for (int i = 0; i < TEMP_LABLE_NUMS; i++) {
        float temp = (float) (g_temp_axis_min  + i * temper_label_interval);
        int y = (int)map_value(temp, g_temp_axis_min , temp_highest_scale_up, chart_y_start - CHART_HEIGHT, chart_y_start);
        sprintf(label_buffer, "%0.1f", temp);
        tpdf_draw_colored_text(file_p, CHART_X_START - 20, y - 3, "F1", 8, 0, 0, 0, label_buffer);
    }

    // --- Draw Y-Axis Labels (Radiation) ---
    tpdf_draw_colored_text(file_p, CHART_X_START + CHART_WIDTH + 5, chart_y_start + 15, "F1", 11, 0, 0, 0, yl_R);
    // FIXME: Is it always 6 labels for tempe?
    for (int i = 0; i < RAD_LABEL_NUMS; i++) {
        float rad = RAD_MIN + i * rad_label_interval;
        unsigned int y = (unsigned int)map_value(rad, RAD_MIN, rad_highest_scale_up, chart_y_start - CHART_HEIGHT,chart_y_start );
        sprintf(label_buffer, "%u", (unsigned int)rad);
        tpdf_draw_colored_text(file_p, CHART_X_START + CHART_WIDTH + 5, y - 3, "F1", 8, 0, 0, 0, label_buffer);
    }

    // --- Draw X-Axis Labels (Time/Date) ---
    // FIXME: Only draw start and end date?
    char tmp_time[16];
    snprintf(tmp_time,sizeof(tmp_time),"%02u-%s-%02u", \
            start_time ->day,\
            convert_month_to_string (start_time ->month),\
            start_time ->year);
    tpdf_draw_colored_text(file_p, CHART_X_START-15, chart_y_start - CHART_HEIGHT - 10, "F1", 8, 0, 0, 0, tmp_time);
    snprintf(tmp_time,sizeof(tmp_time),"%02u:%02u:%02u", \
            start_time ->hour,\
            start_time ->minute,\
            start_time ->second);
    tpdf_draw_colored_text(file_p, CHART_X_START-15, chart_y_start - CHART_HEIGHT - 18, "F1", 8, 0, 0, 0, tmp_time);

    //Stop
    snprintf(tmp_time,sizeof(tmp_time),"%02u-%s-%02u", \
            stop_time->day,\
            convert_month_to_string (stop_time->month),\
            stop_time->year);
    tpdf_draw_colored_text(file_p, CHART_X_START + CHART_WIDTH -15, chart_y_start - CHART_HEIGHT - 10, "F1", 8, 0, 0, 0, tmp_time);
    snprintf(tmp_time,sizeof(tmp_time),"%02u:%02u:%02u", \
            stop_time->hour,\
            stop_time->minute,\
            stop_time->second);
    tpdf_draw_colored_text(file_p, CHART_X_START+ CHART_WIDTH -15, chart_y_start - CHART_HEIGHT - 18, "F1", 8, 0, 0, 0, tmp_time);
}

static void pdf_chart_draw_gridlines(FIL* file_p,int y_pos, const float temp_highest_scale_up, const unsigned int rad_highest_scale_up) {
    // --- Draw Horizontal Major Gridlines (using a light gray) ---
    const int chart_y_start = y_pos - FIELD_SPACING;
    int y=0;
    float light_gray = 0.7f;
    float line_size = 0.5f;
    float temper_label_interval = (float)((temp_highest_scale_up - g_temp_axis_min ) / (TEMP_LABLE_NUMS-1));
    float rad_label_interval = (float) ((rad_highest_scale_up - RAD_MIN) / (RAD_LABEL_NUMS-1));

    // Draw square block size: left + 5 --> right -5: (240)
    // 4 line? MAX = 30 + 10 / 10 = 4
    for (int i = 1; i <= (TEMP_LABLE_NUMS -2); i++) {
        float temp = (float)(g_temp_axis_min  + i * temper_label_interval);
        y = (int)map_value(temp, g_temp_axis_min , temp_highest_scale_up,chart_y_start - CHART_HEIGHT, chart_y_start);
        tpdf_draw_simple_dashed_line(file_p, CHART_X_START, y, CHART_X_START + CHART_WIDTH, y, line_size, light_gray, light_gray, light_gray,4);
    }

    for (int i = 1; i <= (RAD_LABEL_NUMS-2); i++) {
        float rad = RAD_MIN + i * rad_label_interval;
        y = (int)map_value(rad, RAD_MIN, rad_highest_scale_up, chart_y_start - CHART_HEIGHT, chart_y_start);
        tpdf_draw_simple_dashed_line(file_p, CHART_X_START, y, CHART_X_START + CHART_WIDTH, y, line_size, light_gray, light_gray, light_gray,4);
    }
    // Height: 260: 6 line --> space: 41.6
    // int(RAD_MAX - RAD_MIN)/100
    // width: 450: 
    // Grid col line:
    const int grid_size = ( CHART_HEIGHT / ((int)(rad_highest_scale_up-RAD_MIN)/rad_label_interval) );
    const int num_grid_col_lines = (int)(CHART_WIDTH / grid_size);

    for (int i = 1; i <= num_grid_col_lines; i++)
    {
        tpdf_draw_simple_dashed_line(file_p, CHART_X_START + (i*grid_size), chart_y_start , CHART_X_START + (i*grid_size), chart_y_start - CHART_HEIGHT, line_size, light_gray, light_gray, light_gray,4);
    }
}


#define ALARM_SHOULD_DRAW(pos) (GET_ALARM_STATE(device_config.alarm_state, (pos)) != ALARM_DISABLE)

static void pdf_chart_draw_alarm_lines(FIL* file_p,
                                       int y_pos,
                                       const float temp_highest_scale_up,
                                       const float rad_highest_scale_up,
                                       const DeviceSettings *dev_setting)
{
    // 좌표 기준/버퍼 (빼면 안 됨)
    const int chart_y_start = y_pos - FIELD_SPACING;
    const int y_top    = chart_y_start - CHART_HEIGHT; // 그래프 상단(Y 최소)
    const int y_bottom = chart_y_start;                 // 그래프 하단(Y 최대)
    int y = 0;
    char label_buffer[16];

    // 표시 단위 플래그
    const int use_f  = (current_settings.display_temp_unit == 1);
    const int use_uR = (current_settings.display_dose_unit == 1);

    // 내부(raw) → 기본단위(float)
    const float th2_c = dev_setting->alarm_th2 / 10.0f;   // °C
    const float th1_c = dev_setting->alarm_th1 / 10.0f;   // °C
    const float tl2_c = dev_setting->alarm_tl2 / 10.0f;   // °C
    const float tl1_c = dev_setting->alarm_tl1 / 10.0f;   // °C

    const float rh2_usvh = dev_setting->alarm_rh2 / 100.0f; // µSv/h
    const float rh1_usvh = dev_setting->alarm_rh1 / 100.0f; // µSv/h

    // 기본단위 → 표시단위
    const float th2_v = use_f ? (th2_c * 9.0f/5.0f + 32.0f) : th2_c;
    const float th1_v = use_f ? (th1_c * 9.0f/5.0f + 32.0f) : th1_c;
    const float tl2_v = use_f ? (tl2_c * 9.0f/5.0f + 32.0f) : tl2_c;
    const float tl1_v = use_f ? (tl1_c * 9.0f/5.0f + 32.0f) : tl1_c;

    const float rh2_v = use_uR ? (rh2_usvh * 100.0f) : rh2_usvh; // uR/h 또는 µSv/h
    const float rh1_v = use_uR ? (rh1_usvh * 100.0f) : rh1_usvh;

    // 범위 체크 helper (값이 축 범위 내인지)
    #define IN_TEMP_RANGE(v) ((v) >= g_temp_axis_min && (v) <= temp_highest_scale_up)
    #define IN_RAD_RANGE(v)  ((v) >= (float)RAD_MIN && (v) <= (float)rad_highest_scale_up)
    // Y가 실제 표시영역 내인지(픽셀)
    #define IN_Y_BOUNDS(Y)   ((Y) >= y_top && (Y) <= y_bottom)

    // ───────── TH2 ─────────
    if (ALARM_SHOULD_DRAW(ALARM_STATE_POS_TH2) && IN_TEMP_RANGE(th2_v)) {
        y = (int)map_value(th2_v, g_temp_axis_min, temp_highest_scale_up, y_top, y_bottom);
        if (IN_Y_BOUNDS(y)) {
            tpdf_draw_simple_dashed_line(file_p, CHART_X_START, y, CHART_X_START + CHART_WIDTH, y,
                                         1.5, 0.9, 0.4, 0.0, 4);
            snprintf(label_buffer, sizeof(label_buffer), "%.1f", th2_v);
            tpdf_draw_colored_text(file_p, CHART_X_START - 20, y - 3, "F2", 8, 0.9, 0.4, 0.0, label_buffer);
        }
    }

    // ───────── TH1 ─────────
    if (ALARM_SHOULD_DRAW(ALARM_STATE_POS_TH1) && IN_TEMP_RANGE(th1_v)) {
        y = (int)map_value(th1_v, g_temp_axis_min, temp_highest_scale_up, y_top, y_bottom);
        if (IN_Y_BOUNDS(y)) {
            tpdf_draw_simple_dashed_line(file_p, CHART_X_START, y, CHART_X_START + CHART_WIDTH, y,
                                         1.5, 1.0, 0.75, 0.5, 4);
            snprintf(label_buffer, sizeof(label_buffer), "%.1f", th1_v);
            tpdf_draw_colored_text(file_p, CHART_X_START - 20, y - 3, "F2", 8, 1.0, 0.75, 0.5, label_buffer);
        }
    }

    // ───────── TL2 ─────────
    if (ALARM_SHOULD_DRAW(ALARM_STATE_POS_TL2) && IN_TEMP_RANGE(tl2_v)) {
        y = (int)map_value(tl2_v, g_temp_axis_min, temp_highest_scale_up, y_top, y_bottom);
        if (IN_Y_BOUNDS(y)) {
            tpdf_draw_simple_dashed_line(file_p, CHART_X_START, y, CHART_X_START + CHART_WIDTH, y,
                                         1.5, 0.6, 0.9, 0.6, 4);
            snprintf(label_buffer, sizeof(label_buffer), "%.1f", tl2_v);
            tpdf_draw_colored_text(file_p, CHART_X_START - 20, y - 3, "F2", 8, 0.6, 0.9, 0.6, label_buffer);
        }
    }

    // ───────── TL1 ─────────
    if (ALARM_SHOULD_DRAW(ALARM_STATE_POS_TL1) && IN_TEMP_RANGE(tl1_v)) {
        y = (int)map_value(tl1_v, g_temp_axis_min, temp_highest_scale_up, y_top, y_bottom);
        if (IN_Y_BOUNDS(y)) {
            tpdf_draw_simple_dashed_line(file_p, CHART_X_START, y, CHART_X_START + CHART_WIDTH, y,
                                         1.5, 0.1, 0.5, 0.1, 4);
            snprintf(label_buffer, sizeof(label_buffer), "%.1f", tl1_v);
            tpdf_draw_colored_text(file_p, CHART_X_START - 20, y - 3, "F2", 8, 0.1, 0.5, 0.1, label_buffer);
        }
    }

    // ───────── RH2 ─────────
    if (ALARM_SHOULD_DRAW(ALARM_STATE_POS_RH2) && IN_RAD_RANGE(rh2_v)) {
        y = (int)map_value(rh2_v, (float)RAD_MIN, (float)rad_highest_scale_up, y_top, y_bottom);
        if (IN_Y_BOUNDS(y)) {
            tpdf_draw_simple_dashed_line(file_p, CHART_X_START, y, CHART_X_START + CHART_WIDTH, y,
                                         1.5, 0.0, 0.75, 1.0, 4);
            if (use_uR) snprintf(label_buffer, sizeof(label_buffer), "%.0f", rh2_v);   // uR/h: 정수
            else        snprintf(label_buffer, sizeof(label_buffer), "%.2f", rh2_v);   // uSv/h: 소수
            tpdf_draw_colored_text(file_p, CHART_X_START + CHART_WIDTH + 5, y - 3, "F2", 8, 0.0, 0.75, 1.0, label_buffer);
        }
    }

    // ───────── RH1 ─────────
    if (ALARM_SHOULD_DRAW(ALARM_STATE_POS_RH1) && IN_RAD_RANGE(rh1_v)) {
        y = (int)map_value(rh1_v, (float)RAD_MIN, (float)rad_highest_scale_up, y_top, y_bottom);
        if (IN_Y_BOUNDS(y)) {
            tpdf_draw_simple_dashed_line(file_p, CHART_X_START, y, CHART_X_START + CHART_WIDTH, y,
                                         1.0, 0.5, 0.8, 1.0, 4);
            if (use_uR) snprintf(label_buffer, sizeof(label_buffer), "%.0f", rh1_v);   // uR/h: 정수
            else        snprintf(label_buffer, sizeof(label_buffer), "%.2f", rh1_v);   // uSv/h: 소수
            tpdf_draw_colored_text(file_p, CHART_X_START + CHART_WIDTH + 5, y - 3, "F2", 8, 0.5, 0.8, 1.0, label_buffer);
        }
    }

    #undef IN_TEMP_RANGE
    #undef IN_RAD_RANGE
    #undef IN_Y_BOUNDS
}




/**
 * @brief Plots a set of data points onto a PDF chart as a line graph.
 * @note This function has been updated to correctly map X-coordinates across multiple chunks.
 *
 * @param file_p              Pointer to the PDF file object.
 * @param y_pos               The top Y-coordinate for the chart area.
 * @param data                An array of FLOATING POINT data to plot.
 * @param start_index         The starting index of this data chunk in the overall dataset.
 * @param num_points_in_chunk The number of points in the current data array (chunk).
 * @param total_points        The total number of points in the entire dataset across all chunks.
 * @param r, g, b             The color of the line (0.0 to 1.0).
 * @param data_min            The minimum value of the data range for scaling.
 * @param data_max            The maximum value of the data range for scaling.
 */
static void pdf_chart_plot_data(FIL* file_p, int y_pos, float* data, int start_index, int num_points_in_chunk, int total_points, float r, float g, float b, float data_min, float data_max) {
    // A single point cannot be drawn as a line, so we need at least 2.
    if (num_points_in_chunk < 2) {
        return;
    }

    const int chart_y_start = y_pos - FIELD_SPACING;

    for (int i = 0; i < num_points_in_chunk - 1; i++) {
        // Map current point (i) and next point (i+1) to PDF coordinates.
        // The X-coordinate is now mapped relative to the total number of points.
        float x1 = map_value((float)(start_index + i), 0.0f, (float)(total_points - 1), CHART_X_START, CHART_X_START + CHART_WIDTH);
        float y1 = map_value(data[i], data_min, data_max, chart_y_start - CHART_HEIGHT, chart_y_start);

        float x2 = map_value((float)(start_index + i + 1), 0.0f, (float)(total_points - 1), CHART_X_START, CHART_X_START + CHART_WIDTH);
        float y2 = map_value(data[i + 1], data_min, data_max, chart_y_start - CHART_HEIGHT, chart_y_start);

        // Draw a line connecting the two points
        tpdf_draw_line(file_p, (int)x1, (int)y1, (int)x2, (int)y2, 1.25, r, g, b);
    }
}


static void pdf_gen_chart(FIL* file_p, int *y_pos, const logging_summary_t *log,
                                        const DeviceSettings *dev_setting)
{
    //FIXME: hack right here
     const uint16_t record_nums = log->data_points_temp_count;

     float t_max_c = log->highest_temp + TEMP_SCALE_UP;
     float t_min_c = log->lowest_temp - 10.0f;
     const int use_f  = (current_settings.display_temp_unit == 1);
     const int use_uR = (current_settings.display_dose_unit == 1);

     float temp_highest_scale_up = use_f ? (t_max_c*9.0f/5.0f + 32.0f) : t_max_c;
     g_temp_axis_min            = use_f ? (t_min_c*9.0f/5.0f + 32.0f) : t_min_c;

     float r_max_base = log->highest_radiation + RAD_SCALE_UP;   // base: uSv/h
     unsigned int rad_highest_scale_up = use_uR ? (unsigned int)(r_max_base*100.0f)
                                                : (unsigned int)(r_max_base);

    pdf_chart_draw_axes(file_p, *y_pos, temp_highest_scale_up ,rad_highest_scale_up, &log->start_time, &log->stop_time);
//    pdf_chart_draw_axes(file_p,*y_pos,temp_highest_scale_up,rad_highest_scale_up,&(log->start_time),&(log->stop_time));
    pdf_chart_draw_gridlines(file_p,*y_pos,temp_highest_scale_up,rad_highest_scale_up);

    pdf_chart_draw_alarm_lines(file_p,*y_pos,temp_highest_scale_up,rad_highest_scale_up,dev_setting);

    // --- Plot the actual data ---
    log_entry_t entry;
    log_entry_t last_entry_of_chunk;
    // DATAPOINTS_PER_PAGE: 480
    //FIXME: use: 3840 byte here?
    float tmp_temper[DATAPOINTS_PER_PAGE];
    float tmp_dose[DATAPOINTS_PER_PAGE];
    uint16_t buffer_count =0;
    uint16_t chunk_start_index = 0;

    for (int i = 0; i < record_nums; i++) {
        // Read the record from data log
        meas_data_log_read_entry(i, &entry);

//        if (entry.year == 0xFF || entry.index == 0xFFFF || entry.month == 0xFF)
//            break;
//        file_log_time_t time = {
//            .year   = entry.year,
//            .month  = entry.month,
//            .day    = entry.day,
//            .hour   = entry.hour,
//            .minute = entry.minute,
//            .second = entry.second
//        };
//        pdf_append_new_record(i, &time, (float)entry.temperature/ 10.0f, (float)entry.dose/ 100.0f);

        // Line between last point of previous chunk and current chunk. (such as: point 479 to 480)
        if (chunk_start_index > 0 && buffer_count == 0) {
            const int chart_y_start = *y_pos - FIELD_SPACING;

            // Calculate coordinates of the previous chunk's last point
            float prev_x = map_value((float)(chunk_start_index - 1), 0.0f, (float)(record_nums - 1), CHART_X_START, CHART_X_START + CHART_WIDTH);



            // Calculate coordinates of the current chunk's first point
            float curr_x = map_value((float)chunk_start_index, 0.0f, (float)(record_nums - 1), CHART_X_START, CHART_X_START + CHART_WIDTH);

            float prev_t = (float)last_entry_of_chunk.temperature/10.0f;
            float prev_r = (float)last_entry_of_chunk.dose/100.0f;
            if (use_f)  prev_t = prev_t*9.0f/5.0f + 32.0f;
            if (use_uR) prev_r = prev_r*100.0f;

            float curr_t = (float)entry.temperature/10.0f;
            float curr_r = (float)entry.dose/100.0f;
            if (use_f)  curr_t = curr_t*9.0f/5.0f + 32.0f;
            if (use_uR) curr_r = curr_r*100.0f;

            float prev_temp_y = map_value(prev_t, g_temp_axis_min, temp_highest_scale_up, chart_y_start - CHART_HEIGHT, chart_y_start);
            float prev_dose_y = map_value(prev_r, RAD_MIN,          rad_highest_scale_up, chart_y_start - CHART_HEIGHT, chart_y_start);
            float curr_temp_y = map_value(curr_t, g_temp_axis_min,  temp_highest_scale_up, chart_y_start - CHART_HEIGHT, chart_y_start);
            float curr_dose_y = map_value(curr_r, RAD_MIN,          rad_highest_scale_up, chart_y_start - CHART_HEIGHT, chart_y_start);

            // Draw the connecting lines
            tpdf_draw_line(file_p, (int)prev_x, (int)prev_temp_y, (int)curr_x, (int)curr_temp_y, 0.75, 1.0f, 0.0f, 0.0f);
            tpdf_draw_line(file_p, (int)prev_x, (int)prev_dose_y, (int)curr_x, (int)curr_dose_y, 0.75, 0.0f, 0.0f, 1.0f);
        }
        // Add the data to our temporary buffers
        float t = (float)entry.temperature/10.0f;   // base: C
        float r = (float)entry.dose/100.0f;         // base: uSv/h
        if (use_f)  t = t*9.0f/5.0f + 32.0f;
        if (use_uR) r = r*100.0f;
        tmp_temper[buffer_count] = t;
        tmp_dose[buffer_count]   = r;
        buffer_count++;

        // If the buffer is full, plot the data and reset the buffer
        if (buffer_count == DATAPOINTS_PER_PAGE) {
            // Plot temperature data (Red)
            pdf_chart_plot_data(file_p, *y_pos, tmp_temper, chunk_start_index, buffer_count, record_nums, 1.0f, 0.0f, 0.0f, g_temp_axis_min , temp_highest_scale_up);
            // Plot dose data (Blue)
            pdf_chart_plot_data(file_p, *y_pos, tmp_dose, chunk_start_index, buffer_count, record_nums, 0.0f, 0.0f, 1.0f, RAD_MIN, rad_highest_scale_up);

            last_entry_of_chunk = entry; // Save the last point
            // Update the start index for the next chunk
            chunk_start_index += buffer_count;
            // Reset counter to start filling the buffer again
            buffer_count = 0;
        }

    }
    // If records num < 480?:
    if (buffer_count > 0) {
        // Plot remaining temperature data (Red)
        pdf_chart_plot_data(file_p, *y_pos, tmp_temper, chunk_start_index, buffer_count, record_nums, 1.0f, 0.0f, 0.0f, g_temp_axis_min , temp_highest_scale_up);
        // Plot remaining dose data (Blue)
        pdf_chart_plot_data(file_p, *y_pos, tmp_dose, chunk_start_index, buffer_count, record_nums, 0.0f, 0.0f, 1.0f, RAD_MIN, rad_highest_scale_up);
    }

    // Legend
    int y = *y_pos - FIELD_SPACING;
    // start at 2/3 chart
    tpdf_draw_colored_text(file_p, (CHART_X_START + CHART_WIDTH /2 ), y + 16, "F2", 6, 0, 0, 0, "Temperature");
    tpdf_draw_line(file_p, (CHART_X_START + CHART_WIDTH /2 + 40),  y + 18, (CHART_X_START + CHART_WIDTH /2 + 40 + 15), y + 18, 1.0, 1.0, 0, 0);

    tpdf_draw_colored_text(file_p, (CHART_X_START + CHART_WIDTH /2), y + 8, "F2", 6, 0, 0, 0, "Rad");
    tpdf_draw_line(file_p, (CHART_X_START + CHART_WIDTH /2 + 40),  y + 10, (CHART_X_START + CHART_WIDTH /2 + 40 + 15), y + 10, 1.0, 0, 0, 1.0);

    int alarm_x = CHART_X_START + CHART_WIDTH /2 + 75;

    // TH1: Light Orange: 1.0 0.75 0.5 dashed line for alarm
    // TH2: Dark Orange: 0.9 0.4 0.0 dashed line for alarm
    tpdf_draw_colored_text(file_p, alarm_x , y + 16, "F2", 6, 0, 0, 0, "TH2");
    tpdf_draw_line(file_p, alarm_x +15,  y + 18, (alarm_x +15 + 15), y + 18, 1.0, 0.9, 0.4, 0.0);
    tpdf_draw_colored_text(file_p, alarm_x, y + 8, "F2", 6, 0, 0, 0, "TH1");
    tpdf_draw_line(file_p, alarm_x +15 ,  y + 10, (alarm_x +15 + 15), y + 10, 1.0, 1.0, 0.75, 0.5);

    // TL1: Dark Green: 0.1,0.5,0.1 dashed line for alarm
    // TL2: Light Green: 0.6 0.9 0.6 dashed line for alarm
    alarm_x += 50;
    tpdf_draw_colored_text(file_p, alarm_x , y + 16, "F2", 6, 0, 0, 0, "TL2");
    tpdf_draw_line(file_p, alarm_x +15,  y + 18, (alarm_x +15 + 15), y + 18, 1.0, 0.6,0.9,0.6);
    tpdf_draw_colored_text(file_p, alarm_x, y + 8, "F2", 6, 0, 0, 0, "TL1");
    tpdf_draw_line(file_p, alarm_x +15 ,  y + 10, (alarm_x +15 + 15), y + 10, 1.0, 0.1,0.5,0.1);

    // RH2: Dark Sky Blue (or Deep Sky Blue): 0.0,0.75,1.0  dashed line for alarm
    // RH1: Light Sky Blue: 0.5,0.8,1.0 dashed line for alarm
    alarm_x += 50;
    tpdf_draw_colored_text(file_p, alarm_x , y + 16, "F2", 6, 0, 0, 0, "RH2");
    tpdf_draw_line(file_p, alarm_x +15,  y + 18, (alarm_x +15 + 15), y + 18, 1.0, 0.0,0.75,1.0);
    tpdf_draw_colored_text(file_p, alarm_x, y + 8, "F2", 6, 0, 0, 0, "RH1");
    tpdf_draw_line(file_p, alarm_x +15 ,  y + 10, (alarm_x +15 + 15), y + 10, 1.0, 0.5,0.8,1.0);

}


//Update at the stop time
static void pdf_table_start_stop_time_header_obj_6(const logging_summary_t *log) {
    unsigned int stream_length = 0;
    char tmp_start_time[32];
    char tmp_stop_time[32];
    char buff[64];

    time_to_dmyhms_string(&log->start_time, tmp_start_time, sizeof(tmp_start_time));
    time_to_dmyhms_string(&log->stop_time, tmp_stop_time, sizeof(tmp_stop_time));

    tpdf_start_new_stream_obj(&pdf_file, TPDF_OBJ_NUM_HEADER_TIME_START_STOP, TPDF_OBJ_NUM_HEADER_TIME_START_STOP + 1, &pdf_monitor);

    snprintf(buff, sizeof(buff), "Start time: %s", tmp_start_time);
    stream_length += tpdf_draw_colored_text(&pdf_file, PDF_PAGE_LEFT, PDF_PAGE_TOP, "F1", 9, 0,0,0, buff);

    snprintf(buff, sizeof(buff), "Stop time: %s", tmp_stop_time);
    stream_length += tpdf_draw_colored_text(&pdf_file, PDF_PAGE_RIGHT - 120, PDF_PAGE_TOP, "F1", 9, 0,0,0, buff);

    stream_length += tpdf_draw_line(&pdf_file, PDF_PAGE_LEFT, PDF_PAGE_TOP - 5, PDF_PAGE_RIGHT, PDF_PAGE_TOP - 5, 1.5, 0,0,0);
    tpdf_end_new_stream_obj(&pdf_file, TPDF_OBJ_NUM_HEADER_TIME_START_STOP + 1, stream_length, &pdf_monitor);
}


// Create obj 8 which is shared for data record pages.
// Table information
// Split into 5 smaller table with size: 99
// Data area: PAGE LEFT to PAPE LEFT + 49 (49)
// C: PAPE LEFT + 49 to LEFT + 69 (20)
// mCv: PAPE LEFT + 69 to PAPE LEFT +99 (30)

static void pdf_table_format_obj_8(void) {
  unsigned int stream_length = 0;

  tpdf_start_new_stream_obj(&pdf_file,TPDF_OBJ_NUM_TABLE_TEMPL,TPDF_OBJ_NUM_TABLE_TEMPL + 1, &pdf_monitor);


  // draw a rectangle
  // 1st table row
  stream_length += tpdf_draw_line(&pdf_file,PDF_PAGE_RIGHT, PDF_PAGE_TOP-20, PDF_PAGE_LEFT, PDF_PAGE_TOP-20, 1.5, 0,0,0);
  // 2nd table row (Y: 800 - 35 down to 50 + 20)
  stream_length += tpdf_draw_line(&pdf_file,PDF_PAGE_RIGHT, PDF_PAGE_TOP - 35, PDF_PAGE_LEFT, PDF_PAGE_TOP - 35, 1.5, 0,0,0);

  stream_length += tpdf_draw_line(&pdf_file,PDF_PAGE_LEFT, PDF_PAGE_BOTTOM + 20, PDF_PAGE_RIGHT, PDF_PAGE_BOTTOM + 20, 1.5, 0,0,0);

  //Split into 5 tables
  for (int i = 0; i<= CHILD_TABLE_NUMS; i++ ) {
    stream_length += tpdf_draw_line(&pdf_file, (PDF_PAGE_LEFT + i *99), PDF_PAGE_TOP-20, (PDF_PAGE_LEFT + i *99), PDF_PAGE_BOTTOM + 20, 1.5, 0,0,0);
    if (i < CHILD_TABLE_NUMS)
    {
      stream_length += tpdf_draw_line(&pdf_file, (PDF_PAGE_LEFT + 51 + i *CHILD_TABLE_SIZE), PDF_PAGE_TOP-20, (PDF_PAGE_LEFT + 51 + i *99), PDF_PAGE_BOTTOM + 20, 0.5, 0.5,0.5,0.5);
      stream_length += tpdf_draw_line(&pdf_file, (PDF_PAGE_LEFT + 69 + i *CHILD_TABLE_SIZE), PDF_PAGE_TOP-20, (PDF_PAGE_LEFT + 69 + i *99), PDF_PAGE_BOTTOM + 20, 0.5, 0.5,0.5,0.5);
      //Data, C, mCV (49,20,30)
      stream_length += tpdf_draw_colored_text(&pdf_file,PDF_PAGE_LEFT + 10 + i *CHILD_TABLE_SIZE, PDF_PAGE_TOP - 30 ,"F2",6,0,0,0,"Date Time");
      const char *hdr_T = (current_settings.display_temp_unit==0) ? "C" : "F";
      const char *hdr_R = (current_settings.display_dose_unit==0) ? "uSv/h" : "uR/h";
      stream_length += tpdf_draw_colored_text(&pdf_file,PDF_PAGE_LEFT + 56 + i *CHILD_TABLE_SIZE, PDF_PAGE_TOP - 30 ,"F2",6,0,0,0,hdr_T);
      stream_length += tpdf_draw_colored_text(&pdf_file,PDF_PAGE_LEFT + 76 + i *CHILD_TABLE_SIZE, PDF_PAGE_TOP - 30 ,"F2",6,0,0,0,hdr_R);

    }
  }
  //FIXME: Should create obj footer??
  //https://gonucare.com/
  stream_length += tpdf_draw_line(&pdf_file,PDF_PAGE_LEFT, PDF_PAGE_BOTTOM, PDF_PAGE_RIGHT, PDF_PAGE_BOTTOM, 1.5, 0,0,0);
  stream_length += tpdf_draw_colored_text(&pdf_file,PDF_PAGE_LEFT , PDF_PAGE_BOTTOM -10 ,"F1",9,0,0,1, PDF_COMP_HTTPS_LINK);
  stream_length += tpdf_draw_colored_text(&pdf_file,PDF_PAGE_RIGHT-40 , PDF_PAGE_BOTTOM -10 ,"F1",9,0,0,1,"NUCARE");
  tpdf_end_new_stream_obj(&pdf_file,TPDF_OBJ_NUM_TABLE_TEMPL + 1,stream_length,&pdf_monitor);

}

// This function does not handle page numbers;
// it only calculates the position of each data point based on its index.
static inline void convert_record_num_to_x_y_position(const UINT record_num, UINT *x, UINT* y) {

    UINT index = 0;
    UINT index_in_child_table =0;
    UINT child_table_num = 0;
    // find the child table this record is settle on
    // settle on child_table_idx : (UINT) (index / DATAPOINTS_PER_CHILD_TABLE)
    // index in child table: (UINT) (index % DATAPOINTS_PER_CHILD_TABLE)
    // *x = TEXT_DATAPOINT_X_POS_START + child_table_idx * CHILD_TABLE_SIZE;
    // *y = TEXT_DATAPOINT_Y_POS_START - index in child table * DATAPOINTS_Y_MARGIN

    index = record_num % DATAPOINTS_PER_PAGE;
    child_table_num = (UINT) (index / DATAPOINTS_PER_CHILD_TABLE);
    index_in_child_table = (index % DATAPOINTS_PER_CHILD_TABLE);
    *x = TEXT_DATAPOINT_X_POS_START + child_table_num * CHILD_TABLE_SIZE;
    *y = TEXT_DATAPOINT_Y_POS_START - index_in_child_table * DATAPOINTS_Y_MARGIN;
    // determine child table
}

static int pdf_insert_datapoint_to_table(const UINT record_num,
                                         const file_log_time_t *time,
                                         float temperature,
                                         float radiation,
                                         const DeviceSettings *dev_setting)
{
    unsigned int y_pos = 0, x_pos = 0;
    char buff[128], time_buff[64];

    // 임계값 스케일 해제 (정수 보관 가정: rad×100, temp×10)
    const float rh1 = dev_setting->alarm_rh1 / 100.0f;
    const float rh2 = dev_setting->alarm_rh2 / 100.0f;
    const float tl1 = dev_setting->alarm_tl1 / 10.0f;
    const float tl2 = dev_setting->alarm_tl2 / 10.0f;
    const float th1 = dev_setting->alarm_th1 / 10.0f;
    const float th2 = dev_setting->alarm_th2 / 10.0f;

    // 방사선 n/a(radiation<0)는 방사선 알람에서 제외
    bool rad_alarm  = (radiation >= 0.0f) && (radiation > rh1 || radiation > rh2);
    bool temp_alarm = (temperature < tl1 || temperature < tl2 ||
                       temperature >= th1 || temperature >= th2);
    bool is_alarm = (rad_alarm || temp_alarm);

    time_to_dmyhms_string(time, time_buff, sizeof(time_buff));
    convert_record_num_to_x_y_position(record_num, &x_pos, &y_pos);

    const int use_f = (current_settings.display_temp_unit == 1);
    const int use_uR = (current_settings.display_dose_unit == 1);

    float t_disp = temperature;           // base: C
    float r_disp = radiation;             // base: uSv/h (or -1.0f for n/a)
    if (use_f)  t_disp = t_disp*9.0f/5.0f + 32.0f;
    if (use_uR && r_disp >= 0.0f) r_disp = r_disp*100.0f;

    if (radiation < 0.0f) {
        snprintf(buff, sizeof(buff), "%s     %-5.1f      n/a", time_buff, t_disp);
    } else {
        if (use_uR) {
            unsigned r_int = (unsigned)(r_disp + 0.5f); // uR/h 정수
            snprintf(buff, sizeof(buff), "%s     %-5.1f      %-6u", time_buff, t_disp, r_int);
        } else {
            snprintf(buff, sizeof(buff), "%s     %-5.1f      %-6.2f", time_buff, t_disp, r_disp);
        }
    }



    // ★ 여기서 색을 최종 결정해서 찍습니다. (위쪽 rg 설정은 쓰지 마세요)
    float r = is_alarm ? 1.0f : 0.0f, g = 0.0f, b = 0.0f;
    return tpdf_draw_colored_text(&pdf_file, x_pos, y_pos, "F1", TEXT_DATAPOINT_FONT_SIZE, r, g, b, buff);
}
//static int pdf_insert_datapoint_to_table(const UINT record_num,
//                                         const file_log_time_t *time,
//                                         float temperature,
//                                         float radiation,
//                                         const DeviceSettings *dev_setting)
//{
//    unsigned int y_pos = 0, x_pos = 0;
//    char buff[128], time_buff[64];
//
//    // 임계값 스케일 해제 (정수 보관 가정: rad×100, temp×10)
//    const float rh1 = dev_setting->alarm_rh1 / 100.0f;
//    const float rh2 = dev_setting->alarm_rh2 / 100.0f;
//    const float tl1 = dev_setting->alarm_tl1 / 10.0f;
//    const float tl2 = dev_setting->alarm_tl2 / 10.0f;
//    const float th1 = dev_setting->alarm_th1 / 10.0f;
//    const float th2 = dev_setting->alarm_th2 / 10.0f;
//
//    // --- 추가: 경과 시간(초) → 딜레이 게이트 ---
//    // record_num 이 0부터 증가한다고 가정.
//    // (만약 1부터라면 max(0, record_num-1)로 바꿔주세요)
//    uint32_t elapsed_sec = (uint32_t)record_num * (uint32_t)dev_setting->temp_interval;
//
//    bool gate_rh1 = (elapsed_sec >= dev_setting->alarm_delay_rh1);
//    bool gate_rh2 = (elapsed_sec >= dev_setting->alarm_delay_rh2);
//    bool gate_th1 = (elapsed_sec >= dev_setting->alarm_delay_th1);
//    bool gate_th2 = (elapsed_sec >= dev_setting->alarm_delay_th2);
//    bool gate_tl1 = (elapsed_sec >= dev_setting->alarm_delay_tl1);
//    bool gate_tl2 = (elapsed_sec >= dev_setting->alarm_delay_tl2);
//
//    // 방사선 n/a(radiation<0)는 방사선 알람에서 제외
//    bool rad_alarm  = (radiation >= 0.0f) &&
//                      ( (gate_rh1 && (radiation >  rh1)) ||
//                        (gate_rh2 && (radiation >  rh2)) );
//
//    // 온도: 각 임계에 대한 게이트가 열린 경우만 판정
//    bool temp_alarm = ( (gate_tl1 && (temperature <  tl1)) ||
//                        (gate_tl2 && (temperature <  tl2)) ||
//                        (gate_th1 && (temperature >= th1)) ||
//                        (gate_th2 && (temperature >= th2)) );
//
//    bool is_alarm = (rad_alarm || temp_alarm);
//
//    time_to_dmyhms_string(time, time_buff, sizeof(time_buff));
//    convert_record_num_to_x_y_position(record_num, &x_pos, &y_pos);
//
//    if (radiation < 0.0f)
//        snprintf(buff, sizeof(buff), "%s     %-5.1f      n/a",     time_buff, temperature);
//    else
//        snprintf(buff, sizeof(buff), "%s     %-5.1f      %-6.2f", time_buff, temperature, radiation);
//
//    // 알람이면 빨강(1,0,0), 아니면 검정(0,0,0)
//    float r = is_alarm ? 1.0f : 0.0f, g = 0.0f, b = 0.0f;
//    return tpdf_draw_colored_text(&pdf_file, x_pos, y_pos, "F1", TEXT_DATAPOINT_FONT_SIZE, r, g, b, buff);
//}

static void pdf_write_page_structure(void) {
	printf("PDF First Page Make\r\n");
    fatfs_write_string(&pdf_file, TPDF_HEADER);

    char content_buffer[256];

    // --- Create Object 2: The Catalog ---
    sprintf(content_buffer, "<<\n/Type /Catalog\n/Pages %d 0 R >>", TPDF_OBJ_PAGES_KID);
    tpdf_add_new_obj(&pdf_file, TPDF_OBJ_NUM_CATALOG, content_buffer, &pdf_monitor);

    // --- Create Object 4: Arial Font ---
    sprintf(content_buffer,"<<\n/Type /Font\n/Subtype /TrueType\n/Name /F1\n/BaseFont /%s\n/Encoding /WinAnsiEncoding\n>>",TPDF_FONT);
    tpdf_add_new_obj(&pdf_file, TPDF_OBJ_NUM_FONT_ARIAL, content_buffer, &pdf_monitor);

    // --- Create Object 5: Arial Bold Font ---
    sprintf(content_buffer,"<<\n/Type /Font\n/Subtype /TrueType\n/Name /F2\n/BaseFont /%s\n/Encoding /WinAnsiEncoding\n>>",TPDF_FONT_BOLD);
    tpdf_add_new_obj(&pdf_file, TPDF_OBJ_NUM_FONT_ARIAL_BOLD, content_buffer, &pdf_monitor);

    // --- Create Object 17: The Page's Resource Dictionary ---
    // This object references the two fonts we just created.
    sprintf(content_buffer, "<<\n/Font << /F1 %d 0 R /F2 %d 0 R >>\n/ProcSet [ /PDF /Text ] \n>>",
            TPDF_OBJ_NUM_FONT_ARIAL,
            TPDF_OBJ_NUM_FONT_ARIAL_BOLD);
    tpdf_add_new_obj(&pdf_file, TPDF_OBJ_NUM_PAGE_RESOURCES, content_buffer, &pdf_monitor);

    pdf_table_format_obj_8();
}

static void pdf_write_footer_and_xref(void) {
    char  work_buffer[128];
    ULONG xref_offset = f_size(&pdf_file);

    // xref header
    fatfs_write_string(&pdf_file, "xref\n");
    sprintf(work_buffer, "0 %u\n", (unsigned)(pdf_monitor.num_objs + 1));
    fatfs_write_string(&pdf_file, work_buffer);

    // obj 0 : free
    fatfs_write_string(&pdf_file, "0000000000 65535 f \n");

    // obj 1..N : in-use
    for (unsigned i = 0; i < (unsigned)pdf_monitor.num_objs; i++) {
        fatfs_write_string(&pdf_file, "0000000000 00000 n \n");
    }

    // trailer
    fatfs_write_string(&pdf_file, "trailer\n");
    sprintf(work_buffer, "<< /Size %u /Root %u 0 R >>\n",
            (unsigned)(pdf_monitor.num_objs + 1),
            (unsigned)TPDF_OBJ_NUM_CATALOG);
    fatfs_write_string(&pdf_file, work_buffer);

    // startxref & EOF
    sprintf(work_buffer, "startxref\n%lu\n%%EOF\n", (unsigned long)xref_offset);
    fatfs_write_string(&pdf_file, work_buffer);
}


static void generate_pdf_raw_report(void) {

    pdf_write_page_structure();
    generate_pdf_report_2nd_page();

}

//FIXME: should be generate the whole page or only chart??
static void generate_pdf_report_1st_page(const logging_summary_t *log,
                                        const DeviceSettings *dev_setting)
{
    // --- DRY RUN: Calculate the total length of the content stream ---
        char content_buffer[256];
        const logging_summary_t *summary = csv_get_logging_summary();

    // --- Create Object 10: first page
    sprintf(content_buffer, "<<\n/Type /Page\n/Parent %d 0 R\n/Contents [ %d 0 R ]\n/Resources %d 0 R\n/Rotate 0\n>>", \
            TPDF_OBJ_PAGES_KID, \
            TPDF_OBJ_NUM_PAGE_1ST_CONTENTS,\
            TPDF_OBJ_NUM_PAGE_RESOURCES);
    //OBJ10: root obj page
    tpdf_add_new_obj(&pdf_file, TPDF_OBJ_NUM_PAGE_1ST, content_buffer, &pdf_monitor);

    tpdf_start_new_stream_obj(&pdf_file,TPDF_OBJ_NUM_PAGE_1ST_CONTENTS,TPDF_OBJ_NUM_PAGE_1ST_CONTENTS_LENGTH,&pdf_monitor);

    ULONG stream_len = 0;
    // Write the content by calling the functions again, this time with a file pointer
    int current_y = PDF_PAGE_TOP;
    stream_len += tpdf_draw_colored_text(&pdf_file,PDF_PAGE_LEFT,PDF_PAGE_TOP,"F2",24,0,0,1,"DATA LOG");
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    char created_at[32];
    snprintf(created_at, sizeof(created_at), "20%02u-%02u-%02u %02u:%02u:%02u",
             sDate.Year, sDate.Month, sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds);

    stream_len += tpdf_draw_colored_text(&pdf_file, PDF_PAGE_LEFT + 130, PDF_PAGE_TOP, "F2", 9, 0,0,0, "File Created At: ");
    stream_len += tpdf_draw_colored_text(&pdf_file, PDF_PAGE_LEFT + 260, PDF_PAGE_TOP, "F1", 9, 0,0,0, created_at);


//    stream_len += tpdf_draw_colored_text(&pdf_file,PDF_PAGE_LEFT + 130 + 70 ,PDF_PAGE_TOP,"F1",9,0,0,0,tmp_start_time);
    stream_len += tpdf_draw_line(&pdf_file, PDF_PAGE_LEFT, PDF_PAGE_TOP - 5, PDF_PAGE_RIGHT,PDF_PAGE_TOP - 5, 2.0, 0,0,0);

    current_y -= 40;
    stream_len += pdf_device_info(&pdf_file, &current_y, dev_setting);
    stream_len += pdf_trip_info(&pdf_file, &current_y,  dev_setting);
    stream_len += pdf_config_info(&pdf_file, &current_y, dev_setting);
    stream_len += pdf_alarm_table(&pdf_file, &current_y,dev_setting, summary); // Draw the new table
    stream_len += pdf_logging_summary(&pdf_file, &current_y, summary);

    current_y -= 10;

    pdf_gen_chart(&pdf_file,&current_y,summary,dev_setting);

    //FIXME: Should create obj footer??
    //https://gonucare.com/
    stream_len += tpdf_draw_line(&pdf_file,PDF_PAGE_LEFT, PDF_PAGE_BOTTOM, PDF_PAGE_RIGHT, PDF_PAGE_BOTTOM, 1.5, 0,0,0);
    stream_len += tpdf_draw_colored_text(&pdf_file,PDF_PAGE_LEFT , PDF_PAGE_BOTTOM -10 ,"F1",9,0,0,1, PDF_COMP_HTTPS_LINK);
    stream_len += tpdf_draw_colored_text(&pdf_file,PDF_PAGE_RIGHT-40 , PDF_PAGE_BOTTOM -10 ,"F1",9,0,0,1,"NUCARE");
    tpdf_end_new_stream_obj(&pdf_file,TPDF_OBJ_NUM_PAGE_1ST_CONTENTS_LENGTH,stream_len,&pdf_monitor);

}

static void generate_pdf_report_2nd_page(void) {
    // --- Create Object 20: first page
	char content_buffer[128];
    // Ref to table template and start/stop time header (OBJ 6 and 8 )
    sprintf(content_buffer, "<<\n/Type /Page\n/Parent %d 0 R\n/Contents [ %d 0 R %d 0 R %d 0 R ]\n/Resources %d 0 R\n/Rotate 0\n>>", \
            TPDF_OBJ_PAGES_KID, \
            TPDF_OBJ_NUM_HEADER_TIME_START_STOP,\
            TPDF_OBJ_NUM_TABLE_TEMPL,\
            TPDF_OBJ_NUM_PAGE_2ND +2,\
            TPDF_OBJ_NUM_PAGE_RESOURCES);

    tpdf_add_new_obj(&pdf_file, TPDF_OBJ_NUM_PAGE_2ND, content_buffer, &pdf_monitor);
    // obj 22: contain data content.
    // obj 24: contain data stream length
    // Open stream for writing:
    tpdf_start_new_stream_obj(&pdf_file, TPDF_OBJ_NUM_PAGE_2ND + 2, TPDF_OBJ_NUM_PAGE_2ND + 4, &pdf_monitor);

}

// Number of report page equal record_nums / (number of data points per page)
static void pdf_page_handle_for_new_datapoints(const UINT current_page) {

	char content_buffer[128];
    int stream_length = 0;
    UINT current_obj = TPDF_OBJ_NUM_PAGE_2ND + (current_page *10);
    UINT next_obj = TPDF_OBJ_NUM_PAGE_2ND + (current_page +1) *10;

    // End of stream of previous obj page 
    // FIXME: how to get stream length of current page?
    tpdf_end_new_stream_obj(&pdf_file,current_obj + 4, stream_length ,&pdf_monitor );

    sprintf(content_buffer, "<<\n/Type /Page\n/Parent %d 0 R\n/Contents [ %d 0 R %d 0 R %d 0 R ]\n/Resources %d 0 R\n/Rotate 0\n>>", \
            TPDF_OBJ_PAGES_KID, \
            TPDF_OBJ_NUM_HEADER_TIME_START_STOP,\
            TPDF_OBJ_NUM_TABLE_TEMPL,\
            (next_obj  +2),\
            TPDF_OBJ_NUM_PAGE_RESOURCES);

    tpdf_add_new_obj(&pdf_file, next_obj, content_buffer, &pdf_monitor);
    // obj x2: contain data content.
    // obj x4: contain data stream length
    // Open stream for writing:
    tpdf_start_new_stream_obj(&pdf_file, next_obj + 2, next_obj + 4, &pdf_monitor);
}
