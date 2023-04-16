CC=cc

CFLAGS=-g -Wall -I.. -std=c99 -pedantic -D_DEFAULT_SOURCE

O=bin

default: dirs test

dirs:
	mkdir -p $(O)

clean:
	rm -rf $(O)/*

test: read_line
	$(CC) $(CFLAGS) main.c $(O)/read_line.o -o $(O)/test

read_line:
	$(CC) $(CFLAGS) -c ./../read_line.c \
	-o $(O)/read_line.o
