################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FileX/App/app_filex.c 

OBJS += \
./FileX/App/app_filex.o 

C_DEPS += \
./FileX/App/app_filex.d 


# Each subdirectory must supply rules for building sources it contributes
FileX/App/%.o FileX/App/%.su FileX/App/%.cyclo: ../FileX/App/%.c FileX/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U083xx -DUX_INCLUDE_USER_DEFINE_FILE -DFX_INCLUDE_USER_DEFINE_FILE -DLX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Fatfs -I../Drivers/STM32U0xx_HAL_Driver/Inc -I../Drivers/STM32U0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U0xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../FileX/App -I../FileX/Target -I../LevelX/App -I../LevelX/Target -I../Middlewares/ST/levelx/common/inc -I../Middlewares/ST/filex/common/inc -I../Middlewares/ST/filex/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-FileX-2f-App

clean-FileX-2f-App:
	-$(RM) ./FileX/App/app_filex.cyclo ./FileX/App/app_filex.d ./FileX/App/app_filex.o ./FileX/App/app_filex.su

.PHONY: clean-FileX-2f-App

