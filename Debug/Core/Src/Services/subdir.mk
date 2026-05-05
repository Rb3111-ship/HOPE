################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Services/light_service.c \
../Core/Src/Services/music_service.c \
../Core/Src/Services/sensor_service.c \
../Core/Src/Services/time_service.c 

OBJS += \
./Core/Src/Services/light_service.o \
./Core/Src/Services/music_service.o \
./Core/Src/Services/sensor_service.o \
./Core/Src/Services/time_service.o 

C_DEPS += \
./Core/Src/Services/light_service.d \
./Core/Src/Services/music_service.d \
./Core/Src/Services/sensor_service.d \
./Core/Src/Services/time_service.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Services/%.o Core/Src/Services/%.su Core/Src/Services/%.cyclo: ../Core/Src/Services/%.c Core/Src/Services/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F412Rx -c -I../Core/Inc -I"D:/ST projects/Hope_V1/Core/Src/Applications" -I"D:/ST projects/Hope_V1/Drivers/OLED" -I"D:/ST projects/Hope_V1/Core/Src/UI" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I"D:/ST projects/Hope_V1/Core/Src/Drivers" -I"D:/ST projects/Hope_V1/Core/Src/Services" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Services

clean-Core-2f-Src-2f-Services:
	-$(RM) ./Core/Src/Services/light_service.cyclo ./Core/Src/Services/light_service.d ./Core/Src/Services/light_service.o ./Core/Src/Services/light_service.su ./Core/Src/Services/music_service.cyclo ./Core/Src/Services/music_service.d ./Core/Src/Services/music_service.o ./Core/Src/Services/music_service.su ./Core/Src/Services/sensor_service.cyclo ./Core/Src/Services/sensor_service.d ./Core/Src/Services/sensor_service.o ./Core/Src/Services/sensor_service.su ./Core/Src/Services/time_service.cyclo ./Core/Src/Services/time_service.d ./Core/Src/Services/time_service.o ./Core/Src/Services/time_service.su

.PHONY: clean-Core-2f-Src-2f-Services

