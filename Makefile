.SUFFIXES:
ifeq ($(strip $(DEVKITPRO)),)
$(error Set DEVKITPRO to your devkitPro installation path)
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules
APP_TITLE := MangaCAP Switch
APP_AUTHOR := MangaCAP
APP_VERSION := 0.3.3
TARGET := MangaCAP
BUILD := build
SOURCES := source
DATA :=
INCLUDES :=
ROMFS :=
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS := -g -Wall -O2 -ffunction-sections $(ARCH) $(DEFINES)
CFLAGS += $(INCLUDE)
CXXFLAGS := $(CFLAGS) -std=gnu++17 -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS := -lSDL2_image -lSDL2 -lEGL -lpng -ljpeg -lwebp -lz -lnx
LIBDIRS := $(PORTLIBS) $(LIBNX)
# SDL2 headers live in the SDL2 subdirectory of the Switch portlibs.
INCLUDES :=
ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
.PHONY: all clean
all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
$(BUILD):
	@mkdir -p $@
clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).nro $(TARGET).nacp
else
DEPENDS := $(OFILES:.o=.d)
-include $(DEPENDS)
.PHONY: all
all: $(OUTPUT).nro
$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf: $(OFILES)
	$(LINK.cpp) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
endif
