# =====================================
# Toolchain
# =====================================
CC      = x86_64-elf-gcc
LD      = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
ASM     = nasm

# Compiler and assembler flags
CFLAGS32  = -m32 -ffreestanding -O0 -g -Wall -Wextra -Wno-pointer-to-int-cast
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
BOOT_ASM := $(wildcard boot/boot32/*.s)
BOOT_C   := $(wildcard boot/boot32/*.c)

# Fix: Explicitly define these for pattern substitution
BOOT32_ASM := $(BOOT_ASM)
BOOT32_C   := $(BOOT_C)

BOOT_ASM_OBJ := $(patsubst boot/boot32/%.s, build/%.o, $(BOOT32_ASM))
BOOT_C_OBJ   := $(patsubst boot/boot32/%.c, build/%.o, $(BOOT32_C))

C_DRIVER_SRC   := $(wildcard kernel/drivers/*.c)
C_MEM_SRC     := $(wildcard kernel/mem/*.c)
C_IDT_SRC     := $(wildcard kernel/idt/*.c)
C_UTIL_SRC    := $(wildcard kernel/util/*.c)
C_SCHED_SRC := $(wildcard kernel/sched/*.c)
C_SRC := kernel/kernel_main.c $(C_DRIVER_SRC) $(C_MEM_SRC) $(C_IDT_SRC) $(C_UTIL_SRC) $(C_SCHED_SRC)
ASM_SRC := $(wildcard kernel/**/*.s)

C_OBJ   := $(patsubst kernel/%.c, build/%.o, $(C_SRC))
ASM_OBJ := $(patsubst kernel/%.s, build/%.o, $(ASM_SRC))

OBJ := $(C_OBJ) $(ASM_OBJ)

# =====================================
# Helper: Find mke2fs on macOS
# =====================================
# Try to find e2fsprogs in Homebrew paths, fallback to system path
MKE2FS := $(shell \
	if [ -x /opt/homebrew/opt/e2fsprogs/sbin/mke2fs ]; then echo /opt/homebrew/opt/e2fsprogs/sbin/mke2fs; \
	elif [ -x /usr/local/opt/e2fsprogs/sbin/mke2fs ]; then echo /usr/local/opt/e2fsprogs/sbin/mke2fs; \
	else echo mke2fs; fi)

# =====================================
# Default target
# =====================================
all: clean build/os.img

build:
	mkdir -p build
	mkdir -p build/drivers
	mkdir -p build/mem
	mkdir -p build/idt
	mkdir -p build/util
	mkdir -p build/sched

# =====================================
# Pattern Rules
# =====================================
build/%.o: kernel/%.c | build
	$(CC) $(CFLAGS64) -c $< -o $@

build/%.o: kernel/%.s | build
	$(ASM) $(ASMFLAGS64) $< -o $@

build/%.o: boot/%.c | build
	$(CC) $(CFLAGS32) -c $< -o $@

build/%.o: boot/%.s | build
	$(ASM) $(ASMFLAGS32) $< -o $@

# =====================================
# Kernel linking
# =====================================
build/kernel.elf: $(OBJ) ld_scripts/kernel.ld | build
	$(LD) $(LDFLAGS64) -T ld_scripts/kernel.ld $(OBJ) -o $@

build/kernel.bin: build/kernel.elf
	$(OBJCOPY) -O binary $< $@

# =====================================
# Stage 2 loader
# =====================================
build/stage2.elf: boot/stage2.s | build
	$(ASM) $(ASMFLAGS32) \
		-DST2_SEC_CNT=1 \
		-DSTAGE3_SEC_CNT=3 \
		-DKERN_SEC_CNT=81 \
		-DSTAGE3_BASE=0x15000 \
		-DKERNEL_BASE=0x20000 \
		-D__ELF__ \
		$< -o $@ -g -F DWARF

build/stage2.bin: build/stage2.elf
	$(ASM) $(ASMFLAGS_BIN) \
		-DST2_SEC_CNT=1 \
		-DSTAGE3_SEC_CNT=3 \
		-DSTAGE3_BASE=0x15000 \
		-DKERNEL_BASE=0x20000 \
		-DKERN_SEC_CNT=81 \
		boot/stage2.s -o $@

# =====================================
# Boot sector
# =====================================
build/boot.elf: boot/boot_sec.s build/stage2.bin | build
	$(ASM) $(ASMFLAGS32) -DST2_SEC_CNT=1 -D__ELF__ $< -o $@ -g -F DWARF

build/boot.bin: boot/boot_sec.s build/stage2.bin | build
	$(ASM) $(ASMFLAGS_BIN) -DST2_SEC_CNT=1 $< -o $@

# =====================================
# Stage 3 loader
# =====================================
build/stage3.o : boot/stage3.c | build
	$(CC) $(CFLAGS32) -DKERNEL_PG_CNT=12 -c $< -o $@

build/stage3.elf: build/enter_long_mode.o build/stage3.o | build
	$(LD) $(LDFLAGS32) -T ld_scripts/stage3.ld -e stage3_main $^ -o $@ -g

build/stage3.bin: build/stage3.elf
	$(OBJCOPY) -O binary $< $@

# =====================================
# Sector calculations and reporting
# =====================================
# NOTE: macOS uses 'stat -f%z' for size in bytes
define calc_sectors
    $(shell expr $$(stat -f%z "$(1)") / 512 + 1)
endef

define calc_address
    $(shell printf "0x%05x" $$(($(1) * 512)))
endef

# =====================================
# Disk image (HDD + Ext2)
# =====================================
build/os.img: build/boot.bin build/stage2.bin build/stage3.bin build/kernel.bin
	@echo "Creating HDD image..."
	@echo "----------------------------------------"    

	$(eval BOOT_SECTORS := 1)
	$(eval STAGE2_SECTORS := $(call calc_sectors,build/stage2.bin))
	$(eval STAGE3_SECTORS := $(call calc_sectors,build/stage3.bin))
	$(eval KERNEL_SECTORS := $(call calc_sectors,build/kernel.bin))

	$(eval STAGE2_LBA := $(BOOT_SECTORS))
	$(eval STAGE3_LBA := $(shell python3 -c "print($(BOOT_SECTORS) + $(STAGE2_SECTORS))"))
	$(eval KERNEL_LBA := $(shell python3 -c "print($(BOOT_SECTORS) + $(STAGE2_SECTORS) + $(STAGE3_SECTORS))"))
	
	# FS_LBA is where the Raw Sectors end and the Ext2 Partition begins
	$(eval FS_LBA := $(shell python3 -c "print($(KERNEL_LBA) + $(KERNEL_SECTORS))"))

	# FS Size in sectors (32MB = 32 * 1024 * 1024 / 512 = 65536)
	$(eval FS_SECTORS := 65536)

	@echo "Boot Sector      size: $$(stat -f%z build/boot.bin) bytes ($(BOOT_SECTORS) sectors) @ LBA 0 (0x00000)"
	@echo "Stage 2         size: $$(stat -f%z build/stage2.bin) bytes ($(STAGE2_SECTORS) sectors) @ LBA $(STAGE2_LBA) ($(call calc_address,$(STAGE2_LBA)))"
	@echo "Stage 3         size: $$(stat -f%z build/stage3.bin) bytes ($(STAGE3_SECTORS) sectors) @ LBA $(STAGE3_LBA) ($(call calc_address,$(STAGE3_LBA)))"
	@echo "Kernel          size: $$(stat -f%z build/kernel.bin) bytes ($(KERNEL_SECTORS) sectors) @ LBA $(KERNEL_LBA) ($(call calc_address,$(KERNEL_LBA)))"
	@echo "Ext2 Partition  starts @ LBA $(FS_LBA) ($(call calc_address,$(FS_LBA)))"

	# 1. Create a blank 64MB HDD image file (bs=1m for BSD dd)
	dd if=/dev/zero of=$@ bs=1m count=64 2>/dev/null

	# 2. Write the Raw Binaries (Bootloader + Kernel)
	dd if=build/boot.bin of=$@ conv=notrunc 2>/dev/null
	dd if=build/stage2.bin of=$@ bs=512 seek=$(STAGE2_LBA) conv=notrunc 2>/dev/null
	dd if=build/stage3.bin of=$@ bs=512 seek=$(STAGE3_LBA) conv=notrunc 2>/dev/null
	dd if=build/kernel.bin of=$@ bs=512 seek=$(KERNEL_LBA) conv=notrunc 2>/dev/null

	# 3. Create a temporary 32MB file and format it as ext2
	dd if=/dev/zero of=build/fs.img bs=1m count=32 2>/dev/null
	
	@echo "Formatting ext2 partition..."
	$(MKE2FS) -q -t ext2 -F build/fs.img

	# 4. Write the filesystem to the HDD image immediately after the kernel
	dd if=build/fs.img of=$@ bs=512 seek=$(FS_LBA) conv=notrunc 2>/dev/null

	# 5. Create MBR Partition Table Entry (Partition 1)
	# We use python to generate the 16-byte partition entry and inject it at offset 0x1BE (446)
	# Status=0x00, CHS=Dummy, Type=0x83 (Linux), LBA Start=$(FS_LBA), Sectors=$(FS_SECTORS)
	python3 -c "import struct, sys; sys.stdout.buffer.write(struct.pack('<B3sB3sII', 0x00, b'\xFE\xFF\xFF', 0x83, b'\xFE\xFF\xFF', $(FS_LBA), $(FS_SECTORS)))" > build/mbr_entry.bin
	
	dd if=build/mbr_entry.bin of=$@ bs=1 seek=446 conv=notrunc 2>/dev/null
	
	# Add MBR Boot Signature (0x55AA) just in case boot.bin didn't include it (though it should)
	# python3 -c "import sys; sys.stdout.buffer.write(b'\x55\xAA')" | dd of=$@ bs=1 seek=510 conv=notrunc 2>/dev/null

	@echo "----------------------------------------"
	@echo "Total Raw sectors used: $(FS_LBA)"
	@echo "Disk image created: $@"

# =====================================
# Run & clean
# =====================================
run: build/os.img
	# -drive format=raw is required to simulate HDD
	qemu-system-x86_64 -drive file=$<,format=raw,index=0,media=disk

debug: build/os.img build/kernel.elf build/stage3.elf build/stage2.elf build/boot.elf
	qemu-system-x86_64 -drive file=build/os.img,format=raw,index=0,media=disk -S -s &
	sleep 1
	$(eval GDB_CMDS := \
		-ex "target remote localhost:1234" \
		-ex "add-symbol-file build/boot.elf 0x7c00" \
		-ex "add-symbol-file build/stage2.elf 0x1000" \
		-ex "add-symbol-file build/stage3.elf 0x15000" \
		-ex "add-symbol-file build/kernel.elf 0xFFFFFFFF80000000" \
	)
	gdb $(GDB_CMDS) --tui

clean:
	rm -rf build