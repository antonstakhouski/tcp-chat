CC=arm-linux-gnueabihf-gcc
CFLAGS=-c -Wall -g -O0 -pedantic
LDFLAGS=
LIBRARY=unix_lib
SOURCES=client.c ../$(LIBRARY).c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLES=client-beagle

all: clean $(SOURCES) $(EXECUTABLES)
	sshpass -p "temppwd" scp $(EXECUTABLES) debian@192.168.7.2:~/
	# bash win_rules.mk

client-beagle: client.o $(LIBRARY).o
	$(CC) $(LDFLAGS) client.o $(LIBRARY).o -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

$(LIBRARY).o: ../$(LIBRARY).c ../cross_header.h
	$(CC) $(CFLAGS) ../$(LIBRARY).c -o $(LIBRARY).o

clean:
	rm -f *.o client-beagle client.exe
