/*
 * pdflib.c
 *
 *  Created on: Jul 16, 2025
 *      Author: dongo
 */

#include <stdio.h>
#include <string.h>
#include "pdflib.h"



/**
 * @brief Creates a PDF command string to write colored text at a specific position.
 * @param x           The x-coordinate for the text position.
 * @param y           The y-coordinate for the text position.
 * @param font_name   The internal PDF name for the font (e.g., "F1").
 * @param font_size   The size of the font in points.
 * @param r           The red color component (0.0 to 1.0).
 * @param g           The green color component (0.0 to 1.0).
 * @param b           The blue color component (0.0 to 1.0).
 * @param text        The text string to write.
 */
UINT tpdf_draw_colored_text(FIL* file_p, int x, int y, const char* font_name, int font_size, \
                            float r, float g, float b, const char* text) {
    // PDF command sequence:
    // BT = Begin Text block
    // rg = Set non-stroking (fill) color in RGB
    // Tf = Set Typeface and font size
    // Td = Set Text position
    // Tj = Show Text (draw the string)
    // ET = End Text block
    char buffer[512];
    UINT len =0;

    snprintf(buffer, sizeof(buffer), "BT %.2f %.2f %.2f rg /%s %d Tf %d %d Td (%s) Tj ET\n", \
             r, g, b, font_name, font_size, x, y, text);
    len = strlen(buffer);
    if (f_puts(buffer,file_p) < 0) {
        return -1;
    }
    return len;
}

UINT tpdf_draw_line(FIL* file_p, int x1, int y1, int x2, int y2, float line_width, float r, float g, float b){
    char buffer[128];
    UINT len = 0;

    snprintf(buffer, sizeof(buffer), "%.2f %.2f %.2f RG %.2f w %d %d m %d %d l S\n",r,g,b,line_width, x1, y1, x2, y2);
    len = strlen(buffer);
    if (f_puts(buffer,file_p) < 0) {
        return -1;
    }

    return len;
}

/**
 * @brief Creates a PDF command string for a simple dashed line (dash = gap).
 * @param buffer      A character array to store the resulting command string.
 * @param buffer_size The size of the buffer to prevent overflow.
 * @param x1          The starting x-coordinate.
 * @param y1          The starting y-coordinate.
 * @param x2          The ending x-coordinate.
 * @param y2          The ending y-coordinate.
 * @param line_width  The width (thickness) of the line in points.
 * @param r           The red color component (0.0 to 1.0).
 * @param g           The green color component (0.0 to 1.0).
 * @param b           The blue color component (0.0 to 1.0).
 * @param dash_length The length of both the dash and the gap.
 */
UINT tpdf_draw_simple_dashed_line(FIL* file_p, int x1, int y1, int x2, int y2, float line_width, float r, float g, float b, int dash_length) {
    char buffer[128];
    UINT len = 0;

    snprintf(buffer, sizeof(buffer), "%.2f %.2f %.2f RG %.2f w [%d %d] 0 d %d %d m %d %d l S\n[] 0 d\n",
                         r, g, b, line_width, dash_length, dash_length, x1, y1, x2, y2);
    len = strlen(buffer);
    if (f_puts(buffer,file_p) < 0) {
        return -1;
    }

    return len;
}

uint32_t tpdf_add_new_obj(FIL* file_p, int obj_num, const char* content, pdf_doc_t* pdf_monitor) {
    char buffer[64];
    UINT status;

    // 1. Store the current file position. This is the object's offset.
    pdf_monitor->obj_offsets[obj_num] = f_size(file_p);

    // 2. Write the object header (e.g., "1 0 obj\n").
    sprintf(buffer, "%d 0 obj\n", obj_num);
    status = f_puts(buffer,file_p);
    if (status < 0) return status;

    // 3. Write the main content of the object.
    status = f_puts(content,file_p);
    if (status < 0) return status;

    // 4. Write the object footer.
    status = f_puts("\nendobj\n",file_p);
	pdf_monitor->num_objs += 1;

    return status;
}

uint32_t tpdf_start_new_stream_obj(FIL* file_p, int obj_num,int obj_length_num, pdf_doc_t* pdf_monitor) {
    char buffer[64];
    UINT status;

    // 1. Store the current file position. This is the object's offset.
    pdf_monitor->obj_offsets[obj_num] = f_size(file_p);

    sprintf(buffer, "%d 0 obj\n<</Length %d 0 R\n>>\nstream\n", obj_num, obj_length_num);
    status = f_puts(buffer,file_p);
    if (status < 0) return status;
    pdf_monitor->num_objs++;

    return status;
}

uint32_t tpdf_end_new_stream_obj(FIL* file_p, int obj_length_num, int stream_length, pdf_doc_t* pdf_monitor) {
    char buffer[32];
    UINT status;
    char * end_stream = "endstream\nendobj\n";
    status = f_puts(end_stream,file_p);
    if (status < 0) return status;
    sprintf(buffer, "%d", stream_length);
    tpdf_add_new_obj(file_p,obj_length_num,buffer,pdf_monitor);
    return status;
}

