# =====================================
# Toolchain
# =====================================
CC      = x86_64-elf-gcc
LD      = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
ASM     = nasm

# Compiler and assembler flags
CFLAGS32  = -m32 -ffreestanding -O0 -g -Wall -Wextra
CFLAGS64  = -m64 -ffreestanding -O0 -g -Wall -Wextra -mcmodel=large -mno-red-zone
LDFLAGS32 = -m elf_i386 --oformat=elf32-i386
LDFLAGS64 = -m elf_x86_64 --oformat=elf64-x86-64
ASMFLAGS32 = -f elf32
ASMFLAGS64 = -f elf64
ASMFLAGS_BIN = -f bin

# =====================================
# Initial dummy sector counts
# =====================================
STAGE2_SECTORS_INIT  := 1
STAGE3_SECTORS_INIT  := 1
BOOT64_SECTORS_INIT  := 1
KERNEL_SECTORS_INIT  := 1

# =====================================
# Sources & Objects
# =====================================
# Boot sources
BOOT32_ASM := $(wildcard boot/boot32/*.s)
BOOT32_C   := $(wildcard boot/boot32/*.c)
BOOT64_ASM := $(wildcard boot/boot64/*.s)
BOOT64_C   := $(wildcard boot/boot64/*.c)

# Boot objects (output directly to build/)
BOOT32_ASM_OBJ := $(patsubst boot/boot32/%.s, build/%.o, $(BOOT32_ASM))
BOOT32_C_OBJ   := $(patsubst boot/boot32/%.c, build/%.o, $(BOOT32_C))
BOOT64_ASM_OBJ := $(patsubst boot/boot64/%.s, build/%.o, $(BOOT64_ASM))
BOOT64_C_OBJ   := $(patsubst boot/boot64/%.c, build/%.o, $(BOOT64_C))

# Kernel sources
C_SRC   := $(wildcard kernel/*.c)
ASM_SRC := $(wildcard kernel/asm/*.s)

# Kernel objects (output directly to build/)
C_OBJ   := $(patsubst kernel/%.c, build/%.o, $(C_SRC))
ASM_OBJ := $(patsubst kernel/asm/%.s, build/%.o, $(ASM_SRC))

# All objects
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
# Simplified pattern rules that output directly to build/
build/%.o: kernel/%.c | build
	$(CC) $(CFLAGS64) -c $< -o $@

build/%.o: kernel/asm/%.s | build
	$(ASM) $(ASMFLAGS64) $< -o $@

build/%.o: boot/boot32/%.c | build
	$(CC) $(CFLAGS32) -c $< -o $@

build/%.o: boot/boot32/%.s | build
	$(ASM) $(ASMFLAGS32) $< -o $@

build/%.o: boot/boot64/%.c | build
	$(CC) $(CFLAGS64) -c $< -o $@

build/%.o: boot/boot64/%.s | build
	$(ASM) $(ASMFLAGS64) $< -o $@

# =====================================
# Kernel linking (with explicit objects)
# =====================================
build/kernel.elf: $(OBJ) kernel.ld | build
	$(LD) $(LDFLAGS64) -T kernel.ld $(OBJ) -o $@

build/kernel.bin: build/kernel.elf
	$(OBJCOPY) -O binary $< $@

# =====================================
# Stage 2 loader
# =====================================
build/stage2.bin: boot/boot32/stage2.s | build
	$(ASM) $(ASMFLAGS_BIN) \
		-DST2_SEC_CNT=1 \
		-DSTAGE3_SEC_CNT=1 \
		-DKERN_SEC_CNT=1 \
		-DSTAGE3_BASE=0x15000 \
		-DKERNEL_BASE=0x20000 \
		$< -o $@

# =====================================
# Boot sector
# =====================================
build/boot.bin: boot/boot32/boot_sec.s build/stage2.bin | build
	$(ASM) $(ASMFLAGS_BIN) -DST2_SEC_CNT=1 $< -o $@

# =====================================
# Stage 3 loader
# =====================================
build/stage3.o : boot/boot32/stage3.c | build
	$(CC) $(CFLAGS32) -c $< -o $@

build/stage3.elf: build/enter_long_mode.o build/stage3.o $(ENTER_LONG_OBJ) | build
	$(LD) $(LDFLAGS32) -Ttext=0x15000 -e stage3_main $^ -o $@

build/stage3.bin: build/stage3.elf
	$(OBJCOPY) -O binary $< $@

# =====================================
# Disk image
# =====================================
build/os.img: build/boot.bin build/stage2.bin build/stage3.bin build/kernel.bin
	dd if=/dev/zero of=$@ bs=512 count=2880
	dd if=build/boot.bin of=$@ conv=notrunc
	dd if=build/stage2.bin of=$@ bs=512 seek=1 conv=notrunc
	dd if=build/stage3.bin of=$@ bs=512 seek=2 conv=notrunc
	dd if=build/kernel.bin of=$@ bs=512 seek=3 conv=notrunc

# =====================================
# Run & clean
# =====================================
run: build/os.img
	qemu-system-x86_64 -fda $<

clean:
	rm -rf build
