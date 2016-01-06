# conninfo-parse

`conninfo-parse` is a small and rather niche utility which accepts a PostgreSQL
[connection string](http://www.postgresql.org/docs/9.4/static/libpq-connect.html#LIBPQ-CONNSTRING),
parses it, and prints the result in one of several formats.

![conninfo-parse](screenshot.png)

## Usage

`conninfo-parse` supports three output formats:

* **Delimited** —
  Key/value pairs are printed one pair per line, with each key and value
  separated by an arbitrary delimiter string. This is the default behaviour; the
  default delimiter is a tab character.

* **JSON** —
  Key/value pairs are printed as members of a JSON object.

* **Shell** —
  Similar to the delimited format, except that key/value pairs are valid shell
  variable assignments. This is designed to be used safely with the shell's
  `eval` built-in.

## Building

To build `conninfo-parse`, you'll need:

* `make`, a C compiler, and the other usual stuff
* `pkg-config`
* `libpq` headers
* `json-c` headers

On Debian/Ubuntu you can install the headers via
`sudo apt-get install libpq-dev libjson0-dev`.

On OS X, `brew install postgresql json-c` will install the correct headers for
you.

Once you have the necessary pre-requisites, simply run the following to build:

```
./configure
make
make install # optionally install to /usr/local/bin
```

## License

MIT.

