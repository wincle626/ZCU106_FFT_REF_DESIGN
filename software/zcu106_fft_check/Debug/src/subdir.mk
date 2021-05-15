################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/main.c \
../src/outbyte.c \
../src/xaxidma.c \
../src/xaxidma_bd.c \
../src/xaxidma_bdring.c \
../src/xaxidma_g.c \
../src/xaxidma_selftest.c \
../src/xaxidma_sinit.c \
../src/xil_assert.c \
../src/xil_cache.c \
../src/xil_exception.c \
../src/xil_io.c \
../src/xil_printf.c \
../src/xuartps_hw.c 

OBJS += \
./src/main.o \
./src/outbyte.o \
./src/xaxidma.o \
./src/xaxidma_bd.o \
./src/xaxidma_bdring.o \
./src/xaxidma_g.o \
./src/xaxidma_selftest.o \
./src/xaxidma_sinit.o \
./src/xil_assert.o \
./src/xil_cache.o \
./src/xil_exception.o \
./src/xil_io.o \
./src/xil_printf.o \
./src/xuartps_hw.o 

C_DEPS += \
./src/main.d \
./src/outbyte.d \
./src/xaxidma.d \
./src/xaxidma_bd.d \
./src/xaxidma_bdring.d \
./src/xaxidma_g.d \
./src/xaxidma_selftest.d \
./src/xaxidma_sinit.d \
./src/xil_assert.d \
./src/xil_cache.d \
./src/xil_exception.d \
./src/xil_io.d \
./src/xil_printf.d \
./src/xuartps_hw.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 Linux gcc compiler'
	aarch64-linux-gnu-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


