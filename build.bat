arm-none-eabi-gcc -mcpu=arm1176jzf-s -fpic -ffreestanding -c boot.S -o boot.o
arm-none-eabi-gcc -fpic -ffreestanding -std=gnu99 -c kernel.c -o kernel.o -Wall -Wextra
arm-none-eabi-gcc -T linker.ld -o primalos.elf -ffreestanding -O2 -nostdlib boot.o kernel.o