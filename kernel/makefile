CC = x86_64-elf-gcc
LD = x86_64-elf-ld

CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -I.
LDFLAGS = -n -T linker.ld

OVMF = /opt/homebrew/Cellar/qemu/11.0.3/share/qemu/edk2-x86_64-code.fd

SRC_C = $(wildcard kernel/*.c drivers/*.c mm/*.c kutil/*.c)
OBJ_C = $(patsubst %.c, out/%.o, $(notdir $(SRC_C)))

VPATH = kernel:drivers:mm:kutil

.PHONY: all boot kernel grub no-qemu qemu clean

all: qemu

out/boot.o: boot/boot.asm
	@mkdir -p out
	nasm -f elf64 $< -o $@

out/isr.o: boot/isr.asm
	@mkdir -p out
	nasm -f elf64 $< -o $@

out/%.o: %.c
	@mkdir -p out
	$(CC) $(CFLAGS) -c $< -o $@

out/kernel.bin: out/boot.o out/isr.o $(OBJ_C)
	$(LD) $(LDFLAGS) -o $@ $^

kos.iso: out/kernel.bin
	mkdir -p isodir/boot/grub
	cp out/kernel.bin isodir/boot/kernel.bin
	cp grub/grub.cfg isodir/boot/grub/grub.cfg
	x86_64-elf-grub-mkrescue -o kos.iso isodir/

grub: kos.iso

no-qemu: grub

qemu: grub
	qemu-system-x86_64 -cdrom kos.iso -m 1G \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF) -serial stdio

clean:
	rm -rf kos.iso out/* isodir/*
	clear