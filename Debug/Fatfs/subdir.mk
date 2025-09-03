################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Fatfs/diskio.c \
../Fatfs/ff.c \
../Fatfs/ffsystem.c \
../Fatfs/ffunicode.c 

OBJS += \
./Fatfs/diskio.o \
./Fatfs/ff.o \
./Fatfs/ffsystem.o \
./Fatfs/ffunicode.o 

C_DEPS += \
./Fatfs/diskio.d \
./Fatfs/ff.d \
./Fatfs/ffsystem.d \
./Fatfs/ffunicode.d 


# Each subdirectory must supply rules for building sources it contributes
Fatfs/%.o Fatfs/%.su Fatfs/%.cyclo: ../Fatfs/%.c Fatfs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U083xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Fatfs -I../Core/PDFDriver -I../Drivers/STM32U0xx_HAL_Driver/Inc -I../Drivers/STM32U0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U0xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Fatfs

clean-Fatfs:
	-$(RM) ./Fatfs/diskio.cyclo ./Fatfs/diskio.d ./Fatfs/diskio.o ./Fatfs/diskio.su ./Fatfs/ff.cyclo ./Fatfs/ff.d ./Fatfs/ff.o ./Fatfs/ff.su ./Fatfs/ffsystem.cyclo ./Fatfs/ffsystem.d ./Fatfs/ffsystem.o ./Fatfs/ffsystem.su ./Fatfs/ffunicode.cyclo ./Fatfs/ffunicode.d ./Fatfs/ffunicode.o ./Fatfs/ffunicode.su

.PHONY: clean-Fatfs

