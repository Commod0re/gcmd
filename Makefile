#!/usr/bin/make -f

.SUFFIXES =
SUFFIXES =

CC := cc

CFLAGS := $(CFLAGS) -Wall -Wextra -Wpedantic -std=gnu99 -g -fPIC
LDFLAGS := -shared

prefix ?= /usr/local
gcmd_objects := gcmd.o

.PHONY: all install clean test

all: gcmd.so

clean:
	$(RM) $(gcmd_objects) gcmd.so

gcmd.so: $(gcmd_objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-soname,$@ -o $@ $^

$(gcmd_objects): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

install: all
	$(INSTALL) -m 755 gcmd.so $(prefix)/lib/gawk

tests/%.awk: export AWKLIBPATH=$(CURDIR)
tests/%.awk: gcmd.so
	gawk -f $@ $(wildcard tests/file*)


test: $(sort $(wildcard tests/*.awk))
