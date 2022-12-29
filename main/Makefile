OBJS     = main.o graphics.o font.o unzip.o ioapi.o promote.o sha1.o

LIBS = -lSceLibKernel_stub -lSceCtrl_stub -lSceVshBridge_stub -lSceAppMgr_stub -ltaihen_stub -lScePower_stub -lSceDisplay_stub -lSceHttp_stub -lSceNet_stub -lSceNetCtl_stub -lSceSysmodule_stub libvdkernel_stub.a -lz -lScePromoterUtil_stub_weak

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -Wno-multichar -O3
ASFLAGS = $(CFLAGS)

all: main.bin

main.bin: main.velf
	vita-make-fself -a 0x2800000000000001 $< $@

%.velf: %.elf
	vita-elf-create $< $@

main.elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@