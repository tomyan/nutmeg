
CC=arm-elf-gcc-4.6
CFLAGS=-Iinclude -ansi -pedantic -Wall -Wextra -march=armv6 -msoft-float -fPIC -mapcs-frame -marm
LD=arm-elf-ld
LDFLAGS=-N -Ttext=0x10000

run: target/kernel
	echo "press ctrl+a followed by x to exit..."
	qemu-system-arm -M versatilepb -cpu arm1176 -nographic -kernel target/kernel

build/kernel.o: src/kernel.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $^ -o $@

build/bootstrap.o: src/bootstrap.s
	mkdir -p build
	$(CC) $(CFLAGS) -c $^ -o $@

build/context_switch.o: src/context_switch.s
	mkdir -p build
	$(CC) $(CFLAGS) -c $^ -o $@

build/syscalls.o: src/syscalls.s
	mkdir -p build
	$(CC) $(CFLAGS) -c $^ -o $@

target/kernel: build/bootstrap.o build/kernel.o build/context_switch.o build/syscalls.o
	mkdir -p target
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -rf build target

