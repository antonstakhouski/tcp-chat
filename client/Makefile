CC=gcc
CFLAGS=-c -Wall -g -O0 -pedantic
LDFLAGS=
LIBRARY=unix_lib
SOURCES=client.c ../$(LIBRARY).c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLES=client

all: $(SOURCES) $(EXECUTABLES)
	bash Makefile.win
	cp client.exe ~/"PlayOnLinux's virtual drives"/cygwin/drive_c/

client: client.o $(LIBRARY).o
	$(CC) $(LDFLAGS) client.o $(LIBRARY).o -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

$(LIBRARY).o: ../$(LIBRARY).c
	$(CC) $(CFLAGS) ../$(LIBRARY).c -o $(LIBRARY).o

clean:
	rm *.o client client.exe
