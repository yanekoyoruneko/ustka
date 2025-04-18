#if 0
# Hail Plan9
set -xe

SRC="read.c prog.c"
BIN="prog"

CPPFLAGS="-D_DEFAULT_SOURCE"
CFLAGS="-ggdb -std=c11 -pedantic -Wextra -Wall $CPPFLAGS"
LDFLAGS=""
CC="cc"

if [ "$1" = clean ]; then
	for f in $SRC; do
		rm -f "${f%.c}.o"
	done
	rm -f "$BIN"
	[ $# -eq 1 ] && exit 0
fi

for f in $SRC; do
	out=${f%.c}.o
	OBJ="$OBJ $out"
	if [ ! -e "$out" ] || [ "$(find -L "$f" -prune -newer "$out")" ]; then
		$CC -c $CFLAGS $f -o "$out"
	fi
done

[ "$OBJ" ] && $CC $LDFLAGS $OBJ -o "$BIN"

exit 0
#endif
#ifndef AUX_
#define AUX_
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdalign.h>
#include <sysexits.h>

typedef unsigned char      uchar;
typedef signed char        schar;
typedef unsigned short     ushort;
typedef unsigned int       uint;
typedef long long          vlong;
typedef unsigned long      ulong;
typedef unsigned long long uvlong;

#define nil               ((void*)0)
#define nelem(x)          (sizeof(x) / sizeof(*x))
#define entryof(p, t, m)  (t *)((char *)(p) - offsetof(t, m))
#define max(x, y)         ((x) > (y) ? (x) : (y))
#define min(x, y)         ((x) < (y) ? (x) : (y))
#define limit(a, x, b)    max(min(x, b), x, a)
#define align(x)          (((x) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1))
#define USED(x)           (void)(x)
#define TRACE(...)        aux_trace_(__FILE__, __LINE__, __VA_ARGS__)

extern char *argv0;

/* use main(int argc, char *argv[]) */
#define ARGBEGIN                                                              \
        char argc_;                                                           \
        char **argv_;                                                         \
        int brk_;                                                             \
        for (argv0 = *argv, argv++, argc--;                                   \
             *argv && (*argv)[0] == '-' && (*argv)[1];                        \
             argc--, argv++) {                                                \
                if ((*argv)[1] == '-' && (*argv)[2] == '\0') {                \
                        argv++; argc--; break;                                \
                }                                                             \
                for (brk_ = 0, ++*argv, argv_ = argv;                         \
                     **argv && !brk_; ++*argv) {                              \
                        if (argv != argv_) break;                             \
                        argc_ = **argv;                                       \
                        switch (argc_)
                        /*
                        case 'a':
                                opt_a = EARGF(usage());
                                break;
                         */

#define ARGC() argc_

/* handles obsolete -NUM syntax */
#define ARGNUM                                                                \
                        case '0':                                             \
                        case '1':                                             \
                        case '2':                                             \
                        case '3':                                             \
                        case '4':                                             \
                        case '5':                                             \
                        case '6':                                             \
                        case '7':                                             \
                        case '8':                                             \
                        case '9'

#define ARGNUMF() (brk_ = 1, (int)estrtonum(*argv, 0, INT_MAX))

#define AUX_GETARG(err)                                                       \
        (((*argv)[1] == '\0' && argv[1] == NULL) ?                            \
                (err) :                                                       \
                (brk_ = 1, ((*argv)[1] != '\0') ?                             \
                        (*argv + 1) :                                         \
                        (argc--, *++argv)))


#define ARGF()     AUX_GETARG((char *)0)
#define EARGF(efn) AUX_GETARG((efn, abort(), (char *)0))

#define EARGF2NUM(efn, min, max) (estrtonum(EARGF(efn), min, max))
#define EARGF2UINT(efn)          (int)EARGF2NUM(efn, 0, INT_MAX)
#define EARGF2INT(efn)           (int)EARGF2NUM(efn, INT_MIN, INT_MAX)

#define ARGEND }}

void exits(const char *fmt, ...);
void exits2(int status, const char *fmt, ...);
void eprintf(const char *fmt, ...);
long estrtonum(const char *numstr, long minval, long maxval);
void aux_trace_(const char *file, int line, char *fmt, ...);

extern void *aux_tmp;
#define aux_alloc(fn, ...) (aux_tmp = fn(__VA_ARGS__) ? aux_tmp : (exits(#fn), nil))
#define emalloc(...)  aux_alloc(malloc,  __VA_ARGS__)
#define ecalloc(...)  aux_alloc(calloc,  __VA_ARGS__)
#define erealloc(...) aux_alloc(realloc, __VA_ARGS__)

#ifdef AUX_IMPL

void *aux_tmp;
char *argv0;

static void aux_veprintf(const char *fmt, va_list ap);

void
exits(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	aux_veprintf(fmt, ap);
	va_end(ap);

	exit(fmt && *fmt ? 1 : 0);
}

void
exits2(int status, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	aux_veprintf(fmt, ap);
	va_end(ap);

	exit(status);
}

void
eprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	aux_veprintf(fmt, ap);
	va_end(ap);
}

void
aux_veprintf(const char *fmt, va_list ap)
{
	if (!fmt || !*fmt) return;
	if (argv0 && strncmp(fmt, "usage", strlen("usage")))
		fprintf(stderr, "%s: ", argv0);

	vfprintf(stderr, fmt, ap);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else fputc('\n', stderr);
}

void
aux_trace_(const char *file, int line, char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "%s:%d ", file, line);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	putchar('\n');
}

long
estrtonum(const char *numstr, long minval, long maxval)
{
	const char *error;
	long num;
	char *ep;

	assert(numstr != NULL);
	assert(minval <= maxval);

	error = NULL;
	num = strtol(numstr, &ep, 10);
	if (numstr == ep || *ep != '\0')
		error = "invalid";
	else if ((num == LLONG_MIN && errno == ERANGE) || num < minval)
		error = "too small";
	else if ((num == LLONG_MAX && errno == ERANGE) || num > maxval)
		error = "too large";
	if (error) exits("%s: %s number", numstr, error);
	return num;
}


#endif

#define exit(status) exits2(status, 0);
#endif
