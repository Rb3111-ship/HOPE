################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Drivers/df_player_driver.c \
../Core/Src/Drivers/gpio_driver.c 

OBJS += \
./Core/Src/Drivers/df_player_driver.o \
./Core/Src/Drivers/gpio_driver.o 

C_DEPS += \
./Core/Src/Drivers/df_player_driver.d \
./Core/Src/Drivers/gpio_driver.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Drivers/%.o Core/Src/Drivers/%.su Core/Src/Drivers/%.cyclo: ../Core/Src/Drivers/%.c Core/Src/Drivers/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F412Rx -c -I../Core/Inc -I"D:/ST projects/Hope_V1/Core/Src/Applications" -I"D:/ST projects/Hope_V1/Drivers/OLED" -I"D:/ST projects/Hope_V1/Core/Src/UI" -I"D:/ST projects/Hope_V1/Drivers/OLED" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Drivers

clean-Core-2f-Src-2f-Drivers:
	-$(RM) ./Core/Src/Drivers/df_player_driver.cyclo ./Core/Src/Drivers/df_player_driver.d ./Core/Src/Drivers/df_player_driver.o ./Core/Src/Drivers/df_player_driver.su ./Core/Src/Drivers/gpio_driver.cyclo ./Core/Src/Drivers/gpio_driver.d ./Core/Src/Drivers/gpio_driver.o ./Core/Src/Drivers/gpio_driver.su

.PHONY: clean-Core-2f-Src-2f-Drivers

