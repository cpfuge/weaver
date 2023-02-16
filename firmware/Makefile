TARGET = weaver

# Debug build
DEBUG = 1

# Optimization
OPT = -Og

# Build directory
BUILD_DIR = build

# Toolchain configuration
PREFIX = arm-none-eabi-

ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# Flags
CPU = -mcpu=cortex-m0plus
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32G070xx

ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# Include paths
C_INCLUDES =  \
-Isrc \
-Idrivers/STM32G0xx_HAL_Driver/Inc \
-Idrivers/STM32G0xx_HAL_Driver/Inc/Legacy \
-Idrivers/CMSIS/Device/ST/STM32G0xx/Include \
-Idrivers/CMSIS/Include

# C source files
C_SOURCES = \
src/bme280.c \
src/ccs811.c \
src/circular_buffer.c \
src/main.c \
src/software_timer.c \
src/status_led.c \
src/stm32g0xx_hal_msp.c \
src/stm32g0xx_it.c \
src/syscalls.c \
src/sysmem.c \
src/system_stm32g0xx.c \
src/uart_logger.c \
src/wifi.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_cortex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_exti.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_gpio.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_i2c.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_i2c_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_tim.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_tim_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_uart.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_uart_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_dma.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_rcc.c

# Assembly sources
ASM_SOURCES = \
src/startup_stm32g071rbtx.s

# Linker script
LDSCRIPT = STM32G071RBTX_FLASH.ld

# Libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections -u _printf_float

# Default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# List of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

# List of assembler program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@

# Clean
clean:
	-rm -fR $(BUILD_DIR)

# Dependencies
-include $(wildcard $(BUILD_DIR)/*.d)
