################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/UI/ui_events.c \
../Core/Src/UI/ui_renderer.c \
../Core/Src/UI/ui_state.c 

OBJS += \
./Core/Src/UI/ui_events.o \
./Core/Src/UI/ui_renderer.o \
./Core/Src/UI/ui_state.o 

C_DEPS += \
./Core/Src/UI/ui_events.d \
./Core/Src/UI/ui_renderer.d \
./Core/Src/UI/ui_state.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/UI/%.o Core/Src/UI/%.su Core/Src/UI/%.cyclo: ../Core/Src/UI/%.c Core/Src/UI/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F412Rx -c -I../Core/Inc -I"D:/ST projects/Hope_V1/Core/Src/Applications" -I"D:/ST projects/Hope_V1/Drivers/OLED" -I"D:/ST projects/Hope_V1/Core/Src/UI" -I"D:/ST projects/Hope_V1/Drivers/OLED" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-UI

clean-Core-2f-Src-2f-UI:
	-$(RM) ./Core/Src/UI/ui_events.cyclo ./Core/Src/UI/ui_events.d ./Core/Src/UI/ui_events.o ./Core/Src/UI/ui_events.su ./Core/Src/UI/ui_renderer.cyclo ./Core/Src/UI/ui_renderer.d ./Core/Src/UI/ui_renderer.o ./Core/Src/UI/ui_renderer.su ./Core/Src/UI/ui_state.cyclo ./Core/Src/UI/ui_state.d ./Core/Src/UI/ui_state.o ./Core/Src/UI/ui_state.su

.PHONY: clean-Core-2f-Src-2f-UI

