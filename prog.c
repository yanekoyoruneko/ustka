#define AUX_IMPL
#include "aux.h"
#include "types/arena.h"
#include "types/sexp.h"
#include "read.h"

void usage() { printf("error\n"); }

int main_(int argc, char *argv[]) {
	Reader *reader = ropen(NULL);
	Sexp *sexp;
	ARGBEGIN {
		case 'a': printf("%s", ARGF()); break;
	} ARGEND
	for (int i = 0; i < 3; i++) {
		sexp = reades(reader);
		if (readerr(reader)) {
			fprintf(stderr, "%ld: %s\n", readerrat(reader), readerr(reader));
		} else {
			printes(sexp);
			fflush(stdout);
		}
		sexpfree(sexp);
	}
	rclose(reader);
	return 0;
}
