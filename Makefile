# Cross compiler toolchain
CC      = x86_64-elf-gcc
LD      = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
ASM     = nasm

CFLAGS  = -m32 -ffreestanding -O0 -Wall -Wextra -g
LDFLAGS = -m elf_i386
ASMFLAGS_BIN = -f bin
ASMFLAGS_ELF = -f elf32 -g -F dwarf

all: build/os.img

build:
	mkdir -p build

# =========================
# Drivers
# =========================

build/vga_control.o: kernel/io/vga_control.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# =========================
# Kernel
# =========================
build/kernel.o: kernel/kernel_main.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/kernel.elf: build/vga_control.o build/kernel.o kernel.ld
	$(LD) $(LDFLAGS) -T kernel.ld $< -o $@

build/kernel.bin: build/kernel.elf
	$(OBJCOPY) -O binary --change-section-lma .text.boot=0x2000 build/kernel.elf build/kernel.bin

# =========================
# Stage 2 loader
# =========================
# First pass with dummy sector count
build/stage2_temp.bin: boot/stage2.s build/kernel.bin | build
	$(eval KERNEL_SECTORS := $(shell expr $$(stat -f%z build/kernel.bin) / 512 + 1))
	$(ASM) $(ASMFLAGS_BIN) -DKERN_SEC_CNT=$(KERNEL_SECTORS) -DKERNEL_BASE=0x2000 -DST2_SEC_CNT=1 $< -o $@

# Final pass with real size
build/stage2.bin build/stage2.elf: boot/stage2.s build/stage2_temp.bin build/kernel.bin | build
	$(eval KERNEL_SECTORS := $(shell expr $$(stat -f%z build/kernel.bin) / 512 + 1))
	$(eval STAGE2_SECTORS := $(shell expr $$(stat -f%z build/stage2_temp.bin) / 512 + 1))
	# Binary for booting
	$(ASM) $(ASMFLAGS_BIN) -DKERN_SEC_CNT=$(KERNEL_SECTORS) -DKERNEL_BASE=0x2000 -DST2_SEC_CNT=$(STAGE2_SECTORS) $< -o build/stage2.bin
	# ELF with debug info
	$(ASM) $(ASMFLAGS_ELF) -D__ELF__=true -DKERN_SEC_CNT=$(KERNEL_SECTORS) -DKERNEL_BASE=0x2000 -DST2_SEC_CNT=$(STAGE2_SECTORS) $< -o build/stage2.elf
	rm build/stage2_temp.bin

# =========================
# Boot sector
# =========================
build/boot.bin build/boot.elf: boot/boot_sec.s build/stage2.bin | build
	$(eval STAGE2_SECTORS := $(shell expr $$(stat -f%z build/stage2.bin) / 512 + 1))
	# Binary for booting
	$(ASM) $(ASMFLAGS_BIN) -DST2_SEC_CNT=$(STAGE2_SECTORS) $< -o build/boot.bin
	# ELF with debug info
	$(ASM) $(ASMFLAGS_ELF) -D__ELF__= -DST2_SEC_CNT=$(STAGE2_SECTORS) $< -o build/boot.elf

# =========================
# Disk image
# =========================
build/os.img: build/boot.bin build/stage2.bin build/kernel.bin | build
	dd if=/dev/zero of=$@ bs=512 count=2880
	dd if=build/boot.bin of=$@ conv=notrunc,sync
	dd if=build/stage2.bin of=$@ bs=512 seek=1 conv=notrunc,sync
	dd if=build/kernel.bin of=$@ bs=512 \
	seek=$$((1 + ( ($$(stat -f%z build/stage2.bin) + 511) / 512 ))) \
   	conv=notrunc,sync


# =========================
# Run & Debug
# =========================
run: build/os.img
	qemu-system-i386 -fda $<

debug: build/os.img
	qemu-system-i386 -fda $< -S -s &
	sleep 1
	gdb -ex "target remote localhost:1234" -ex "add-symbol-file build/boot.elf 0x7c00" -ex "add-symbol-file build/stage2.elf 0x1000" -ex "add-symbol-file build/kernel.elf 0x2000"

clean:
	rm -rf build
