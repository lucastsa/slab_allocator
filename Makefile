CC=gcc
CFLAGS=-c -Wall

all: tests

tests: slab.o test.o
	$(CC) $? -o tests

slab.o: slab.c slab.h
	$(CC) $(CFLAGS) $< -o $@

test.o: test.c slab.h
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm -rf *.o slab tests