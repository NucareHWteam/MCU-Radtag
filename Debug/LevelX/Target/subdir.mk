################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../LevelX/Target/lx_stm32_nor_custom_driver.c 

OBJS += \
./LevelX/Target/lx_stm32_nor_custom_driver.o 

C_DEPS += \
./LevelX/Target/lx_stm32_nor_custom_driver.d 


# Each subdirectory must supply rules for building sources it contributes
LevelX/Target/%.o LevelX/Target/%.su LevelX/Target/%.cyclo: ../LevelX/Target/%.c LevelX/Target/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U083xx -DUX_INCLUDE_USER_DEFINE_FILE -DFX_INCLUDE_USER_DEFINE_FILE -DLX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Fatfs -I../Drivers/STM32U0xx_HAL_Driver/Inc -I../Drivers/STM32U0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U0xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../FileX/App -I../FileX/Target -I../LevelX/App -I../LevelX/Target -I../Middlewares/ST/levelx/common/inc -I../Middlewares/ST/filex/common/inc -I../Middlewares/ST/filex/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-LevelX-2f-Target

clean-LevelX-2f-Target:
	-$(RM) ./LevelX/Target/lx_stm32_nor_custom_driver.cyclo ./LevelX/Target/lx_stm32_nor_custom_driver.d ./LevelX/Target/lx_stm32_nor_custom_driver.o ./LevelX/Target/lx_stm32_nor_custom_driver.su

.PHONY: clean-LevelX-2f-Target

