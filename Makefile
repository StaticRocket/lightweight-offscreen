CC ?= gcc
CFLAGS ?= -Wall -std=c99
CFLAGS += $(shell pkg-config --libs --cflags glesv2 egl gbm)
DESTDIR ?= /

bindir ?= $(DESTDIR)usr/bin

lightweight-offscreen: lightweight-offscreen.c

install: lightweight-offscreen
	install -Dm755 lightweight-offscreen $(bindir)/lightweight-offscreen

clean:
	rm -f lightweight-offscreen
