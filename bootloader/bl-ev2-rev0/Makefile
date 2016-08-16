# ----------------------------------------------------------------------------
#         BT Wrist Watch Project
# ----------------------------------------------------------------------------
# Copyright (c) 2009, Wrist Technology Ltd
#
# All rights reserved.
#
# ----------------------------------------------------------------------------

# 	Makefile for compiling Btwwev-1 board - bootloader (bl/mass storage)

#-------------------------------------------------------------------------------
#		User-modifiable options
#-------------------------------------------------------------------------------

# Chip & board used for compilation
# (can be overriden by adding CHIP=chip and BOARD=board to the command-line)
CHIP  = at91sam7se512

BOARD = btwwev-2

# Trace level used for compilation (NOT USED NOW)
# (can be overriden by adding TRACE_LEVEL=#number to the command-line)
# TRACE_LEVEL_DEBUG      5
# TRACE_LEVEL_INFO       4
# TRACE_LEVEL_WARNING    3
# TRACE_LEVEL_ERROR      2
# TRACE_LEVEL_FATAL      1
# TRACE_LEVEL_NO_TRACE   0
# TRACE_LEVEL = 5
TRACE_LEVEL = 0 

# Optimization level, put in comment for debugging
OPTIMIZATION = -O0


# Output file basename
OUTPUT = bl-ev2-rev0-$(BOARD)-$(CHIP)

# Compile for all memories available on the board (this sets $(MEMORIES))
# bootloader memory mapping (onchip flash)
include ../board/$(BOARD)/board-bl.mak

# Output directories
BIN = bin
OBJ = obj



#-------------------------------------------------------------------------------
#		Tools
#-------------------------------------------------------------------------------

# Tool suffix when cross-compiling
CROSS_COMPILE = arm-elf-

# Compilation tools
CC = $(CROSS_COMPILE)gcc
SIZE = $(CROSS_COMPILE)size
STRIP = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy

# Flags
INCLUDES = -I.. -I../board/$(BOARD) -I../board/$(BOARD)/$(CHIP)  
INCLUDES += -I../debug   


CFLAGS = -Wall -mlong-calls -ffunction-sections
CFLAGS += -g $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -DTRACE_LEVEL=$(TRACE_LEVEL)
ASFLAGS = -g $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -D__ASSEMBLY__
# also math lib, std lib
LDFLAGS = -g $(OPTIMIZATION) -nostartfiles -Wl,--gc-sections -lm -Xlinker -Map main.map

#-------------------------------------------------------------------------------
#		Files
#-------------------------------------------------------------------------------

# Directories where source files can be found


VPATH += ../board/$(BOARD) ../board/$(BOARD)/$(CHIP)
VPATH += ../debug  
VPATH += ../peripherals += ../peripherals/pmc += ../peripherals/oled
VPATH += ../utils
VPATH += ../device
VPATH += ../fat
VPATH += ../sdcard
VPATH += ../usb 
VPATH += ../screen

# Objects built from C source files
C_OBJECTS = main.o
C_OBJECTS += board_lowlevel.o pmc.o
C_OBJECTS += fat.o
C_OBJECTS += serial.o spi.o i2c.o
C_OBJECTS += sdcard.o
C_OBJECTS += usb_bot.o usb_drv.o usb_dsc.o usb_lun.o usb_sbc_func.o usb_std.o
C_OBJECTS += delay.o rprintf.o
C_OBJECTS += screen.o oled.o font.o
#C_OBJECTS += stdio.o math.o

# Objects built from C source files ARM mode only
C_ARMOBJ = abort.o interrupt_utils.o usb_irq.o time.o

# Objects built from Assembly source files
ASM_OBJECTS = board_cstartup-bl.o

# Append OBJ and BIN directories to output filename
OUTPUT := $(BIN)/$(OUTPUT)

#-------------------------------------------------------------------------------
#		Rules
#-------------------------------------------------------------------------------

all: $(BIN) $(OBJ) $(MEMORIES)

$(BIN) $(OBJ):
	mkdir $@

define RULES
C_ARMOBJ_$(1) = $(addprefix $(OBJ)/$(1)_, $(C_ARMOBJ))
C_OBJECTS_$(1) = $(addprefix $(OBJ)/$(1)_, $(C_OBJECTS))
ASM_OBJECTS_$(1) = $(addprefix $(OBJ)/$(1)_, $(ASM_OBJECTS))

$(1): $$(ASM_OBJECTS_$(1)) $$(C_OBJECTS_$(1)) $$(C_ARMOBJ_$(1))
	$(CC) $(LDFLAGS) -T"../board/$(BOARD)/$(CHIP)/$$@.lds" -o $(OUTPUT)-$$@.elf $$^
	$(OBJCOPY) -O binary $(OUTPUT)-$$@.elf $(OUTPUT)-$$@.bin
	$(SIZE) $$^ $(OUTPUT)-$$@.elf

$$(C_ARMOBJ_$(1)): $(OBJ)/$(1)_%.o: %.c Makefile $(OBJ) $(BIN)
	$(CC) $(CFLAGS) -D$(1) -c -o $$@ $$<

$$(C_OBJECTS_$(1)): $(OBJ)/$(1)_%.o: %.c Makefile $(OBJ) $(BIN)
	$(CC) $(CFLAGS) -D$(1) -c -o $$@ $$<

$$(ASM_OBJECTS_$(1)): $(OBJ)/$(1)_%.o: %.S Makefile $(OBJ) $(BIN)
	$(CC) $(ASFLAGS) -D$(1) -c -o $$@ $$<

debug_$(1): $(1)
	perl ../resources/gdb/debug.pl $(OUTPUT)-$(1).elf

endef

$(foreach MEMORY, $(MEMORIES), $(eval $(call RULES,$(MEMORY))))

clean:
	-rm -f $(OBJ)/*.o $(BIN)/*.bin $(BIN)/*.elf

