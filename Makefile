CFLAGS=$(shell pkg-config --cflags gtk+-3.0) -std=gnu11 -g -Wall
LDFLAGS=$(shell pkg-config --libs gtk+-3.0) -lm -rdynamic

gtk_gestures: gtk_gestures.o ui.o
ui.o: ui.c
ui.c: ui.gresource.xml ui.glade
	glib-compile-resources --generate-source ui.gresource.xml

.PHONY: clean
clean:
	rm gtk_gestures *.o ui.c
