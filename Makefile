# Makefile for compiling client.c and server.c

CC=gcc                # Compiler to use
CFLAGS=-Wall -g       # Compilation flags: Enable all warnings and debugging information
TARGETS=client server # Target executable names

all: $(TARGETS)

client: client.o
	$(CC) $(CFLAGS) client.o -o client -lpthread

server: server.o
	$(CC) $(CFLAGS) server.o -o server

client.o: client.c
	$(CC) $(CFLAGS) -c client.c

server.o: server.c chatroom.h
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
