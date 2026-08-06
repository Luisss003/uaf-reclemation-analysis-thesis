CC = gcc
CFLAGS = -Wall -g -O0

all: wrapper_syscalls.so main

wrapper_syscalls.so: wrapper_syscalls.c
	$(CC) $(CFLAGS) -DRUNTIME -shared -fpic -o $@ $< -ldl

main: main.c
	$(CC) $(CFLAGS) -o $@ $<

run: wrapper_syscalls.so main
	LD_PRELOAD=$(CURDIR)/wrapper_syscalls.so ./main

clean:
	rm -f wrapper_syscalls.so main
