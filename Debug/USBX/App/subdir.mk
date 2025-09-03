################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../USBX/App/app_usbx_device.c \
../USBX/App/ux_device_customhid.c \
../USBX/App/ux_device_descriptors.c \
../USBX/App/ux_device_msc.c 

OBJS += \
./USBX/App/app_usbx_device.o \
./USBX/App/ux_device_customhid.o \
./USBX/App/ux_device_descriptors.o \
./USBX/App/ux_device_msc.o 

C_DEPS += \
./USBX/App/app_usbx_device.d \
./USBX/App/ux_device_customhid.d \
./USBX/App/ux_device_descriptors.d \
./USBX/App/ux_device_msc.d 


# Each subdirectory must supply rules for building sources it contributes
USBX/App/%.o USBX/App/%.su USBX/App/%.cyclo: ../USBX/App/%.c USBX/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U083xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Pdflib -I../Fatfs -I../Drivers/STM32U0xx_HAL_Driver/Inc -I../Drivers/STM32U0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U0xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-USBX-2f-App

clean-USBX-2f-App:
	-$(RM) ./USBX/App/app_usbx_device.cyclo ./USBX/App/app_usbx_device.d ./USBX/App/app_usbx_device.o ./USBX/App/app_usbx_device.su ./USBX/App/ux_device_customhid.cyclo ./USBX/App/ux_device_customhid.d ./USBX/App/ux_device_customhid.o ./USBX/App/ux_device_customhid.su ./USBX/App/ux_device_descriptors.cyclo ./USBX/App/ux_device_descriptors.d ./USBX/App/ux_device_descriptors.o ./USBX/App/ux_device_descriptors.su ./USBX/App/ux_device_msc.cyclo ./USBX/App/ux_device_msc.d ./USBX/App/ux_device_msc.o ./USBX/App/ux_device_msc.su

.PHONY: clean-USBX-2f-App

