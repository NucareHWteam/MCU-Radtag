################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/specs.c 

OBJS += \
./Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/specs.o 

C_DEPS += \
./Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/specs.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/%.o Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/%.su Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/%.cyclo: ../Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/%.c Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U083xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Pdflib -I../Fatfs -I../Drivers/STM32U0xx_HAL_Driver/Inc -I../Drivers/STM32U0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U0xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f--2e-metadata-2f--2e-plugins-2f-org-2e-eclipse-2e-cdt-2e-make-2e-core

clean-Core-2f-Src-2f--2e-metadata-2f--2e-plugins-2f-org-2e-eclipse-2e-cdt-2e-make-2e-core:
	-$(RM) ./Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/specs.cyclo ./Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/specs.d ./Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/specs.o ./Core/Src/.metadata/.plugins/org.eclipse.cdt.make.core/specs.su

.PHONY: clean-Core-2f-Src-2f--2e-metadata-2f--2e-plugins-2f-org-2e-eclipse-2e-cdt-2e-make-2e-core

