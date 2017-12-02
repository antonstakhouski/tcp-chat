CC=gcc
CFLAGS=-c -Wall -g -O0 -pedantic
LDFLAGS=
LIBRARY=unix_lib
SOURCES=client.c ../$(LIBRARY).c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLES=client

all: $(SOURCES) $(EXECUTABLES)
	# bash win_rules.mk

client: client.o $(LIBRARY).o
	$(CC) $(LDFLAGS) client.o $(LIBRARY).o -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

$(LIBRARY).o: ../$(LIBRARY).c ../cross_header.h
	$(CC) $(CFLAGS) ../$(LIBRARY).c -o $(LIBRARY).o

clean:
	rm *.o client client.exe
