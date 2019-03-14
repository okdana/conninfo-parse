/**
 * Command-line utility to parse PostgreSQL a conninfo (DSN) string and print it
 * back in one of several formats.
 *
 * @todo It should be possible to specify both column and row delimiters for
 *       `-d`
 *
 * @copyright © dana <https://github.com/okdana/conninfo-parse>
 * @license   MIT
 */

#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#ifdef CP_HAVE_JSON
  #include "json.h"
#endif

#include "libpq-fe.h"

#define CP_NAME    "conninfo-parse"
#define CP_DESC    "Parse a PostgreSQL conninfo string and output the result"
#define CP_VERSION "0.2.0"

#define CP_OUT_DELIMITED 0
#define CP_OUT_SHELL     1
#define CP_OUT_JSON      2

// Adapted from sysexits.h
#define CP_EX_OK          0
#define CP_EX_ERR         1
#define CP_EX_USAGE       64
#define CP_EX_UNAVAILABLE 69

static const char *short_opts = "d:hjqsV";
static const struct option long_opts[] = {
  { "help",      no_argument,       0, 'h' },
  { "version",   no_argument,       0, 'V' },
  { "quiet",     no_argument,       0, 'q' },
  { "delimited", required_argument, 0, 'd' },
  { "delimiter", required_argument, 0, 'd' }, // Legacy name
  { "json",      no_argument,       0, 'j' },
  { "shell",     no_argument,       0, 's' },
  { 0, 0, 0, 0 }
};

static const char *short_usage_args =
  "[-h|-V] [-q] [-d <dc>|-j|-s] <conninfo>"
;
static const char *long_usage_args =
  "  -h, --help            Display this help information and exit\n"
  "  -V, --version         Display version information and exit\n"
  "  -q, --quiet           Suppress normal output (validate only)\n"
  "  -d, --delimited <dc>  Output in delimited format, where <dc> delimits columns\n"
  "                        and \\n delimits rows\n"
  "  -j, --json            Output in JSON format\n"
  "  -s, --shell           Output in shell variable format\n"
  "  <conninfo>            conninfo string to parse\n"
;

/**
 * Print formatted error message.
 *
 * @see fprintf(3)
 *
 * @param stream Output stream.
 * @param name   Program name (argv[0]).
 * @param format Format string.
 * @param ...    (optional) Format arguments.
 *
 * @return According to *printf().
 */
static int
cp_efprintf(
  FILE *restrict stream,
  const char *restrict name,
  const char *restrict format,
  ...
)
__attribute__((__nonnull__(1, 3)))
__attribute__((__format__(printf, 3, 4)));

/**
 * Print usage help.
 *
 * @see fprintf(3)
 *
 * @param stream The output stream to print to.
 * @param name   Program name (argv[0]).
 * @param brief  Whether to print the brief (as opposed to full) usage help.
 *
 * @return int According to *printf().
 */
static int
cp_fprintu(FILE *restrict stream, const char *restrict name, bool brief)
__attribute__((__nonnull__(1)));

/**
 * Escape a string for use as a shell argument.
 *
 * @param arg The argument to escape.
 *
 * @return The escaped argument. Caller must free.
 */
static char *
cp_escape_shell_arg(const char *restrict arg)
__attribute__((__nonnull__))
__attribute__((__unused__));

static int
cp_efprintf(
  FILE *restrict stream,
  const char *restrict name,
  const char *restrict format,
  ...
) {
  int     ret = 0;
  va_list ap;

  va_start(ap, format);
  ret += fprintf(stream, "%s: ", name ?: CP_NAME);
  ret += vfprintf(stream, format, ap);
  va_end(ap);

  return ret;
}

static int
cp_fprintu(FILE *restrict stream, const char *restrict name, bool brief) {
  if ( brief ) {
    return fprintf(stream, "usage: %s %s\n", name ?: CP_NAME, short_usage_args);
  }
  return fprintf(
    stream,
    "%s\n\n"
    "usage:\n"
    "  %s %s\n\n"
    "options:\n"
    "%s",
    CP_DESC,
    name ?: CP_NAME,
    short_usage_args,
    long_usage_args
  );
}

static char *
cp_escape_shell_arg(const char *arg) {
  char *escaped, *eptr;

  eptr = escaped = malloc(strlen(arg) * strlen("'\\''") + 3);
  *eptr++ = '\'';

  while ( *arg ) {
    if ( *arg == '\'' ) {
      *eptr++ = '\'';
      *eptr++ = '\\';
      *eptr++ = '\'';
      *eptr++ = '\'';
    } else {
      *eptr++ = *arg;
    }
    arg++;
  }

  *eptr++ = '\'';
  *eptr++ = '\0';

  return escaped;
}

int main(int argc, char *argv[]) {
  bool quiet  = false;
  int  idx    = 0;
  int  opt    = 0;
  int  output = CP_OUT_DELIMITED;
  const char *delimiter = "\t";

  while ( (opt = getopt_long(argc, argv, short_opts, long_opts, &idx)) >= 0 ) {
    switch ( opt ) {
      case 'h':
        cp_fprintu(stdout, argv[0], false);
        return CP_EX_OK;
      case 'V':
        fprintf(stdout, "%s version %s\n", CP_NAME, CP_VERSION);
        return CP_EX_OK;
      case 'q':
        quiet = true;
        break;
      case 'd':
        if ( ! *optarg ) {
            cp_efprintf(stderr, argv[0], "invalid delimiter spec\n");
            cp_fprintu(stderr, argv[0], true);
            return CP_EX_USAGE;
        }
        output    = CP_OUT_DELIMITED;
        delimiter = optarg;
        break;
      case 'j':
        #ifndef CP_HAVE_JSON
          cp_efprintf(stderr, argv[0], "JSON support not available\n");
          return CP_EX_UNAVAILABLE;
        #endif
        output = CP_OUT_JSON;
        break;
      case 's':
        output = CP_OUT_SHELL;
        break;
      case ':':
      case '?':
        cp_fprintu(stderr, argv[0], true);
        return CP_EX_USAGE;
      // ???
      default: abort();
    }
  }

  if ( optind >= argc ) {
    cp_efprintf(stderr, argv[0], "expected conninfo string\n");
    cp_fprintu(stderr, argv[0], true);
    return CP_EX_USAGE;
  } else if ( argc > optind + 1 ) {
    cp_efprintf(stderr, argv[0], "unexpected argument: %s\n", argv[optind + 1]);
    cp_fprintu(stderr, argv[0], true);
    return CP_EX_USAGE;
  }

  char *errmsg = NULL;
  PQconninfoOption *conninfo, *param;
  conninfo = PQconninfoParse(argv[optind], &errmsg);

  if ( ! conninfo ) {
    if ( ! quiet ) {
      // libpq new-line-terminates its own error messages i guess
      cp_efprintf(stderr, argv[0], "parse error: %s", errmsg ?: "unknown\n");
    }
    if ( errmsg ) {
      PQfreemem(errmsg);
    }
    return CP_EX_ERR;
  }

  if ( quiet ) {
    goto end;
  }

  #ifdef CP_HAVE_JSON
    json_object *json_obj = json_object_new_object();
  #endif

  for ( param = conninfo; param && param->keyword; param++ ) {
    if ( ! param->val ) {
      continue;
    }
    switch ( output ) {
      case CP_OUT_DELIMITED:
        fprintf(stdout, "%s%s%s\n", param->keyword, delimiter, param->val);
        break;

      case CP_OUT_SHELL:
        {
          char *arg = cp_escape_shell_arg(param->val);
          fprintf(stdout, "%s=%s\n", param->keyword, arg);
          free(arg);
        }
        break;

      case CP_OUT_JSON:
        #ifdef CP_HAVE_JSON
          json_object_object_add(
            json_obj,
            param->keyword,
            json_object_new_string(param->val)
          );
        #else
          abort();
        #endif
        break;

      default: abort();
    }
  }

  if ( output == CP_OUT_JSON ) {
    #ifdef CP_HAVE_JSON
      fprintf(stdout, "%s\n", json_object_to_json_string(json_obj));
      json_object_put(json_obj);
    #else
      abort();
    #endif
  }

  end:
    PQconninfoFree(conninfo);
    return CP_EX_OK;
}
