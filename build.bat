arm-none-eabi-gcc -mcpu=arm1176jzf-s -fpic -ffreestanding -c boot.S -o boot.o
arm-none-eabi-gcc -fpic -ffreestanding -std=gnu99 -c kernel.c -o kernel.o -O2 -Wall -Wextra
arm-none-eabi-gcc -T linker.ld -o primalos.elf -ffreestanding -O2 -nostdlib boot.o kernel.o
qemu-system-arm -m 512 -M raspi0 -serial stdio -kernel "D:\Spring 2022\CSE 496 - Graduation Project 2\Project Repo\primalos.elf"