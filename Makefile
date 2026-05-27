CC      = gcc
CFLAGS  = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -g
TARGET  = fs_explorer

all: $(TARGET)

$(TARGET): fs_explorer.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
	rm -rf /tmp/fs_lab

.PHONY: all clean
