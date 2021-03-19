TITLE_ID = SKGVD2VS0
TARGET   = VD2VS0
OBJS     = main.o graphics.o font.o

LIBS = -lSceLibKernel_stub -lSceVshBridge_stub -lSceAppMgr_stub -lScePower_stub -lSceDisplay_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

%.vpk: eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE_ID) "$(TARGET)" param.sfo;
	vita-pack-vpk -s param.sfo -b eboot.bin $@;

eboot.bin: eboot.velf
	vita-make-fself -a 0x2800000000000001 $< $@

%.velf: %.elf
	vita-elf-create $< $@

eboot.elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@