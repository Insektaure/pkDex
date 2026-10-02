#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# The version is the NACP's DisplayVersion, and so what the self-updater compares
# against the release tag (v2.0.0 <-> 2.0.0) and checks a download against.
#---------------------------------------------------------------------------------
APP_TITLE	:=	pkDex
APP_VERSION	:=	2.0.0
APP_AUTHOR	:=	Insektaure
ICON		:=	resources/img/icon.jpg

TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	include

# The RomFS is staged from resources/ on every build, so only what the app
# reads ends up in the NRO (and the JSON is minified when jq is installed).
ROMFS		:=	$(BUILD)/romfs
# No fonts: text is drawn with the console's shared system fonts (pl:u).
ROMFS_CONTENT	:=	data i18n img/pokemon img/states img/pkdex_256.png changelog.txt cacert.pem

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-g -Wall -O2 -ffunction-sections -fdata-sections \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__ -DAPP_VERSION=\"$(APP_VERSION)\" -DAPP_AUTHOR=\"$(APP_AUTHOR)\"

CXXFLAGS	:= $(CFLAGS) -fno-exceptions -std=c++20

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

# curl before mbedtls before -lz: curl needs symbols from both, and the linker
# only looks forward. minizip (the image pack) needs zlib, so it sits before -lz.
#     dkp-pacman -S switch-sdl2 switch-sdl2_ttf switch-sdl2_image switch-curl \
#                   switch-mbedtls switch-zlib switch-minizip
LIBS	:=	-lcurl -lmbedtls -lmbedx509 -lmbedcrypto \
			-lSDL2_image -lSDL2_ttf -lSDL2 \
			-lfreetype -lharfbuzz -lpng16 -ljpeg -lwebp -lminizip -lz -lbz2 \
			-lEGL -lGLESv2 -lglapi -ldrm_nouveau \
			-lm -lnx

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CURDIR) $(PORTLIBS) $(LIBNX)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_ICON := $(TOPDIR)/$(ICON)
export NROFLAGS += --icon=$(APP_ICON)
export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)

ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

.PHONY: $(BUILD) clean all romfs

#---------------------------------------------------------------------------------
all: $(BUILD)

romfs:
	@rm -rf "$(CURDIR)/$(ROMFS)" && mkdir -p "$(CURDIR)/$(ROMFS)"
	@for f in $(ROMFS_CONTENT); do \
		if [ -e "$(CURDIR)/resources/$$f" ]; then \
			d="$(CURDIR)/$(ROMFS)/$$(dirname $$f)"; mkdir -p "$$d" && cp -r "$(CURDIR)/resources/$$f" "$$d/"; \
		else echo "warning: resources/$$f is missing"; fi; \
	done
	@if command -v jq >/dev/null 2>&1; then \
		find "$(CURDIR)/$(ROMFS)" -type f -name '*.json' -exec sh -c 'jq -c . "$$1" > "$$1.tmp" && mv "$$1.tmp" "$$1"' _ {} \; ; \
	fi

$(BUILD): romfs
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf


#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(OUTPUT).nro

$(OUTPUT).nro	:	$(OUTPUT).elf $(OUTPUT).nacp
	@elf2nro $< $@ $(NROFLAGS)
	@echo "built ... $(notdir $@)"

$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
