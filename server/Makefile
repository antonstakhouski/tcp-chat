CC=gcc
CFLAGS=-c -Wall -g -O0 -pedantic
LDFLAGS=

SOURCEDIR = src
BUILDDIR = build

SOURCES = $(wildcard $(SOURCEDIR)/*.c)
OBJECTS = $(patsubst $(SOURCEDIR)/*.c,build/%.o,$(SOURCES))

EXECUTABLE=server

all: dir $(BUILDDIR)/$(EXECUTABLE)
	# make -f beagle_rules.mk
	# bash win_rules.mk

dir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ -o $@

$(OBJECTS):
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf build
