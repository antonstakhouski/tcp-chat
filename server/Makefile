CC=gcc
CFLAGS=-c -Wall -g -O0 -pedantic
LDFLAGS=
LIBRARY=unix_lib
SOURCES=server.c ../$(LIBRARY).c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLES=server

all: $(SOURCES) $(EXECUTABLES)
	# bash Makefile.win
	# cp server.exe ~/"PlayOnLinux's virtual drives"/cygwin/drive_c/

server: server.o $(LIBRARY).o
	$(CC) $(LDFLAGS) server.o $(LIBRARY).o -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

$(LIBRARY).o: ../$(LIBRARY).c ../cross_header.h
	$(CC) $(CFLAGS) ../$(LIBRARY).c -o $(LIBRARY).o

clean:
	rm *.o server server.exe
