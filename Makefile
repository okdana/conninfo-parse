include ./Makefile.cf

build: conninfo-parse

conninfo-parse: force
	$(CC) -Wall $(CFLAGS) $(LDFLAGS) -o ./conninfo-parse ./conninfo-parse.c $(LIBS)

install: build
	cp ./conninfo-parse /usr/local/bin/

clean:
	rm -f  ./*.o ./*.out ./*.so ./conninfo-parse
	rm -rf ./*.dSYM

distclean: clean
	rm -f ./Makefile.cf

force:;

.PHONY: build install clean distclean force
.SILENT:

