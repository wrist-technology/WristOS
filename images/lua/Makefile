#
# Dynawa TCH -  Makefile for the Dynawa TCH boot image
#

# the name for your image...will result in IMAGE_NAME.bin
IMAGE_NAME = lua
# set DEBUG to -g to include debug info, otherwise leave blank
DEBUG_OPTS = 
# available optimization levels: -O0, -O1, -O2, -O3, -Os
#OPTIMIZATION = -Os -ffast-math
OPTIMIZATION = -Os -ffast-math
# -S (asm listing)

BUILDID=$(shell date +%Y%m%d)

CONFIG_OPTS = \
	-DBUILDID=$(BUILDID) \
	-Dusb_CDCMSD \
	-D_USB_COMPOSITE \
	-DCFG_USB_CDC_MSD_SWITCH \
	-DCFG_FATFS \
	-DCFG_USB \
	-DCFG_USB_SER \
	-DCFG_RTC \
	-DBUTTON_TASK \
	-DCFG_PM \
	-DCFG_SCHEDULER_RTT \
	-DCFG_DEEP_SLEEP \
	-DCFG_CONSOLE_USB

ROOT = ../..
CORE = ../../core
FREERTOS = ../../core/freertos
#FREERTOS = ../../core/freertos-6.05/Source

# lwIP/lwBT
_LWIP = ../../core/lwip
_LWBT = $(LWIP)/src/netif/lwbt

# lwBT
#LWBT = ../../core/lwbt
LWBT = ../../core/lwalkera-lwBT-f64e170

USB = ../../core/usb
DYNAWA = ../../core/dynawa
LIBRARIES = ../../libraries
STARTUP = ../../core/startup
DEBUG = ../../core/debug
PERIPHERALS = ../../core/peripherals
BOARD = ../../core/board
UTILS = ../../core/utils
LUA = ../../core/lua-5.1.4.lnum/src
SCREEN = ../../core/screen
YABCSP = ../../core/yabcsp/src
MONITOR = ../../core/monitor
ZLIB = ../../libraries/zlib-1.2.3
LIBPNG = ../../libraries/libpng-1.4.0

# Source files that can be built in THUMB mode.
#$(DYNAWA)/SAM7_EMAC.c

#  $(LUA)/luac.c
#  $(LUA)/lua.c
#  $(DYNAWA)/usbserial.c

#  $(LWBT)/lwip/sys.c 
#  $(LWBT)/lwbt/bcspif.c
#  $(LWBT)/arch/sys_arch.c
#  $(LWBT)/lwip/mem.c
#  $(LWBT)/bt_spp.c
LWBT_SRC = \
  $(LWBT)/lwbt/fcs.c \
  $(LWBT)/lwbt/hci.c \
  $(LWBT)/lwbt/l2cap.c \
  $(LWBT)/lwbt/lwbt_memp.c \
  $(LWBT)/lwbt/rfcomm.c \
  $(LWBT)/lwbt/sdp.c \
  $(LWBT)/lwip/inet.c \
  $(LWBT)/lwip/memp.c \
  $(LWBT)/lwip/pbuf.c \
  $(LWBT)/lwip/stats.c

LWBT_INC = \
  -I$(LWBT)

_LWBT_SRC = \
  $(LWIP)/src/core/tcp_out.c \
  $(LWIP)/src/core/ipv4/inet.c \
  $(LWIP)/src/core/mem.c \
  $(LWIP)/src/core/memp.c \
  $(LWIP)/src/core/netif.c \
  $(LWIP)/src/core/pbuf.c \
  $(LWIP)/src/core/raw.c \
  $(LWIP)/src/core/stats.c \
  $(LWIP)/src/core/sys.c \
  $(LWIP)/src/core/tcp.c \
  $(LWIP)/src/core/tcp_in.c \
  $(LWIP)/src/core/init.c \
  $(LWIP)/src/core/ipv4/ip.c \
  $(LWIP)/src/core/ipv4/ip_addr.c \
  $(LWIP)/src/core/ipv4/icmp.c \
  $(LWIP)/src/core/ipv4/inet_chksum.c \
  $(LWIP)/src/core/dhcp.c \
  $(LWIP)/src/core/dns.c \
  $(LWIP)/src/api/tcpip.c \
  $(LWIP)/src/api/api_msg.c \
  $(LWIP)/src/api/err.c \
  $(LWIP)/src/api/netbuf.c \
  $(LWIP)/src/api/api_lib.c \
  $(LWIP)/src/netif/etharp.c \
  $(LWIP)/contrib/port/FreeRTOS/AT91SAM7X/sys_arch.c \
  $(LWIP)/src/core/udp.c \
  $(LWIP)/src/core/ipv4/ip_frag.c \
  $(LWBT)/src/core/fcs.c \
  $(LWBT)/src/core/hci.c \
  $(LWBT)/src/core/l2cap.c \
  $(LWBT)/src/core/lwbt_memp.c \
  $(LWBT)/src/core/ppp.c \
  $(LWBT)/src/core/rfcomm.c \
  $(LWBT)/src/core/sdp.c

_LWBT_INC = \
  -I$(LWIP)/src/include  \
  -I$(LWIP)/contrib/port/FreeRTOS/AT91SAM7X \
  -I$(LWIP)/src/include/ipv4 \
  -I$(DYNAWA)/lwbt

USB_COMMON_CDC_SRC= \
  $(USB)/common/cdc/CDCSetControlLineStateRequest.c \
  $(USB)/common/cdc/CDCLineCoding.c

#MV
USB_COMMON_MSD_SRC= \
  $(CORE)/memories/Media.c \
  $(CORE)/memories/MEDSD.c \
  $(USB)/device/massstorage/MSDLun.c \
  $(USB)/device/massstorage/SBCMethods.c \
  $(USB)/device/massstorage/MSDDStateMachine.c

# composite USB driver (CDC+MSD)
_USB_SRC= \
  $(USB_COMMON_CDC_SRC) \
  $(USB_COMMON_MSD_SRC) \
  $(USB)/device/composite/MSDDFunctionDriver.c \
  $(USB)/device/composite/COMPOSITEDDriver.c \
  $(USB)/device/composite/COMPOSITEDDriverDescriptors.c \
  $(USB)/device/composite/CDCDFunctionDriver.c

# separate USB drivers (CDC+MSD)
USB_CDC_SRC= \
  $(USB)/device/cdc-serial/CDCDSerialDriver.c \
  $(USB)/device/cdc-serial/CDCDSerialDriverDescriptors.c

#MV
USB_MSD_SRC= \
  $(USB)/device/massstorage/MSDDriver.c \
  $(USB)/device/massstorage/MSDDriverDescriptors.c

#  $(USB)/device/core/USBDDriverCb_CfgChanged.c
USB_SRC= \
  $(USB_COMMON_CDC_SRC) \
  $(USB_COMMON_MSD_SRC) \
  $(USB_CDC_SRC) \
  $(USB_MSD_SRC) \
  $(USB)/device/core/USBDCallbacks_Resumed.c \
  $(USB)/device/core/USBDCallbacks_Suspended.c \
  $(USB)/device/core/USBDDriverCb_IfSettingChanged.c
	
_FAT_SRC= \
  $(CORE)/fat/fat.c

_FAT_SRC= \
  $(CORE)/fatfs-0.04/ff.c \
  $(CORE)/fatfs-0.04/disk.c

_FAT_INC= \
  -I$(CORE)/fatfs-0.04

#FATFS_VER = 0.07
FATFS_VER = 0.08a

FAT_SRC= \
  $(CORE)/fatfs-$(FATFS_VER)/src/option/ccsbcs.c \
  $(CORE)/fatfs-$(FATFS_VER)/src/diskio.c \
  $(CORE)/fatfs-$(FATFS_VER)/src/ff.c

FAT_INC= \
  -I$(CORE)/fatfs-$(FATFS_VER)/src

MONITOR_SRC = \
  $(MONITOR)/args.c \
  $(MONITOR)/argsdispatch.c \
  $(MONITOR)/monitor.c

#  $(UTILS)/time.c
THUMB_SRC= \
  ../../core/startup/extras.c \
  $(FREERTOS)/tasks.c \
  $(FREERTOS)/queue.c \
  $(FREERTOS)/list.c \
  $(FREERTOS)/portable/GCC/ARM7_AT91SAM7S/port.c \
  $(FREERTOS)/portable/MemMang/heap_3.c \
  $(USB_SRC) \
  $(USB)/device/core/USBDCallbacks_Initialized.c \
  $(USB)/device/core/USBDCallbacks_Reset.c \
  $(USB)/device/core/USBDDriver.c \
  $(USB)/common/core/USBSetAddressRequest.c \
  $(USB)/common/core/USBGenericDescriptor.c \
  $(USB)/common/core/USBGenericRequest.c \
  $(USB)/common/core/USBGetDescriptorRequest.c \
  $(USB)/common/core/USBSetConfigurationRequest.c \
  $(USB)/common/core/USBFeatureRequest.c \
  $(USB)/common/core/USBEndpointDescriptor.c \
  $(USB)/common/core/USBConfigurationDescriptor.c \
  $(USB)/common/core/USBInterfaceRequest.c \
  $(UTILS)/rprintf.c \
  $(UTILS)/delay.c \
  $(UTILS)/wav.c \
  $(UTILS)/math.c \
  $(UTILS)/fastfixmath.c \
  $(SCREEN)/font.c \
  $(LUA)/lapi.c \
  $(LUA)/lauxlib.c \
  $(LUA)/lbaselib.c \
  $(LUA)/lcode.c \
  $(LUA)/ldblib.c \
  $(LUA)/ldebug.c \
  $(LUA)/ldump.c \
  $(LUA)/lfunc.c \
  $(LUA)/lgc.c \
  $(LUA)/linit.c \
  $(LUA)/liolib.c \
  $(LUA)/llex.c \
  $(LUA)/lmathlib.c \
  $(LUA)/lmem.c \
  $(LUA)/loadlib.c \
  $(LUA)/lobject.c \
  $(LUA)/lopcodes.c \
  $(LUA)/loslib.c \
  $(LUA)/lparser.c \
  $(LUA)/lstate.c \
  $(LUA)/lstring.c \
  $(LUA)/lstrlib.c \
  $(LUA)/ltable.c \
  $(LUA)/ltablib.c \
  $(LUA)/ltm.c \
  $(LUA)/lundump.c \
  $(LUA)/lvm.c \
  $(LUA)/lzio.c \
  $(LUA)/print.c \
  $(LUA)/lnum.c \
  $(DYNAWA)/rtos.c \
  $(DYNAWA)/usbserial.c \
  $(DYNAWA)/serial.c \
  $(DYNAWA)/main.c \
  $(DYNAWA)/pm.c \
  $(DYNAWA)/sleep.c \
  $(DYNAWA)/io.c \
  $(DYNAWA)/led.c \
  $(DYNAWA)/lua/lua.c \
  $(DYNAWA)/lua/lib_dynawa.c \
  $(DYNAWA)/lua/lib_dynawa_timer.c \
  $(DYNAWA)/lua/lib_dynawa_bitmap.c \
  $(DYNAWA)/lua/lib_dynawa_file.c \
  $(DYNAWA)/lua/lib_dynawa_time.c \
  $(DYNAWA)/lua/lib_dynawa_bt.c \
  $(DYNAWA)/lua/lib_dynawa_audio.c \
  $(DYNAWA)/lua/lib_dynawa_x.c \
  $(DYNAWA)/event.c \
  $(DYNAWA)/timer.c \
  $(DYNAWA)/analogin.c \
  $(DYNAWA)/display.c \
  $(DYNAWA)/button.c \
  $(DYNAWA)/battery.c \
  $(DYNAWA)/accel.c \
  $(DYNAWA)/audio.c \
  $(DYNAWA)/fat.c \
  $(DYNAWA)/usb.c \
  $(CORE)/sdcard/sdcard.c \
  $(FAT_SRC) \
  $(YABCSP)/crc.c \
  $(YABCSP)/init.c \
  $(YABCSP)/le.c \
  $(YABCSP)/rx.c \
  $(YABCSP)/txmsg.c \
  $(YABCSP)/txslip.c \
  $(DYNAWA)/bt/bt.c \
  $(DYNAWA)/bt/abcsp_support_functions.c \
  $(DYNAWA)/bt/uSched.c \
  $(DYNAWA)/bt/SerialCom.c \
  $(DYNAWA)/bt/bt_spp.c \
  $(PERIPHERALS)/serial.c \
  $(PERIPHERALS)/spi.c \
  $(PERIPHERALS)/ssc.c \
  $(PERIPHERALS)/dac7311.c \
  $(PERIPHERALS)/pmc/pmc.c \
  $(PERIPHERALS)/usart/usart.c \
  $(PERIPHERALS)/aic/aic.c \
  $(PERIPHERALS)/pio/pio.c \
  $(PERIPHERALS)/rtc.c \
  $(PERIPHERALS)/i2c.c \
  $(PERIPHERALS)/accel.c \
  $(PERIPHERALS)/gasgauge.c \
  $(PERIPHERALS)/amblightsensor.c \
  $(PERIPHERALS)/ledrgb.c \
  $(PERIPHERALS)/vibrator.c \
  $(SCREEN)/screen.c \
  $(LWBT_SRC) \
  $(ZLIB)/adler32.c \
  $(ZLIB)/compress.c \
  $(ZLIB)/crc32.c \
  $(ZLIB)/gzio.c \
  $(ZLIB)/uncompr.c \
  $(ZLIB)/deflate.c \
  $(ZLIB)/trees.c \
  $(ZLIB)/zutil.c \
  $(ZLIB)/inflate.c \
  $(ZLIB)/infback.c \
  $(ZLIB)/inftrees.c \
  $(ZLIB)/inffast.c \
  $(LIBPNG)/png.c \
  $(LIBPNG)/pngerror.c \
  $(LIBPNG)/pngget.c \
  $(LIBPNG)/pngmem.c \
  $(LIBPNG)/pngpread.c \
  $(LIBPNG)/pngread.c \
  $(LIBPNG)/pngrio.c \
  $(LIBPNG)/pngrtran.c \
  $(LIBPNG)/pngrutil.c \
  $(LIBPNG)/pngset.c \
  $(LIBPNG)/pngtrans.c \
  $(LIBPNG)/pngwio.c \
  $(LIBPNG)/pngwrite.c \
  $(LIBPNG)/pngwtran.c \
  $(LIBPNG)/pngwutil.c \
  $(DYNAWA)/png.c \
  image.c

#$(DYNAWA)/spi.cpp
#$(DYNAWA)/rtos.cpp
#$(STARTUP)/extras_cpp.cpp
CPP_THUMB_SRC = 

# Source files that must be built in ARM mode.
#$(DYNAWA)/SAM7_EMAC_ISR.c \

#  $(USB)/device/core/USBD_OTGHS.c 
#  $(USB)/device/core/USBD_UDPHS.c

ARM_SRC= \
  ../../core/startup/abort.c \
  $(DYNAWA)/isr.c \
  $(DYNAWA)/sys_isr.c \
  $(FREERTOS)/portable/GCC/ARM7_AT91SAM7S/portISR.c \
  $(USB)/device/core/USBD_UDP.c \
  $(DYNAWA)/serial_isr.c \
  $(DYNAWA)/timer_isr.c \
  $(DYNAWA)/analogin_isr.c \
  $(DYNAWA)/audio_isr.c \
  $(PERIPHERALS)/spi_isr.c \
  $(PERIPHERALS)/i2c_isr.c \
  $(PERIPHERALS)/oled/oled.c \
  $(DYNAWA)/bitmap.c \
  $(SCREEN)/screen_fast.c \
  $(LUA)/ldo.c \
  $(DYNAWA)/io_isr.c
  
CPP_ARM_SRC =


# All the include directories need to be appended here
#-I$(UTILS)
INCLUDEDIRS = \
-I.  \
-I$(ROOT)/include  \
-I$(CORE) \
-I$(DYNAWA)  \
-I$(LIBRARIES)/appled  \
-I$(LIBRARIES)/digitalin  \
-I$(LIBRARIES)/digitalout  \
-I$(LIBRARIES)/dipswitch  \
-I$(LIBRARIES)/motor  \
-I$(LIBRARIES)/pwmout  \
-I$(LIBRARIES)/servo  \
-I$(LIBRARIES)/stepper  \
-I$(LIBRARIES)/webserver  \
-I$(LIBRARIES)/xbee  \
-I$(FREERTOS)/include  \
-I$(FREERTOS)/portable/GCC/ARM7_AT91SAM7S  \
 $(LWBT_INC) \
-I$(USB)/device/cdc-serial \
-I$(USB)/device/core \
-I$(USB)/common/core \
-I$(USB)/common/cdc \
-I$(BOARD) \
-I$(LUA) \
 $(FAT_INC) \
-I$(CORE)/memories \
-I$(CORE)/yabcsp/src \
-I$(DYNAWA)/bt/ \
-I$(PERIPHERALS) \
-I$(ZLIB) \
-I$(LIBPNG) \
-I$(DEBUG)

############################################################################
# Below here should not change from project to project

CC=arm-elf-gcc
CPP=arm-elf-g++
OBJCOPY=arm-elf-objcopy
OBJDUMP=arm-elf-objdump
ARCH=arm-elf-ar
CRT0=$(STARTUP)/AT91SAM7_Startup.s $(STARTUP)/crt0.s
LDSCRIPT=$(STARTUP)/dynawa.ld

# flags common to both c and c++
# ARM THUMB code options

#-march=armv4t
#-mlong-calls

C_AND_CPP_FLAGS = \
$(INCLUDEDIRS) \
-Wall  \
-Wextra  \
-Wno-strict-aliasing  \
-fno-builtin-printf \
-mcpu=arm7tdmi \
-mthumb-interwork \
-D SAM7_GCC  \
-D THUMB_INTERWORK \
-D__FLASH_BUILD \
-DSUPERVISOR_START \
-T$(LDSCRIPT) \
$(DEBUG_OPTS)  \
$(CONFIG_OPTS) \
$(OPTIMIZATION)

# ARM code options
_C_AND_CPP_FLAGS = \
$(INCLUDEDIRS) \
-Wall  \
-Wextra  \
-Wno-strict-aliasing  \
-fno-builtin-printf \
-D SAM7_GCC  \
-D__FLASH_BUILD \
-DSUPERVISOR_START \
-mcpu=arm7tdmi  \
-T$(LDSCRIPT) \
$(DEBUG_OPTS)  \
$(CONFIG_OPTS) \
$(OPTIMIZATION)

CFLAGS = ${C_AND_CPP_FLAGS}
CFLAGS += -Wmissing-prototypes -Wmissing-declarations

CPPFLAGS = ${C_AND_CPP_FLAGS}
CPPFLAGS += -fno-rtti -fno-exceptions -fno-unwind-tables

THUMB_FLAGS=-mthumb
LINKER_FLAGS=-N -Xlinker -ooutput/$(IMAGE_NAME).elf -Xlinker -M -Xlinker -Map=output/$(IMAGE_NAME)_o.map

ARM_OBJ = $(ARM_SRC:.c=.o)
THUMB_OBJ = $(THUMB_SRC:.c=.o)
CPP_THUMB_OBJ = $(CPP_THUMB_SRC:.cpp=.o)
CPP_ARM_OBJ = $(CPP_ARM_SRC:.cpp=.o)

all: output/$(IMAGE_NAME).bin
  
output/$(IMAGE_NAME).bin : output/$(IMAGE_NAME).elf
	$(OBJCOPY) output/$(IMAGE_NAME).elf -O binary output/$(IMAGE_NAME).bin
	cp output/$(IMAGE_NAME).bin sd_card/main.bin
	
output/$(IMAGE_NAME).elf : $(ARM_OBJ) $(THUMB_OBJ) $(CPP_ARM_OBJ) $(CPP_THUMB_OBJ) $(CRT0)
	$(CC) $(CFLAGS) $(ARM_OBJ) $(THUMB_OBJ) $(CPP_ARM_OBJ) $(CPP_THUMB_OBJ) -nostartfiles -x assembler-with-cpp $(CRT0) $(LINKER_FLAGS) -lm

$(THUMB_OBJ) : %.o : %.c config.h
	$(CC) -c $(THUMB_FLAGS) $(CFLAGS) $< -o $@

$(CPP_THUMB_OBJ) : %.o : %.cpp config.h
	$(CPP) -c $(THUMB_FLAGS) $(CPPFLAGS) $< -o $@

$(ARM_OBJ) : %.o : %.c config.h
	$(CC) -c $(CFLAGS) $< -o $@

$(CPP_ARM_OBJ) : %.o : %.cpp config.h
	$(CPP) -c $(CPPFLAGS) $< -o $@

clean :
	#rm -f $(ARM_OBJ)
	#rm -f $(THUMB_OBJ)
	#rm -f $(CPP_ARM_OBJ)
	#rm -f $(CPP_THUMB_OBJ)
	find ../.. -name '*.o' -exec rm {} \;
	rm -f output/$(IMAGE_NAME).elf
	rm -f output/$(IMAGE_NAME).bin
	rm -f output/$(IMAGE_NAME)_o.map

dump :
	$(OBJDUMP) -S output/$(IMAGE_NAME).elf > output/$(IMAGE_NAME).lst
	$(OBJDUMP) --all output/$(IMAGE_NAME).elf > output/$(IMAGE_NAME).sym

