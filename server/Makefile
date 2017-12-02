CC=gcc
CFLAGS=-c -Wall -g -O0 -pedantic
LDFLAGS=

SOURCEDIR = src
BUILDDIR = build

SOURCES = $(wildcard $(SOURCEDIR)/*.c)
SOURCES += ../unix_lib.c
OBJECTS = $(patsubst $(SOURCEDIR)/*.c,build/%.o,$(SOURCES))

EXECUTABLE=server

all: dir $(BUILDDIR)/$(EXECUTABLE)

dir:
	mkdir -p $(BUILDDIR)

win:
	make -f win_rules.mk

$(BUILDDIR)/$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ -o $@

$(OBJECTS):
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/$(EXECUTABLE)
