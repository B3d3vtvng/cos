# =====================================
# Toolchain
# =====================================
CC      = x86_64-elf-gcc
LD      = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
ASM     = nasm

# Flags
CFLAGS32     = -m32 -ffreestanding -O0 -g -w
CFLAGS64     = -m64 -ffreestanding -O0 -g -w
LDFLAGS32    = -m elf_i386
LDFLAGS64    = -m elf_x86_64
ASMFLAGS_BIN = -f bin
ASMFLAGS32   = -f elf32 -g -F dwarf
ASMFLAGS64   = -f elf64 -g -F dwarf

# =====================================
# Sources
# =====================================
C_SRC32   := $(wildcard kernel/kernel32/src/**/*.c kernel/kernel32/src/*.c)
C_SRC64   := $(wildcard kernel/kernel64/src/**/*.c kernel/kernel64/src/*.c)
ASM_SRC32 := $(wildcard kernel/asm/asm32/**/*.s kernel/asm/asm32/*.s)
ASM_SRC64 := $(wildcard kernel/asm/asm64/**/*.s kernel/asm/asm64/*.s)

# =====================================
# Object files
# =====================================
OBJ_C32 := $(patsubst kernel/kernel32/%.c, build/kernel32/%.o, $(C_SRC32))
OBJ_A32 := $(patsubst kernel/asm/asm32/%.s, build/asm/asm32/%.o, $(filter-out kernel/asm/asm32/long_mode_jmp.s, $(ASM_SRC32)))
OBJ32 := $(OBJ_C32) $(OBJ_A32)

OBJ_C64 := $(patsubst kernel/kernel64/%.c, build/kernel64/%.o, $(C_SRC64))
OBJ_A64 := $(patsubst kernel/asm/asm64/%.s, build/asm/asm64/%.o, $(ASM_SRC64))
OBJ64   := $(OBJ_C64) $(OBJ_A64)

# =====================================
# Default target
# =====================================
.PHONY: all clean run debug
all: build/os.img

build:
	mkdir -p build

# =====================================
# Compile rules
# =====================================
# 32-bit C
build/kernel32/%.o: kernel/kernel32/%.c | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS32) -c $< -o $@

# 64-bit C
build/kernel64/%.o: kernel/kernel64/%.c | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS64) -c $< -o $@

# 32-bit ASM (all except long_mode_jmp.s)
build/asm/asm32/%.o: kernel/asm/asm32/%.s | build
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS32) $< -o $@

# long_mode_jmp.o is built after kernel64.elf
build/asm/asm32/long_mode_jmp.o: kernel/asm/asm32/long_mode_jmp.s build/kernel64.elf
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS32) -DKMAIN_ADDR=$(KMAIN_ADDR) $< -o $@

# 64-bit ASM
build/asm/asm64/%.o: kernel/asm/asm64/%.s | build
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS64) $< -o $@

# =====================================
# Kernel
# =====================================
# 64-bit kernel
build/kernel64.elf: $(OBJ64)
	$(LD) $(LDFLAGS64) -T kernel.ld $(OBJ64) -o $@

build/kernel32_temp.elf: $(OBJ32) long_mode_jmp.s
	$(ASM) $(ASMFLAGS32) -DKMAIN_ADDR=0x00 long_mode_jmp.s -o build/kernel32/asm/long_mode_jmp.o
	$(LD) $(LDFLAGS32) $(OBJ32) build/kernel32/asm/long_mode_jmp.o -o build/kernel32_temp.elf
	$(eval KERNEL32_SIZE := $(shell stat -f %z build/kernel32_temp.elf))
	$(eval KMAIN_OFFSET := $(shell nm -g build/kernel64.elf | grep ' kmain$' | awk '{print $1}' | sed 's/^0*//' | sed 's/0x*//'))
	


# 32-bit kernel (after long_mode_jmp.o is ready)
build/kernel32.elf: kernel32_temp.elf long_mode_jmp.o $(OBJA32) 
	$(LD) $(LDFLAGS32) -T kernel.ld $(OBJ32_NOPLACEHOLDER) build/asm/asm32/long_mode_jmp.o -o $@

# Concatenate 32-bit + 64-bit kernel into binary
build/kernel.bin: build/kernel32.elf build/kernel64.elf
	$(OBJCOPY) -O binary build/kernel32.elf $@
	$(OBJCOPY) -O binary build/kernel64.elf build/kernel64.bin.tmp
	cat build/kernel64.bin.tmp >> $@
	rm -f build/kernel64.bin.tmp

# =====================================
# Stage 2 loader
# =====================================
KERNEL_SECTORS = $(shell expr $$(stat -f %z build/kernel.bin) / 512 + 1)

build/stage2_temp.bin: boot/stage2.s build/kernel.bin | build
	$(ASM) $(ASMFLAGS_BIN) -DKERN_SEC_CNT=$(KERNEL_SECTORS) -DKERNEL_BASE=0x2000 -DST2_SEC_CNT=1 $< -o $@

STAGE2_SECTORS = $(shell expr $$(stat -f %z build/stage2_temp.bin) / 512 + 1)

build/stage2.bin build/stage2.elf: boot/stage2.s build/stage2_temp.bin build/kernel.bin | build
	$(ASM) $(ASMFLAGS_BIN) -DKERN_SEC_CNT=$(KERNEL_SECTORS) -DKERNEL_BASE=0x2000 -DST2_SEC_CNT=$(STAGE2_SECTORS) $< -o build/stage2.bin
	$(ASM) $(ASMFLAGS32) -D__ELF__=true -DKERN_SEC_CNT=$(KERNEL_SECTORS) -DKERNEL_BASE=0x2000 -DST2_SEC_CNT=$(STAGE2_SECTORS) $< -o build/stage2.elf
	rm -f build/stage2_temp.bin

# =====================================
# Boot sector
# =====================================
STAGE2_BIN_SECTORS = $(shell expr $$(stat -f %z build/stage2.bin) / 512 + 1)

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
		seek=$$((1 + ( ($$(stat -f %z build/stage2.bin) + 511) / 512 ))) \
		conv=notrunc,sync

# =====================================
# Run & Debug
# =====================================
run: build/os.img
	qemu-system-x86_64 -fda $<

debug: build/os.img build/kernel64.elf build/kernel32.elf build/stage2.elf build/boot.elf
	qemu-system-x86_64 -fda build/os.img -S -s &
	sleep 1
	$(eval GDB_CMDS := \
		-ex "target remote localhost:1234" \
		-ex "add-symbol-file build/boot.elf 0x7c00" \
		-ex "add-symbol-file build/stage2.elf 0x1000" \
		-ex "add-symbol-file build/kernel32.elf 0x2000" \
		-ex "add-symbol-file build/kernel64.elf $(shell nm -g build/kernel64.elf | grep ' kmain$$' | awk '{print "0x"$$1}')" \
	)
	gdb $(GDB_CMDS) --tui

# =====================================
# Clean
# =====================================
clean:
	rm -rf build
