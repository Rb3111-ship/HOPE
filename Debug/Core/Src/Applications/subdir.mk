################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Applications/light_task.c \
../Core/Src/Applications/music_task.c \
../Core/Src/Applications/ui_task.c 

OBJS += \
./Core/Src/Applications/light_task.o \
./Core/Src/Applications/music_task.o \
./Core/Src/Applications/ui_task.o 

C_DEPS += \
./Core/Src/Applications/light_task.d \
./Core/Src/Applications/music_task.d \
./Core/Src/Applications/ui_task.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Applications/%.o Core/Src/Applications/%.su Core/Src/Applications/%.cyclo: ../Core/Src/Applications/%.c Core/Src/Applications/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F412Rx -c -I../Core/Inc -I"D:/ST projects/Hope_V1/Core/Src/Applications" -I"D:/ST projects/Hope_V1/Drivers/OLED" -I"D:/ST projects/Hope_V1/Core/Src/UI" -I"D:/ST projects/Hope_V1/Drivers/OLED" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Applications

clean-Core-2f-Src-2f-Applications:
	-$(RM) ./Core/Src/Applications/light_task.cyclo ./Core/Src/Applications/light_task.d ./Core/Src/Applications/light_task.o ./Core/Src/Applications/light_task.su ./Core/Src/Applications/music_task.cyclo ./Core/Src/Applications/music_task.d ./Core/Src/Applications/music_task.o ./Core/Src/Applications/music_task.su ./Core/Src/Applications/ui_task.cyclo ./Core/Src/Applications/ui_task.d ./Core/Src/Applications/ui_task.o ./Core/Src/Applications/ui_task.su

.PHONY: clean-Core-2f-Src-2f-Applications

