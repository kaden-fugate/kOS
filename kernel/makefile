OVMF = /opt/homebrew/Cellar/qemu/11.0.3/share/qemu/edk2-x86_64-code.fd
CC = x86_64-elf-gcc
CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone
INCLUDES = -I.

.PHONY: all boot kernel grub no-qemu qemu clean

all: qemu

boot:
	mkdir -p out/
	nasm -f elf64 boot/boot.asm -o out/boot.o

kernel: boot
	$(CC) $(CFLAGS) $(INCLUDES) -c kernel/kernel.c -o out/kernel.o
	$(CC) $(CFLAGS) $(INCLUDES) -c drivers/serial.c -o out/serial.o
	x86_64-elf-ld -n -o out/kernel.bin -T linker.ld out/boot.o out/kernel.o out/serial.o

grub: boot kernel
	mkdir -p isodir/boot/grub
	cp out/kernel.bin isodir/boot/kernel.bin
	cp grub/grub.cfg isodir/boot/grub/grub.cfg
	x86_64-elf-grub-mkrescue -o multix.iso isodir/

no-qemu: grub

qemu: grub
	qemu-system-x86_64 -cdrom multix.iso -drive if=pflash,format=raw,readonly=on,file=$(OVMF) -serial stdio

clean:
	rm -rf multix.iso out/* isodir/*
	clear