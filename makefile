PREFIX=~/.local

TARGET=./aquarium

CC=gcc
CFLAGS=-std=c99 -lm -O0 -g -DPREFIX=\"$(shell realpath $(PREFIX))\" -Wall -Werror

SRCS=aquarium.c term.c

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) 

install:
	mkdir -p $(PREFIX)/share/aquarium/
	cp -rf ./fish/ $(PREFIX)/share/aquarium/
	cp $(TARGET) $(PREFIX)/bin/

tc: all
	$(TARGET)
