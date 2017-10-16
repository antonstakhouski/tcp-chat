CC=gcc
CFLAGS=-c -Wall -g -O0 -pedantic
LDFLAGS=
SOURCES=server.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLES=server

all: $(SOURCES) $(EXECUTABLES)

server: server.o
	$(CC) $(LDFLAGS) server.o -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLES)
