GCC=../../contrib/bwilks/gcc-arm/bin/arm-none-linux-gnueabihf-gcc
HXC=../../vs2022/src/hxc/Release/hxc
AR=../../contrib/bwilks/gcc-arm/arm-none-linux-gnueabihf/bin/ar

libc.a: FORCE
	mkdir -p bin
	$(HXC) -c -o bin/libc.o libc.c --nostdlib
	$(GCC) -c -o bin/arch.o start.s
	$(AR) rcs bin/libc.a bin/libc.o bin/arch.o

clean:
	rm -f bin/libc.o
	rm -f bin/libc.a
	rm -f bin/arch.o

.PHONY: FORCE
