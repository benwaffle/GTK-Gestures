CFLAGS=$(shell pkg-config --cflags gtk+-3.0) -std=c11 -include header.h -ggdb -Wall
LDFLAGS=$(shell pkg-config --libs gtk+-3.0) -lm

gtk_gestures: gtk_gestures.o ui.o
ui.o: ui.c
ui.c:
	glib-compile-resources --generate-source ui.gresource.xml

.PHONY: clean
clean:
	rm gtk_gestures *.o
