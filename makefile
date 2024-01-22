PREFIX := ~/.local

TARGET := ./aquarium

CC := gcc
CFLAGS := -std=c99 -lm -Oz -DPREFIX=\"$(shell realpath $(PREFIX))\" -Wall -Werror -Wno-array-bounds

SRCS := aquarium.c term.c

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) 

install:
	mkdir -p $(PREFIX)/share/aquarium/
	cp -rf ./fish/ $(PREFIX)/share/aquarium/
	cp $(TARGET) $(PREFIX)/bin/

tc: all
	$(TARGET) -e
