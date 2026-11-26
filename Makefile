VERSION = 1.0.0

CC ?= cc
PKG_CONFIG ?= pkg-config
INSTALL ?= install

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man
DATADIR ?= $(PREFIX)/share

DESTDIR ?=

CFLAGS ?= -O2

WARNINGS = -Wall -Wextra -Wpedantic -Werror
STANDARD = -std=c11

GRAPHICS_CFLAGS = $(shell $(PKG_CONFIG) --cflags x11 gl glu libjpeg)
GRAPHICS_LIBS = $(shell $(PKG_CONFIG) --libs x11 gl glu libjpeg)

PROGRAM = qso-globe

SOURCES = src/qso-globe.c \
          src/config.c \
          src/state.c \
          src/filesystem.c \
          src/markers.c \
          src/geodesy.c \
          src/locator.c \
          src/qso-input.c \
          src/solar.c

HEADERS = $(wildcard src/*.h)

TESTS = tests/test-config \
        tests/test-state \
        tests/test-input \
        tests/test-solar \
        tests/test-network \
        tests/test-filesystem

.PHONY: all test clean install uninstall

all: $(PROGRAM)

$(PROGRAM): $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STANDARD) $(WARNINGS) \
	    -Wno-unused-function $(GRAPHICS_CFLAGS) \
	    $(SOURCES) $(LDFLAGS) $(GRAPHICS_LIBS) -lm \
	    -o $@

tests/test-config: tests/test-config.c src/config.c src/config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STANDARD) $(WARNINGS) \
	    -Isrc tests/test-config.c src/config.c \
	    $(LDFLAGS) -lm -o $@

tests/test-state: tests/test-state.c src/config.c src/state.c src/config.h src/state.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STANDARD) $(WARNINGS) \
	    -Isrc tests/test-state.c src/config.c src/state.c \
	    $(LDFLAGS) -lm -o $@

tests/test-input: tests/test-input.c src/qso-input.c src/locator.c src/geodesy.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STANDARD) $(WARNINGS) \
	    -Isrc tests/test-input.c src/qso-input.c \
	    src/locator.c src/geodesy.c \
	    $(LDFLAGS) -lm -o $@

tests/test-solar: tests/test-solar.c src/solar.c src/solar.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STANDARD) $(WARNINGS) \
	    -Isrc tests/test-solar.c src/solar.c \
	    $(LDFLAGS) -lm -o $@

tests/test-network: tests/test-network.c src/qso-input.c src/locator.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STANDARD) $(WARNINGS) \
	    -Isrc tests/test-network.c src/qso-input.c \
	    src/locator.c $(LDFLAGS) -lm -o $@


tests/test-filesystem: tests/test-filesystem.c src/filesystem.c src/filesystem.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STANDARD) $(WARNINGS) \
	    -Isrc tests/test-filesystem.c src/filesystem.c \
	    $(LDFLAGS) -o $@


test: $(TESTS)
	./tests/test-config
	./tests/test-state
	./tests/test-solar
	./tests/test-network
	./tests/test-filesystem

install: all
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -d "$(DESTDIR)$(MANDIR)/man1"
	$(INSTALL) -d "$(DESTDIR)$(DATADIR)/qso-globe"
	$(INSTALL) -d "$(DESTDIR)$(DATADIR)/applications"
	$(INSTALL) -d "$(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps"
	$(INSTALL) -m 755 $(PROGRAM) "$(DESTDIR)$(BINDIR)/qso-globe"
	$(INSTALL) -m 755 scripts/prepare-textures.sh "$(DESTDIR)$(BINDIR)/qso-globe-prepare-textures"
	$(INSTALL) -m 644 man/qso-globe.1 "$(DESTDIR)$(MANDIR)/man1/qso-globe.1"
	$(INSTALL) -m 644 examples/qso-globe.conf.example "$(DESTDIR)$(DATADIR)/qso-globe/"
	$(INSTALL) -m 644 desktop/qso-globe.desktop "$(DESTDIR)$(DATADIR)/applications/"
	$(INSTALL) -m 644 desktop/qso-globe.svg "$(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps/"

	$(INSTALL) -m 644 LICENSE "$(DESTDIR)$(DATADIR)/qso-globe/LICENSE"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/qso-globe"
	rm -f "$(DESTDIR)$(BINDIR)/qso-globe-prepare-textures"
	rm -f "$(DESTDIR)$(MANDIR)/man1/qso-globe.1"
	rm -f "$(DESTDIR)$(DATADIR)/qso-globe/qso-globe.conf.example"
	rm -f "$(DESTDIR)$(DATADIR)/applications/qso-globe.desktop"
	rm -f "$(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps/qso-globe.svg"

	rm -f "$(DESTDIR)$(DATADIR)/qso-globe/LICENSE"

clean:
	rm -f $(PROGRAM) $(TESTS)
