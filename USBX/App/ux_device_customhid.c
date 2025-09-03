/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ux_device_customhid.c
  * @author  MCD Application Team
  * @brief   USBX Device Custom HID applicative source file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "ux_device_customhid.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32u0xx_hal.h"
#include "stm32u0xx_hal_rtc.h"
#include "stm32u0xx_hal_rtc_ex.h"
#include "main.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static UX_SLAVE_CLASS_HID *hid_instance_global;
static uint8_t hid_get_report_buff[UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH];
static uint8_t hid_data_ready = 0; // OUT 패킷 도착시 set (인터럽트 or 콜백에서)
static uint8_t last_report_len = 0;

/**
  * @brief  Sends a custom HID Input Report to the host.
  * @param  report_buffer: Pointer to the buffer containing the data to send.
  * @param  report_len: The number of bytes to send. Must be <= 64.
  * @retval UX_SUCCESS on success, or an error code on failure.
  */
UINT USB_Send_HidReport(uint8_t *report_buffer, ULONG report_len)
{
  UINT status;
  UX_SLAVE_CLASS_HID_EVENT hid_event;

  if (_ux_system_slave->ux_system_slave_device.ux_slave_device_state != UX_DEVICE_CONFIGURED || hid_instance_global == UX_NULL)
  {
    return UX_ERROR;
  }

  if (report_buffer == UX_NULL || report_len == 0 || report_len > UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH)
  {
    return UX_INVALID_PARAMETER;
  }

  /*
   * Prepare the HID event for sending.
   */
  // 1. Point the event's buffer pointer to YOUR data buffer.
  ux_utility_memory_copy(hid_event.ux_device_class_hid_event_buffer, \
		  	  	  	  	  report_buffer,	\
						  report_len);

  // 2. Set the length of the data to send.
  hid_event.ux_device_class_hid_event_length = report_len;

  /*
   * 3. Send the event. This will queue the report for transmission on the
   * Interrupt IN endpoint.
   */
  status = ux_device_class_hid_event_set(hid_instance_global, &hid_event);

  return status;
}



//UINT USB_Get_HidReport(uint8_t *report_buffer, ULONG *report_len) {
//
//	ux_utility_memory_copy(report_buffer,hid_get_report_buff,UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH);
//	*report_len = UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH;
//	return UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH;
//}

UINT USB_Get_HidReport(uint8_t *report_buffer, ULONG *report_len) {
    if (hid_data_ready) {
        ux_utility_memory_copy(report_buffer, hid_get_report_buff, last_report_len);
        *report_len = last_report_len;
        hid_data_ready = 0;
        return 0; // 0 = Success
    } else {
        *report_len = 0;
        return 1; // No new data
    }
}

/* USER CODE END 0 */

/**
  * @brief  USBD_Custom_HID_Activate
  *         This function is called when insertion of a Custom HID device.
  * @param  hid_instance: Pointer to the hid class instance.
  * @retval none
  */
VOID USBD_Custom_HID_Activate(VOID *hid_instance)
{
  /* USER CODE BEGIN USBD_Custom_HID_Activate */
  UX_PARAMETER_NOT_USED(hid_instance);
  printf("Custom HID device activated.\n");
  hid_instance_global = (UX_SLAVE_CLASS_HID *)hid_instance;
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
  Switch_Backup_reg(MODE_Stop);
  /* USER CODE END USBD_Custom_HID_Activate */

  return;
}

/**
  * @brief  USBD_Custom_HID_Deactivate
  *         This function is called when extraction of a Custom HID device.
  * @param  hid_instance: Pointer to the hid class instance.
  * @retval none
  */
VOID USBD_Custom_HID_Deactivate(VOID *hid_instance)
{
  /* USER CODE BEGIN USBD_Custom_HID_Deactivate */
  UX_PARAMETER_NOT_USED(hid_instance);
  printf("Custom HID device deactivated.\n");
  /* USER CODE END USBD_Custom_HID_Deactivate */

  return;
}

/**
  * @brief  USBD_Custom_HID_SetFeature
  *         This function is invoked when the host sends a HID SET_REPORT
  *         to the application over Endpoint 0 (Set Feature).
  * @param  hid_instance: Pointer to the hid class instance.
  * @param  hid_event: Pointer to structure of the hid event.
  * @retval status
  */
UINT USBD_Custom_HID_SetFeature(UX_SLAVE_CLASS_HID *hid_instance,
                                UX_SLAVE_CLASS_HID_EVENT *hid_event)
{
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_Custom_HID_SetFeature */
  UX_PARAMETER_NOT_USED(hid_instance);
  UX_PARAMETER_NOT_USED(hid_event);
  printf("Custom HID Set Feature request received.\n");
  /* USER CODE END USBD_Custom_HID_SetFeature */

  return status;
}

/**
  * @brief  USBD_Custom_HID_GetReport
  *         This function is invoked when host is requesting event through
  *         control GET_REPORT request.
  * @param  hid_instance: Pointer to the hid class instance.
  * @param  hid_event: Pointer to structure of the hid event.
  * @retval status
  */
UINT USBD_Custom_HID_GetReport(UX_SLAVE_CLASS_HID *hid_instance,
                               UX_SLAVE_CLASS_HID_EVENT *hid_event)
{
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_Custom_HID_GetReport */
  UX_PARAMETER_NOT_USED(hid_instance);
  UX_PARAMETER_NOT_USED(hid_event);
  printf("Custom HID Get Report request received.\n");
  /* USER CODE END USBD_Custom_HID_GetReport */

  return status;
}

#ifdef UX_DEVICE_CLASS_HID_INTERRUPT_OUT_SUPPORT

/**
  * @brief  USBD_Custom_HID_SetReport
  *         This function is invoked when the host sends a HID SET_REPORT
  *         to the application over Endpoint OUT (Set Report).
  * @param  hid_instance: Pointer to the hid class instance.
  * @retval none
  */
VOID USBD_Custom_HID_SetReport(struct UX_SLAVE_CLASS_HID_STRUCT *hid_instance)
{
  /* USER CODE BEGIN USBD_Custom_HID_SetReport */

  UX_DEVICE_CLASS_HID_RECEIVED_EVENT hid_received_event;

  ux_utility_memory_set(&hid_received_event, 0, sizeof(UX_DEVICE_CLASS_HID_RECEIVED_EVENT));

  // FIXME: Add flag to check write event?
  if(ux_device_class_hid_receiver_event_get(hid_instance_global, &hid_received_event) == UX_SUCCESS)
  {
	ux_utility_memory_copy(hid_get_report_buff, \
							hid_received_event.ux_device_class_hid_received_event_data, \
							UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH);
    /* Free hid received event */
    ux_device_class_hid_receiver_event_free(hid_instance_global);

    last_report_len = hid_received_event.ux_device_class_hid_received_event_length;
    hid_data_ready = 1;
  }
  /* USER CODE END USBD_Custom_HID_SetReport */

  return;
}

/**
  * @brief  USBD_Custom_HID_EventMaxNumber
  *         This function to set receiver event max number parameter.
  * @param  none
  * @retval receiver event max number
  */
ULONG USBD_Custom_HID_EventMaxNumber(VOID)
{
  ULONG max_number = 0U;

  /* USER CODE BEGIN USBD_Custom_HID_EventMaxNumber */
  printf("Custom HID Event Max Number requested.\n");

  max_number = UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE;
  /* USER CODE END USBD_Custom_HID_EventMaxNumber */

  return max_number;
}

/**
  * @brief  USBD_Custom_HID_EventMaxLength
  *         This function to set receiver event max length parameter.
  * @param  none
  * @retval receiver event max length
  */
ULONG USBD_Custom_HID_EventMaxLength(VOID)
{
  ULONG max_length = 0U;

  /* USER CODE BEGIN USBD_Custom_HID_EventMaxLength */
  printf("Custom HID Event Max Length requested.\n");
   max_length = UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH;
  /* USER CODE END USBD_Custom_HID_EventMaxLength */

  return max_length;
}

#endif /* UX_DEVICE_CLASS_HID_INTERRUPT_OUT_SUPPORT */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
