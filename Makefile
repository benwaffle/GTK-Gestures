CC=clang
CFLAGS=$(shell pkg-config --cflags gtk+-3.0) -std=c11 -include header.h -ggdb -Wall
LDFLAGS=$(shell pkg-config --libs gtk+-3.0) -lm
all: gtk_gestures
debug: CFLAGS += -fsanitize=address
debug: all
.PHONY: clean
clean:
	rm gtk_gestures
