CELL_MK_DIR = $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

BUILD_TYPE     	= release

LIBSTUB_DIR		= ./lib
PRX_DIR			= .
INSTALL			= cp
PEXPORTPICKUP		= ppu-lv2-prx-exportpickup
PRX_LDFLAGS_EXTRA	= -L ./lib -Wl,--strip-unused-data

CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='ecrti.o)
CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='crtbegin.o)
CRT_TAIL                += $(shell ppu-lv2-gcc -print-file-name'='crtend.o)
CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='ecrtn.o)

PPU_PRX_EXLIBS =	-lstdc_export_stub \
          		 	-lsysPrxForUser_export_stub \
             	 	-lvsh_export_stub \
               		-lpaf_export_stub \
               		-lvshmain_export_stub \
               		-lvshtask_export_stub \
              		-lallocator_export_stub \
               		-lsdk_export_stub \
               		-lxsetting_export_stub \
               		-lsys_io_export_stub \
               		-lpngdec_ppuonly_export_stub \
               		# -lsysutil_np_trophy_stub \
               		-lsysutil_stub

PPU_SRCS = printf.c main.c
PPU_PRX_TARGET = EBOOTLoader.prx
PPU_PRX_LDFLAGS += $(PRX_LDFLAGS_EXTRA)
PPU_PRX_STRIP_FLAGS = -s
# PPU_PRX_LDLIBS 	+= -lfs_stub -lnet_stub -lio_stub -lsysutil_np_trophy_stub -lvshmain-export_stub
# PPU_PRX_EXLIBS 	= -lstdc_export_stub -lsysPrxForUser_export_stub -lvsh_export_stub -lpaf_export_stub -lvshmain_export_stub -lvshtask_export_stub -lallocator_export_stub -lsdk_export_stub -lxsetting_export_stub -lsys_io_export_stub -lpngdec_ppuonly_export_stub -lsysutil_np_trophy_stub -lsysutil_stub
PPU_PRX_LDLIBS 	+= $(PPU_PRX_EXLIBS) -lfs_stub -lnet_stub -lrtc_stub -lsysmodule_stub -lio_stub
PPU_CFLAGS += -Os -ffunction-sections -fdata-sections -fno-builtin-printf -nodefaultlibs -std=gnu99 -Wno-shadow -Wno-unused-parameter

ifeq ($(BUILD_TYPE), debug)
PPU_CFLAGS += -DDEBUG 
endif

CLEANFILES = $(PRX_DIR)/$(PPU_SPRX_TARGET) *.elf *.sprx

all:
	$(MAKE) $(PPU_OBJS_DEPENDS)
	$(PPU_PRX_STRIP) --strip-debug --strip-section-header $(PPU_PRX_TARGET)
	# $(MAKE_FSELF) $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)
	f:/bkp/tools/scetool -p f:/bkp/tools/data -0 SELF -1 TRUE -s FALSE -2 04 -3 1070000052000001 -4 01000002 -5 APP -6 0003004000000000 -A 0001000000000000 -e $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)
	rm -f -r objs *.o *.prx *.sym *.elf

include $(CELL_MK_DIR)/sdk.target.mk




