################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ALSABackend.cpp \
../ReadFiles.cpp \
../UserInterface.cpp \
../main.cpp 

OBJS += \
./ALSABackend.o \
./ReadFiles.o \
./UserInterface.o \
./main.o 

CPP_DEPS += \
./ALSABackend.d \
./ReadFiles.d \
./UserInterface.d \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
ALSABackend.o: ../ALSABackend.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/alsa -I/usr/include/libxml2 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"ALSABackend.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/alsa -I/usr/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

main.o: ../main.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/alsa -I/usr/include/libxml2 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"main.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


