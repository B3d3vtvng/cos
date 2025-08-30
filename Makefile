CC = gcc
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -m elf_i386 -Ttext 0x1000 -nostdlib
ASM      = nasm
ASMFLAGS = -f bin

all: build/os.img

build:
	mkdir -p build

build/kernel_entry.bin: kernel/kernel_entry.c | build
	$(CC) $(CFLAGS) -c $< -o build/kernel_entry.o
	$(LD) $(LDFLAGS) build/kernel_entry.o -o $@
	rm build/kernel_entry.o

build/stage2.bin: boot/stage2.s | build
	$(eval KERNEL_)
	$(ASM) $(ASMFLAGS) $< -o $@

build/boot.bin: boot/boot_sec.s build/stage2.bin | build
	$(eval STAGE2_SECTORS := $(shell expr $$(stat -f%z build/stage2.bin) / 512 + 1))
	$(ASM) $(ASMFLAGS) -DST2_SEC_CNT=$(STAGE2_SECTORS) $< -o $@

build/os.img: build/boot.bin build/stage2.bin | build
	dd if=/dev/zero of=$@ bs=512 count=2880
	dd if=build/boot.bin of=$@ conv=notrunc
	dd if=build/stage2.bin of=$@ bs=512 seek=1 conv=notrunc

run: build/os.img
	qemu-system-x86_64 -fda $<

clean:
	rm -rf build