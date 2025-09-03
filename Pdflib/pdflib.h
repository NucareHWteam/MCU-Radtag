/*
 * pdflib.h
 *
 *  Created on: Jul 16, 2025
 *      Author: dongo
 */

#ifndef PDFDRIVER_PDFLIB_H_
#define PDFDRIVER_PDFLIB_H_

#include "stdlib.h"
#include "stdint.h"
#include "ff.h"

#define TPDF_HEADER "%PDF-1.7\n"
#define TPDF_OBJ_MAX_NUM 256
#define TPDF_FONT "Arial"
#define TPDF_FONT_BOLD "Arial,Bold"

// PDF Object Numbers
#define TPDF_OBJ_NUM_CATALOG                          2
#define TPDF_OBJ_PAGES_KID                            3
#define TPDF_OBJ_NUM_FONT_ARIAL                       4
#define TPDF_OBJ_NUM_FONT_ARIAL_BOLD                  5
# define TPDF_OBJ_NUM_HEADER_TIME_START_STOP          6
#define TPDF_OBJ_NUM_TABLE_TEMPL                      8
#define TPDF_OBJ_NUM_PAGE_RESOURCES                   17

// PDF Page Object numbers (assuming they are in increments of 10)
// OBJ_PAGE + 1: page index eg:(1/10)
// OBJ_PAGE + 2: page content
// OBJ_PAGE + 3: page index length
// OBJ_PAGE + 4: page content length
// The 2nd page obj will be TPDF_OBJ_NUM_PAGE_1ST + 10.
#define TPDF_OBJ_NUM_PAGE_1ST                      10
#define TPDF_OBJ_NUM_PAGE_1ST_CONTENTS             11
#define TPDF_OBJ_NUM_PAGE_1ST_CONTENTS_LENGTH      13
#define TPDF_OBJ_NUM_PAGE_1ST_CHART                12
#define TPDF_OBJ_NUM_PAGE_1ST_CHART_LENGTH         14

#define TPDF_OBJ_NUM_PAGE_2ND                      (TPDF_OBJ_NUM_PAGE_1ST + 10)
typedef struct pdfdoc
{
   unsigned long obj_offsets[TPDF_OBJ_MAX_NUM];
   unsigned int num_pages;
   unsigned int num_objs;

} pdf_doc_t;

uint32_t tpdf_add_new_obj(FIL* file_p, int obj_num, const char* content, pdf_doc_t* pdf_monitor);

UINT tpdf_draw_line(FIL* file_p, int x1, int y1, int x2, int y2, float line_width, float r, float g, float b);
UINT tpdf_draw_simple_dashed_line(FIL* file_p, int x1, int y1, int x2, int y2, float line_width, float r, float g, float b, int dash_length);

/**
 * @brief Creates a PDF command string to write colored text at a specific position.
 * @param x           The x-coordinate for the text position.
 * @param y           The y-coordinate for the text position.
 * @param font_name   The internal PDF name for the font (e.g., "F1","F2").
 * @param font_size   The size of the font in points.
 * @param r           The red color component (0.0 to 1.0).
 * @param g           The green color component (0.0 to 1.0).
 * @param b           The blue color component (0.0 to 1.0).
 * @param text        The text string to write.
 */
UINT tpdf_draw_colored_text(FIL* file_p, int x, int y, const char* font_name, int font_size, \
                            float r, float g, float b, const char* text);

// These func for page content obj:
uint32_t tpdf_start_new_stream_obj(FIL* file_p, int obj_num,int obj_length_num, pdf_doc_t* pdf_monitor);
uint32_t tpdf_end_new_stream_obj(FIL* file_p, int obj_length_num, int stream_length, pdf_doc_t* pdf_monitor);


#endif /* PDFDRIVER_PDFLIB_H_ */
