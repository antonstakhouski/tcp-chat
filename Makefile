CC ?= gcc
CFLAGS=-c -Wall -g -O0 -pedantic -std=c11
LDFLAGS=

SOURCEDIR = src
BUILDDIR = build

SOURCES = lib.c
CLIENT_SOURCES= client.c client_lib.c
SERVER_SOURCES= server.c server_lib.c

OBJECTS = $(patsubst %.c,$(BUILDDIR)/%.o,$(SOURCES))

EXECUTABLE ?= client

BBB_ADDR="debian@192.168.7.2:~/"
RPI_ADDR="pi@192.168.12.213:~/"

BBB_PASS="temppwd"
RPI_PASS="raspberry"

BBB_CC = arm-linux-gnueabihf-gcc
RPI_CC = /bin/arm-bcm2708-linux-gnueabi/gcc

ARM_CC = $(BBB_CC)
R_ADDR=$(RPI_ADDR)
R_PASS=$(RPI_PASS)

# ARM_CC = $(BBB_CC)
# R_ADDR=$(BBB_ADDR)
# R_PASS=$(BBB_PASS)

.PHONY: clean lin-client arm-client

all: dir $(BUILDDIR)/$(EXECUTABLE)

dir:
	mkdir -p $(BUILDDIR)

lin-client:
	$(eval EXECUTABLE := client)
	$(eval CC := gcc)
	$(eval SOURCES += $(CLIENT_SOURCES))
	$(eval SOURCES += unix_lib.c)
	@$(MAKE) -f Makefile all EXECUTABLE=$(EXECUTABLE) CC=$(CC) SOURCES="$(SOURCES)"

lin-server:
	$(eval EXECUTABLE := server)
	$(eval CC := gcc)
	$(eval SOURCES += $(SERVER_SOURCES))
	$(eval SOURCES += unix_lib.c)
	@$(MAKE) -f Makefile all EXECUTABLE=$(EXECUTABLE) CC=$(CC) SOURCES="$(SOURCES)"

win-client:
	$(eval EXECUTABLE := client.exe)
	$(eval CC := i686-w64-mingw32-gcc)
	$(eval SOURCES += $(CLIENT_SOURCES))
	$(eval SOURCES += win_lib.c)
	$(eval LDFLAGS := --static -lws2_32)
	@$(MAKE) -f Makefile all EXECUTABLE=$(EXECUTABLE) CC=$(CC) SOURCES="$(SOURCES)" LDFLAGS="$(LDFLAGS)"

win-server:
	$(eval EXECUTABLE := server.exe)
	$(eval CC := i686-w64-mingw32-gcc)
	$(eval SOURCES += $(SERVER_SOURCES))
	$(eval SOURCES += win_lib.c)
	$(eval LDFLAGS := --static -lws2_32)
	@$(MAKE) -f Makefile all EXECUTABLE=$(EXECUTABLE) CC=$(CC) SOURCES="$(SOURCES)" LDFLAGS="$(LDFLAGS)"

arm-client:
	$(eval EXECUTABLE := client-arm)
	$(eval SOURCES += $(CLIENT_SOURCES))
	$(eval SOURCES += unix_lib.c)
	@$(MAKE) -f Makefile EXECUTABLE=$(EXECUTABLE) CC=$(ARM_CC) SOURCES="$(SOURCES)"
	sshpass -p $(R_PASS) scp $(BUILDDIR)/$(EXECUTABLE) $(R_ADDR)

arm-server:
	$(eval EXECUTABLE := server-arm)
	$(eval CC := arm-linux-gnueabihf-gcc)
	$(eval SOURCES += $(SERVER_SOURCES))
	$(eval SOURCES += unix_lib.c)
	@$(MAKE) -f Makefile EXECUTABLE=$(EXECUTABLE) CC=$(ARM_CC) SOURCES="$(SOURCES)"
	sshpass -p $(R_PASS) scp $(BUILDDIR)/$(EXECUTABLE) $(R_ADDR)
win:
	make -f win_rules.mk

$(BUILDDIR)/$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJECTS): $(BUILDDIR)/%.o : $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm -f $(BUILDDIR)/*.o
