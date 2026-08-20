CC = gcc
CFLAGS = -Wall -g -O0

all: generations.so main

generations.so: generations.c 
	$(CC) $(CFLAGS) -DRUNTIME -shared -fpic -o $@ $< -ldl

main: main.c 
	$(CC) $(CFLAGS) -o $@ $<

run: generations.so main
	LD_PRELOAD=$(CURDIR)/generations.so ./main

clean:
	rm -f generations.so main
