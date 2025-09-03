################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/PDFDriver/pdflib.c 

OBJS += \
./Core/PDFDriver/pdflib.o 

C_DEPS += \
./Core/PDFDriver/pdflib.d 


# Each subdirectory must supply rules for building sources it contributes
Core/PDFDriver/%.o Core/PDFDriver/%.su Core/PDFDriver/%.cyclo: ../Core/PDFDriver/%.c Core/PDFDriver/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U083xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Core/PDFDriver -I../Drivers/STM32U0xx_HAL_Driver/Inc -I../Drivers/STM32U0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U0xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-PDFDriver

clean-Core-2f-PDFDriver:
	-$(RM) ./Core/PDFDriver/pdflib.cyclo ./Core/PDFDriver/pdflib.d ./Core/PDFDriver/pdflib.o ./Core/PDFDriver/pdflib.su

.PHONY: clean-Core-2f-PDFDriver

