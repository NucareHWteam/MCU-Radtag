################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/filter.c \
../Core/Src/lcd.c \
../Core/Src/main.c \
../Core/Src/meas_data_log.c \
../Core/Src/rad_usb.c \
../Core/Src/sensor_log.c \
../Core/Src/spi_flash.c \
../Core/Src/stm32u0xx_hal_msp.c \
../Core/Src/stm32u0xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32u0xx.c \
../Core/Src/usb_msc_csv_file_log.c \
../Core/Src/usb_msc_pdf_file_log.c 

OBJS += \
./Core/Src/filter.o \
./Core/Src/lcd.o \
./Core/Src/main.o \
./Core/Src/meas_data_log.o \
./Core/Src/rad_usb.o \
./Core/Src/sensor_log.o \
./Core/Src/spi_flash.o \
./Core/Src/stm32u0xx_hal_msp.o \
./Core/Src/stm32u0xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32u0xx.o \
./Core/Src/usb_msc_csv_file_log.o \
./Core/Src/usb_msc_pdf_file_log.o 

C_DEPS += \
./Core/Src/filter.d \
./Core/Src/lcd.d \
./Core/Src/main.d \
./Core/Src/meas_data_log.d \
./Core/Src/rad_usb.d \
./Core/Src/sensor_log.d \
./Core/Src/spi_flash.d \
./Core/Src/stm32u0xx_hal_msp.d \
./Core/Src/stm32u0xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32u0xx.d \
./Core/Src/usb_msc_csv_file_log.d \
./Core/Src/usb_msc_pdf_file_log.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U083xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Pdflib -I../Fatfs -I../Drivers/STM32U0xx_HAL_Driver/Inc -I../Drivers/STM32U0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U0xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/filter.cyclo ./Core/Src/filter.d ./Core/Src/filter.o ./Core/Src/filter.su ./Core/Src/lcd.cyclo ./Core/Src/lcd.d ./Core/Src/lcd.o ./Core/Src/lcd.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/meas_data_log.cyclo ./Core/Src/meas_data_log.d ./Core/Src/meas_data_log.o ./Core/Src/meas_data_log.su ./Core/Src/rad_usb.cyclo ./Core/Src/rad_usb.d ./Core/Src/rad_usb.o ./Core/Src/rad_usb.su ./Core/Src/sensor_log.cyclo ./Core/Src/sensor_log.d ./Core/Src/sensor_log.o ./Core/Src/sensor_log.su ./Core/Src/spi_flash.cyclo ./Core/Src/spi_flash.d ./Core/Src/spi_flash.o ./Core/Src/spi_flash.su ./Core/Src/stm32u0xx_hal_msp.cyclo ./Core/Src/stm32u0xx_hal_msp.d ./Core/Src/stm32u0xx_hal_msp.o ./Core/Src/stm32u0xx_hal_msp.su ./Core/Src/stm32u0xx_it.cyclo ./Core/Src/stm32u0xx_it.d ./Core/Src/stm32u0xx_it.o ./Core/Src/stm32u0xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32u0xx.cyclo ./Core/Src/system_stm32u0xx.d ./Core/Src/system_stm32u0xx.o ./Core/Src/system_stm32u0xx.su ./Core/Src/usb_msc_csv_file_log.cyclo ./Core/Src/usb_msc_csv_file_log.d ./Core/Src/usb_msc_csv_file_log.o ./Core/Src/usb_msc_csv_file_log.su ./Core/Src/usb_msc_pdf_file_log.cyclo ./Core/Src/usb_msc_pdf_file_log.d ./Core/Src/usb_msc_pdf_file_log.o ./Core/Src/usb_msc_pdf_file_log.su

.PHONY: clean-Core-2f-Src

