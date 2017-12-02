CC ?= gcc
CFLAGS=-c -Wall -g -O0 -pedantic
LDFLAGS=

SOURCEDIR = src
BUILDDIR = build

SOURCES ?= client.c unix_lib.c
OBJECTS = $(patsubst %.c,$(BUILDDIR)/%.o,$(SOURCES))

EXECUTABLE ?= client

.PHONY: clean lin-client arm-client

all: dir $(BUILDDIR)/$(EXECUTABLE)

dir:
	mkdir -p $(BUILDDIR)

lin-client:
	$(eval EXECUTABLE := client)
	$(eval CC := gcc)
	$(eval SOURCES := client.c unix_lib.c)
	@$(MAKE) -f Makefile all EXECUTABLE=$(EXECUTABLE) CC=$(CC) SOURCES="$(SOURCES)"

lin-server:
	$(eval EXECUTABLE := server)
	$(eval CC := gcc)
	$(eval SOURCES := server.c unix_lib.c)
	@$(MAKE) -f Makefile all EXECUTABLE=$(EXECUTABLE) CC=$(CC) SOURCES="$(SOURCES)"

win-client:
	$(eval EXECUTABLE := client.exe)
	$(eval CC := i686-w64-mingw32-gcc)
	$(eval SOURCES := client.c win_lib.c)
	$(eval LDFLAGS := --static -lws2_32)
	@$(MAKE) -f Makefile all EXECUTABLE=$(EXECUTABLE) CC=$(CC) SOURCES="$(SOURCES)" LDFLAGS="$(LDFLAGS)"

win-server:
	$(eval EXECUTABLE := server.exe)
	$(eval CC := i686-w64-mingw32-gcc)
	$(eval SOURCES := server.c win_lib.c)
	$(eval LDFLAGS := --static -lws2_32)
	@$(MAKE) -f Makefile all EXECUTABLE=$(EXECUTABLE) CC=$(CC) SOURCES="$(SOURCES)" LDFLAGS="$(LDFLAGS)"

arm-client:
	$(eval EXECUTABLE := client-arm)
	$(eval CC := arm-linux-gnueabihf-gcc)
	$(eval SOURCES := client.c unix_lib.c)
	@$(MAKE) -f Makefile EXECUTABLE=$(EXECUTABLE) CC=$(CC) SOURCES="$(SOURCES)"

arm-server:
	$(eval EXECUTABLE := server-arm)
	$(eval CC := arm-linux-gnueabihf-gcc)
	$(eval SOURCES := server.c unix_lib.c)
	@$(MAKE) -f Makefile EXECUTABLE=$(EXECUTABLE) CC=$(CC) SOURCES="$(SOURCES)"
win:
	make -f win_rules.mk

$(BUILDDIR)/$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJECTS): $(BUILDDIR)/%.o : $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm -f $(BUILDDIR)/*.o
