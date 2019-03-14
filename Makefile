CFLAGS := -O2 \
          -Werror -Wall -Wextra \
          -Wcast-align \
          -Wcast-qual \
          -Wconversion \
          -Wformat-nonliteral \
          -Wformat-security \
          -Winit-self \
          -Wmissing-declarations \
          -Wmissing-prototypes \
          -Wnested-externs \
          -Wpointer-arith \
          -Wredundant-decls \
          -Wshadow \
          -Wstack-protector \
          -Wstrict-overflow=5 \
          -Wstrict-prototypes \
          -Wwrite-strings \
          -I.
LDFLAGS :=
PREFIX  := /usr/local/bin

-include config.mk

all: conninfo-parse

config.h:
	@echo >&2 'config.h not found; try running configure'
	@false

conninfo-parse: conninfo-parse.c config.h
	$(CC) -Wall $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

install: conninfo-parse
	cp conninfo-parse "$(PREFIX)/"

# @todo This sucks
test: conninfo-parse
	./conninfo-parse   -h | grep -qiE 'usage.*--shell'
	./conninfo-parse   -q host=foo
	! ./conninfo-parse -q fake=foo
	./conninfo-parse      host=foo | grep -qixE 'host[[:space:]]+foo'
	./conninfo-parse   -s host=foo | grep -qixE "host='foo'"

clean:
	rm -f  -- *.[ao] *.out *.so conninfo-parse
	rm -rf -- *.dSYM

distclean: clean
	rm -f config.h config.mk

.PHONY: all install test clean distclean
