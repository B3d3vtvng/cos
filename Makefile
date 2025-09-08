# =====================================
# Toolchain
# =====================================
CC      = x86_64-elf-gcc
LD      = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
ASM     = nasm

# Flags
CFLAGS32     = -m32 -ffreestanding -O0 -g
CFLAGS64     = -m64 -ffreestanding -O0 -g
LDFLAGS32    = -m elf_i386
LDFLAGS64    = -m elf_x86_64
ASMFLAGS_BIN = -f bin
ASMFLAGS32   = -f elf32 -g -F dwarf
ASMFLAGS64   = -f elf64 -g -F dwarf

# =====================================
# Sources & Objects
# =====================================
C_SRC32   := $(wildcard kernel/kernel32/**/*.c kernel/kernel32/*.c)
C_SRC64   := $(wildcard kernel/kernel64/**/*.c kernel/kernel64/*.c)
ASM_SRC32 := $(wildcard kernel/asm32/**/*.s kernel/asm32/*.s)
ASM_SRC64 := $(wildcard kernel/asm64/**/*.s kernel/asm64/*.s)

OBJ_C32 := $(patsubst kernel/kernel32/%.c, build/kernel32/%.o, $(C_SRC32))
OBJ_A32 := $(patsubst kernel/asm32/%.s,     build/asm32/%.o,    $(ASM_SRC32))
OBJ32   := $(OBJ_C32) $(OBJ_A32)

OBJ_C64 := $(patsubst kernel/kernel64/%.c, build/kernel64/%.o, $(C_SRC64))
OBJ_A64 := $(patsubst kernel/asm64/%.s,     build/asm64/%.o,    $(ASM_SRC64))
OBJ64   := $(OBJ_C64) $(OBJ_A64)

OBJ := $(OBJ32) $(OBJ64)

# =====================================
# Default target
# =====================================
.PHONY: all clean run debug
all: build/os.img

build:
	mkdir -p build

# =====================================
# Pattern Rules
# =====================================
# 32-bit C
build/kernel32/%.o: kernel/kernel32/%.c | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS32) -c $< -o $@

# 64-bit C
build/kernel64/%.o: kernel/kernel64/%.c | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS64) -c $< -o $@

# 32-bit ASM
build/asm32/%.o: kernel/asm32/%.s | build
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS32) $< -o $@

# 64-bit ASM
build/asm64/%.o: kernel/asm64/%.s | build
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS64) $< -o $@

# If you have other asm files placed directly under kernel/, you can add:
build/%.o: kernel/%.s | build
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS32) $< -o $@

# =====================================
# Kernel
# =====================================
# NOTE: Adjust linking (64-bit vs 32-bit) to match your kernel.ld & loader expectations.
# This uses a 64-bit link; if you need an ELF32 kernel, change to LDFLAGS32 and adjust inputs.
build/kernel.elf: $(OBJ) kernel.ld
	$(LD) $(LDFLAGS64) -T kernel.ld $(OBJ) -o $@

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
	$(ASM) $(ASMFLAGS32) -D__ELF__=true -DKERN_SEC_CNT=$(KERNEL_SECTORS) -DKERNEL_BASE=0x2000 -DST2_SEC_CNT=$(STAGE2_SECTORS) $< -o build/stage2.elf
	rm -f build/stage2_temp.bin

# =====================================
# Boot sector
# =====================================
STAGE2_BIN_SECTORS = $(shell expr $$(stat -f%z build/stage2.bin) / 512 + 1)

build/boot.bin build/boot.elf: boot/boot_sec.s build/stage2.bin | build
	$(ASM) $(ASMFLAGS_BIN) -DST2_SEC_CNT=$(STAGE2_BIN_SECTORS) $< -o build/boot.bin
	$(ASM) $(ASMFLAGS32) -D__ELF__=true -DST2_SEC_CNT=$(STAGE2_BIN_SECTORS) $< -o build/boot.elf

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
