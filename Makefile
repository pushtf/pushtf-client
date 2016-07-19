# Binaries
BIN = pushtf
CC = gcc
RM = rm -f

# Compiler options
CFLAGS = -D_FILE_OFFSET_BITS=64 -Wall -pedantic -fstack-protector -O2
LDFLAGS = -lcurl

# Rules name definition
.PHONY: all clean

# Rule: all
all:
	$(CC) *.c $(CFLAGS) $(LDFLAGS) -o $(BIN)

# Rule: clean
clean:
	$(RM) $(BIN)
