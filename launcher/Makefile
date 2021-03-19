OBJS     = main.o

LIBS = -lSceLibKernel_stub -lSceVshBridge_stub -lSceAppMgr_stub -ltaihen_stub -lScePower_stub libvdkernel_stub.a

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3
ASFLAGS = $(CFLAGS)

all: launcher.bin

launcher.bin: launcher.velf
	vita-make-fself -a 0x2800000000000001 $< $@

%.velf: %.elf
	vita-elf-create $< $@

launcher.elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@