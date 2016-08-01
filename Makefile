# Binaries
BIN = pushtf
CC = gcc
RM = rm -f
CHECKSUM = md5sum
INSTALL = install -m 0755
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

# Paths
CHECKSUMFILE = $(BIN).md5
INSTALLPATH = /usr/local/bin

# Compiler options
CFLAGS = -D_FILE_OFFSET_BITS=64 -Wall -pedantic -fstack-protector -O2
LDFLAGS = -lcurl

# Rules name definition
.PHONY: all clean install uninstall reset

# Rule: all
all: $(BIN)

# Rule: pushtf
$(BIN): $(OBJ)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $(BIN)

# Build objects
%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

# Rule: checksum
checksum:
	$(CHECKSUM) $(BIN) | cut -d' ' -f1 > $(CHECKSUMFILE)

# Rule: clean
clean:
	$(RM) *.o
	$(RM) $(CHECKSUMFILE)

# Rule: reset
reset: clean
	$(RM) $(BIN)

# Rule: install
install:
	$(INSTALL) $(BIN) $(INSTALLPATH)

# Rule: uninstall
uninstall:
	$(RM) $(INSTALLPATH)/$(BIN)
