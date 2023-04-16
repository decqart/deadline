CC=cc

CFLAGS=-g -Wall -I.. -std=c99 -pedantic -D_DEFAULT_SOURCE

O=bin

default: dirs test

dirs:
	mkdir -p $(O)

clean:
	rm -rf $(O)/*

test: deadline
	$(CC) $(CFLAGS) main.c $(O)/deadline.o -o $(O)/test

deadline:
	$(CC) $(CFLAGS) -c ./../deadline.c \
	-o $(O)/deadline.o
