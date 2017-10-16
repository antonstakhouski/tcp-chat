CC=gcc
CFLAGS=-c -Wall -g -O0 -pedantic
LDFLAGS=
SOURCES=client.c server.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLES=client server

all: $(SOURCES) $(EXECUTABLES)

client: client.o
	$(CC) $(LDFLAGS) client.o -o $@

server: server.o
	$(CC) $(LDFLAGS) server.o -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLES)
