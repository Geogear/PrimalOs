# PRIMAL OS
A barebones operating system developed for raspberry pi zero model as GTU CENG graduation project II.

# BEWARE
This is a very specific implementation, system is only for raspberry pi zero (would probably work with model 1 as well but I'm not testing with it, so can't say %100), it doesn't handle general cases, mostly it is set to handle cases that are known that is going to happen. Example;
- Screen resolution is hardcoded to 1920x1080, if you want to change it, you need to change the framebuffer width and height, in **src/kernel/framebuffer.c**

Also, this is a **Work In Progress**, so there can be minor and major changes throughout and code is dirty, it's filled with commented out code sections, and many printing statements that are used for debugging.

# BUILDING
Build directory gives you everything you need, you can just use a text editor and build from the terminal, but first you need to have arm cross compiler since most probably your computer runs on a processor that's based on the x86_64 architecture, you can download one for your system from [here](https://developer.arm.com/downloads/-/gnu-rm). Makefile assumes path to the compiler is in your environment variables.

Another important thing is, if you are building for the qemu-arm emulator, you have to set the **ON_EMU** macro in **kernel.h** to one, or zero if you are going to run on a raspberry pi zero.

Makefile will generate two files, one is **primalos.elf**, which is needed to run on the emulator, other is **kernel.img** which is needed to run on the hardware.

# RUNNING
You can run the build either on the raspberry pi zero hardware, or on an emulator, makefile is specifically set to work with qemu-system-arm. 

**1. To run on the hardware**, make sure you've built with **ON_EMU** macro set to one. Then you have to have an existing operating system for raspberry pi, written to an sd-card. There are many ways to do this, easiest way is to use raspberry pi's own tool, [Raspberry Pi Imager](https://www.raspberrypi.com/news/raspberry-pi-imager-imaging-utility/). It doesn't matter, which operating system it is, since we only need it to boot our system.

After you've written an existing operating system to the sd-card, you have to replace the **kernel.img** file of that operating system, inside the sd-card, with our **kernel.img** file.

**2. To run on the emulator**, you need to download [Qemu](https://www.qemu.org/download/). After installing, add the path to qemu-system-arm executable in qemu directory, to environment variables, since that's what the Makefile needs. To run it, simply run the command: **make run**. On the emulator build, outputting is done through uart, so you'll see any output on the terminal and not the emulator window.

This is what you need if you're on windows, I'm not sure if there are additional configurations to do on linux or macos. Since I've only run the emulator on windows.