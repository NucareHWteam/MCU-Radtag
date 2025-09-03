################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Zlib/adler32.c \
../Zlib/compress.c \
../Zlib/crc32.c \
../Zlib/deflate.c \
../Zlib/infback.c \
../Zlib/inffast.c \
../Zlib/inflate.c \
../Zlib/inftrees.c \
../Zlib/trees.c \
../Zlib/uncompr.c \
../Zlib/zutil.c 

OBJS += \
./Zlib/adler32.o \
./Zlib/compress.o \
./Zlib/crc32.o \
./Zlib/deflate.o \
./Zlib/infback.o \
./Zlib/inffast.o \
./Zlib/inflate.o \
./Zlib/inftrees.o \
./Zlib/trees.o \
./Zlib/uncompr.o \
./Zlib/zutil.o 

C_DEPS += \
./Zlib/adler32.d \
./Zlib/compress.d \
./Zlib/crc32.d \
./Zlib/deflate.d \
./Zlib/infback.d \
./Zlib/inffast.d \
./Zlib/inflate.d \
./Zlib/inftrees.d \
./Zlib/trees.d \
./Zlib/uncompr.d \
./Zlib/zutil.d 


# Each subdirectory must supply rules for building sources it contributes
Zlib/%.o Zlib/%.su Zlib/%.cyclo: ../Zlib/%.c Zlib/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U083xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Zlib -I../Pdflib -I../Fatfs -I../Drivers/STM32U0xx_HAL_Driver/Inc -I../Drivers/STM32U0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U0xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Zlib

clean-Zlib:
	-$(RM) ./Zlib/adler32.cyclo ./Zlib/adler32.d ./Zlib/adler32.o ./Zlib/adler32.su ./Zlib/compress.cyclo ./Zlib/compress.d ./Zlib/compress.o ./Zlib/compress.su ./Zlib/crc32.cyclo ./Zlib/crc32.d ./Zlib/crc32.o ./Zlib/crc32.su ./Zlib/deflate.cyclo ./Zlib/deflate.d ./Zlib/deflate.o ./Zlib/deflate.su ./Zlib/infback.cyclo ./Zlib/infback.d ./Zlib/infback.o ./Zlib/infback.su ./Zlib/inffast.cyclo ./Zlib/inffast.d ./Zlib/inffast.o ./Zlib/inffast.su ./Zlib/inflate.cyclo ./Zlib/inflate.d ./Zlib/inflate.o ./Zlib/inflate.su ./Zlib/inftrees.cyclo ./Zlib/inftrees.d ./Zlib/inftrees.o ./Zlib/inftrees.su ./Zlib/trees.cyclo ./Zlib/trees.d ./Zlib/trees.o ./Zlib/trees.su ./Zlib/uncompr.cyclo ./Zlib/uncompr.d ./Zlib/uncompr.o ./Zlib/uncompr.su ./Zlib/zutil.cyclo ./Zlib/zutil.d ./Zlib/zutil.o ./Zlib/zutil.su

.PHONY: clean-Zlib

