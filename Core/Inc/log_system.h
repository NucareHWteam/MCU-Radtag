/*
 * log_system.h
 *
 *  Created on: Jul 10, 2025
 *      Author: dongo
 */

#ifndef INC_LOG_SYSTEM_H_
#define INC_LOG_SYSTEM_H_

#include "stdio.h"

#define LOG_LEVEL_APP (1 << 0) // Application specific logs
#define LOG_LEVEL_FX (1 << 1) // FileX
#define LOG_LEVEL_USBX_MSC (1 << 2) // USBX MSC
#define LOG_LEVEL_USBX_HID (1 << 3) // USBX HID


// Enable the log components that want to use
#define ENABLED_LOG_LEVELS (LOG_LEVEL_FX | LOG_LEVEL_APP | LOG_LEVEL_USBX_MSC | LOG_LEVEL_USBX_HID )

#define LOG(level, tag, fmt, ...) \
    do { \
        if (ENABLED_LOG_LEVELS & (level)) { \
            fprintf(stderr, "[%s] " fmt, tag, ##__VA_ARGS__); \
        } \
    } while (0)

#define LOG_FX(fmt, ...)   LOG(LOG_LEVEL_FX, "FX", fmt, ##__VA_ARGS__)
#define LOG_USB_MSC(fmt, ...)  LOG(LOG_LEVEL_USBX_MSC, "USB_MSC", fmt, ##__VA_ARGS__)
#define LOG_USB_HID(fmt, ...)  LOG(LOG_LEVEL_USBX_HID, "USB_HID", fmt, ##__VA_ARGS__)
#define LOG_APP(fmt, ...)  LOG(LOG_LEVEL_APP, "APP", fmt, ##__VA_ARGS__)

#endif /* INC_LOG_SYSTEM_H_ */
