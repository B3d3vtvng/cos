# =====================================
# Toolchain
# =====================================
CC      = x86_64-elf-gcc
LD      = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
ASM     = nasm

CFLAGS  = -m32 -ffreestanding -O0 -g
LDFLAGS = -m elf_i386
ASMFLAGS_BIN = -f bin
ASMFLAGS_ELF = -f elf32 -g -F dwarf

# =====================================
# Sources & Objects
# =====================================
C_SRC := $(wildcard kernel/**/*.c kernel/*.c)
ASM_SRC := $(wildcard kernel/**/*.s kernel/*.s)

C_OBJ := $(patsubst kernel/%.c, build/%.o, $(C_SRC))
ASM_OBJ := $(patsubst kernel/%.s, build/%.o, $(ASM_SRC))
OBJ := $(C_OBJ) $(ASM_OBJ)

# =====================================
# Default target
# =====================================
all: build/os.img

build:
	mkdir -p build

# =====================================
# Pattern Rules
# =====================================
build/%.o: kernel/%.c | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: kernel/%.s | build
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# =====================================
# Kernel
# =====================================
build/kernel.elf: $(OBJ) kernel.ld
	$(LD) $(LDFLAGS) -T kernel.ld $(OBJ) -o $@

build/kernel.bin: build/kernel.elf
	$(OBJCOPY) -O binary --change-section-lma .text.boot=0x2000 $< $@

# =====================================
# Stage 2 loader
# =====================================
KERNEL_SECTORS = $(shell expr $$(stat -f%z build/kernel.bin) / 512 + 1)

build/stage2_temp.bin: boot/stage2.s build/kernel.bin | build
	$(ASM) $(ASMFLAGS_BIN) -DKERN_SEC_CNT=$(KERNEL_SECTORS) -DKERNEL_BASE=0x2000 -DST2_SEC_CNT=1 $< -o $@

STAGE2_SECTORS = $(shell expr $$(stat -f%z build/stage2_temp.bin) / 512 + 1)

build/stage2.bin build/stage2.elf: boot/stage2.s build/stage2_temp.bin build/kernel.bin | build
	$(ASM) $(ASMFLAGS_BIN) -DKERN_SEC_CNT=$(KERNEL_SECTORS) -DKERNEL_BASE=0x2000 -DST2_SEC_CNT=$(STAGE2_SECTORS) $< -o build/stage2.bin
	$(ASM) $(ASMFLAGS_ELF) -D__ELF__=true -DKERN_SEC_CNT=$(KERNEL_SECTORS) -DKERNEL_BASE=0x2000 -DST2_SEC_CNT=$(STAGE2_SECTORS) $< -o build/stage2.elf
	rm build/stage2_temp.bin

# =====================================
# Boot sector
# =====================================
STAGE2_BIN_SECTORS = $(shell expr $$(stat -f%z build/stage2.bin) / 512 + 1)

build/boot.bin build/boot.elf: boot/boot_sec.s build/stage2.bin | build
	$(ASM) $(ASMFLAGS_BIN) -DST2_SEC_CNT=$(STAGE2_BIN_SECTORS) $< -o build/boot.bin
	$(ASM) $(ASMFLAGS_ELF) -D__ELF__=true -DST2_SEC_CNT=$(STAGE2_BIN_SECTORS) $< -o build/boot.elf

# =====================================
# Disk image
# =====================================
build/os.img: build/boot.bin build/stage2.bin build/kernel.bin | build
	dd if=/dev/zero of=$@ bs=512 count=2880
	dd if=build/boot.bin of=$@ conv=notrunc,sync
	dd if=build/stage2.bin of=$@ bs=512 seek=1 conv=notrunc,sync
	dd if=build/kernel.bin of=$@ bs=512 \
		seek=$$((1 + ( ($$(stat -f%z build/stage2.bin) + 511) / 512 ))) \
		conv=notrunc,sync

# =====================================
# Run & Debug
# =====================================
run: build/os.img
	qemu-system-i386 -fda $<

debug: build/os.img
	qemu-system-i386 -fda $< -S -s &
	sleep 1
	gdb -ex "target remote localhost:1234" \
	    -ex "add-symbol-file build/boot.elf 0x7c00" \
	    -ex "add-symbol-file build/stage2.elf 0x1000" \
	    -ex "add-symbol-file build/kernel.elf 0x2000"

clean:
	rm -rf build
