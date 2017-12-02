CC=arm-linux-gnueabihf-gcc
CFLAGS=-c -Wall -g -O0 -pedantic
LDFLAGS=

SOURCEDIR = src
BUILDDIR = build

SOURCES = $(wildcard $(SOURCEDIR)/*.c)
SOURCES += ../unix_lib.c
OBJECTS = $(patsubst $(SOURCEDIR)/*.c,build/%.o,$(SOURCES))

EXECUTABLE=client-arm

all: clean dir $(BUILDDIR)/$(EXECUTABLE)
	sshpass -p "temppwd" scp $(BUILDDIR)/$(EXECUTABLE) debian@192.168.7.2:~/

dir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ -o $@

$(OBJECTS):
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/$(EXECUTABLE)
