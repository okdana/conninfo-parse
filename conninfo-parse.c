/**
 * Parses a PostgreSQL conninfo string and prints it back in one of several
 * formats.
 *
 * @copyright © dana <https://github.com/okdana/conninfo-parse>
 * @license   MIT
 */

#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "libpq-fe.h"

#define CP_NAME    "conninfo-parse"
#define CP_DESC    "Parses a PostgreSQL conninfo string and outputs the result."
#define CP_VERSION "0.1.1"

#define CP_OUTPUT_DELIMITED 0
#define CP_OUTPUT_SHELL     1
#define CP_OUTPUT_JSON      2

// Short and long options
static const char *short_opts = ":d:hjsV";
static struct option long_opts[] = {
	{ "help",      no_argument,       0, 'h' },
	{ "version",   no_argument,       0, 'V' },
	{ "delimiter", required_argument, 0, 'd' },
	{ "json",      no_argument,       0, 'j' },
	{ "shell",     no_argument,       0, 's' },
	{ 0, 0, 0, 0 }
};

// Short and long usage text
static const char *short_usage_opts =
	"[-h|-V] [-d <d>|-j|-s] <conninfo>"
;
static const char *long_usage_opts =
	"  -h, --help               Display this help information and exit.\n"
	"  -V, --version            Display version information and exit.\n"
	"  -d <d>, --delimiter <d>  Output in d-delimited format.\n"
	"  -j, --json               Output in JSON format.\n"
	"  -s, --shell              Output in shell variable format.\n"
	"  <conninfo>               Set conninfo string to parse.\n"
;

/**
 * Escapes a string in a manner suitable for use as a shell command-line
 * argument or variable value.
 *
 * @param const char *arg
 *   The argument to escape.
 *
 * @return char *
 *   The escaped string. Caller must free.
 */
char *escape_shell_argument(const char *arg) {
	int   i = 0, j = 0;
	char *escaped;

	escaped      = malloc(strlen(arg) * strlen("'\\''") + 3);
	escaped[j++] = '\'';

	for ( i = 0; arg[i] != '\0'; i++ ) {
		if ( arg[i] == '\'' ) {
			escaped[j++] = '\'';
			escaped[j++] = '\\';
			escaped[j++] = '\'';
			escaped[j++] = '\'';
		} else {
			escaped[j++] = arg[i];
		}
	}

	escaped[j++] = '\'';
	escaped[j++] = '\0';

	return escaped;
}

/**
 * Prints usage help.
 *
 * @param int long_usage
 *   Whether to print the full or brief usage help. Prints full usage if > 0.
 *
 * @param int fd
 *   The file descriptor to print to.
 *
 * @return void
 */
void print_usage(int long_usage, FILE *fd) {
	// Short usage
	if ( long_usage <= 0 ) {
		fprintf(fd, "usage: %s %s\n", CP_NAME, short_usage_opts);
		return;
	}
	// Long usage
	fprintf(fd, "%s\n\n",              CP_DESC);
	fprintf(fd, "usage:\n  %s %s\n\n", CP_NAME, short_usage_opts);
	fprintf(fd, "options:\n%s",        long_usage_opts);
}

/**
 * Prints an standard-format error to stderr.
 *
 * @param const char *format
 *   A format string to pass to vfprintf().
 *
 * @param ...
 *   Zero or more additional arguments to pass to vfprintf().
 *
 * @return int
 *   Returns according to vfprintf().
 */
int print_error(const char *format, ...) {
	int     ret;
	va_list args;

	fprintf(stderr, "%s: ", CP_NAME);

	va_start(args, format);
	ret = vfprintf(stderr, format, args);
	va_end(args);

	return ret;
}

/**
 * Main entry-point.
 *
 * @param int   argc
 * @param char *argv
 *
 * @return int
 */
int main(int argc, char *argv[]) {
	int   opt       = 0;
	int   opt_index = 0;
	int   output    = CP_OUTPUT_DELIMITED;
	char *delimiter = "\t";
	char *escaped   = NULL;
	char *errmsg    = NULL;

	PQconninfoOption *conninfo, *param;
	json_object *json_obj = json_object_new_object();

	while ( (opt = getopt_long(argc, argv, short_opts, long_opts, &opt_index)) >= 0 ) {
		switch ( opt ) {
			// Usage help
			case 'h':
				print_usage(1, stdout);
				return 0;
			// Print version
			case 'V':
				fprintf(stdout, "%s version %s\n", CP_NAME, CP_VERSION);
				return 0;
			// Output in delimited format, set delimiter
			case 'd':
				output    = CP_OUTPUT_DELIMITED;
				delimiter = optarg;
				break;
			// Output in JSON format
			case 'j':
				output = CP_OUTPUT_JSON;
				break;
			// Output in shell variable format
			case 's':
				output = CP_OUTPUT_SHELL;
				break;
			// Unrecognised option
			case ':':
			case '?':
				print_error("Invalid option '%c'\n", optopt);
				print_usage(0, stderr);
				return 1;
		}
	}

	// Missing string
	if ( optind >= argc ) {
		print_error("No conninfo string provided.\n");
		print_usage(0, stderr);
		return 1;
	}

	conninfo = PQconninfoParse(argv[optind], &errmsg);

	// Problem with string
	if ( conninfo == NULL ) {
		print_error("Parse error: %s", errmsg);
		PQfreemem(errmsg);
		return 1;
	}

	for ( param = conninfo; param->keyword; ++param ) {
		if ( param->val == NULL ) {
			continue;
		}

		switch ( output ) {
			case CP_OUTPUT_DELIMITED:
				fprintf(stdout, "%s%s%s\n", param->keyword, delimiter, param->val);
				break;

			case CP_OUTPUT_SHELL:
				escaped = escape_shell_argument(param->val);
				fprintf(stdout, "%s=%s\n", param->keyword, escaped);
				free(escaped);
				break;

			case CP_OUTPUT_JSON:
				json_object_object_add(
					json_obj,
					param->keyword,
					json_object_new_string(param->val)
				);
				break;
		}
	}

	if ( output == CP_OUTPUT_JSON ) {
		fprintf(stdout, "%s\n", json_object_to_json_string(json_obj));
		json_object_put(json_obj);
	}

	PQconninfoFree(conninfo);

	return 0;
}

