#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <err.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <cairo.h>

__attribute__((unused)) static void _auto_free(void **p) {
    free(*p);
}

#define autofree __attribute__(( cleanup(_auto_free) ))
