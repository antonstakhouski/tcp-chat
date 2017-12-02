CC=i686-w64-mingw32-gcc
CFLAGS=-c -Wall -g -O0 -pedantic
LDFLAGS=

SOURCEDIR = src
BUILDDIR = build

SOURCES = $(wildcard $(SOURCEDIR)/*.c)
SOURCES += ../win_lib.c
OBJECTS = $(patsubst $(SOURCEDIR)/*.c,build/%.o,$(SOURCES))

EXECUTABLE=client-win.exe

all: clean dir $(BUILDDIR)/$(EXECUTABLE)

dir:
	mkdir -p $(BUILDDIR)
arm:
	make -f beagle_rules.mk
win:
	bash win_rules.mk
$(BUILDDIR)/$(EXECUTABLE): $(OBJECTS)
	$(CC) --static $^ -o $@ -lws2_32
$(OBJECTS):
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm -f build/*.o $(EXECUTABLE)
