#include "aux.h"
#include "types/arena.h"
#include "types/sexp.h"
#include "types/vec.h"
#include "types/ht.h"
#include "compi.h"
#include "comp.h"
#include "read.h"


static void compile_(Value cell, Chunk *chunk);

#if 0
static void
compilerest(Value cell, Chunk *chunk)
{
	if (!cell) return;
	if (ATMP(CAR(cell))) {
		/* here emit push instructions for all the literal values */
		if (CAR(cell)->type == A_INT) {
			emitcons(TO_INT(CAR(cell)->integer), CELL_LOC(CAR(cell)));
		}
	} else if (CONSP(CAR(cell))) {
		/* if it's new list this is function call so dispatch it */
		compile_(CAR(cell), chunk);
	}
	compilerest(CDR(cell), chunk);
}

static void
compile_(Value cell, Chunk *chunk)
{
	Value cell = sexp->cell;
	Chunk *chunk = chunknew();
	Comp *comp = compnew(chunk);

	if (ATMP(CAR(cell))) {
		if (CAR(cell)->type == A_SYM) {
			if (!strcmp(CAR(cell)->string, "+")) {
				compilerest(CDR(cell), chunk);
				emit(OP_ADD, CELL_LOC(CAR(cell)));
				return;
			}
		}
	} else {
		/* this is unreachable case right now */
		assert(0 && "unreachable");
	}
	compfree(comp);
	return chunk;

}
#endif

Chunk *
compile(Sexp *sexp)
{
	Value cell = sexp->cell;
	Env *env = envnew(nil);

	/* compile_(cell, chunk); */

	emit(env, OP_CONS, (Range){0, 0});
	emitcons(env, TO_INT(3), (Range){0, 0});
	emitbind(env, "1", (Range){0, 0});

	emit(env, OP_CONS, (Range){0, 0});
	emitcons(env, TO_INT(-190), (Range){0, 0});
	emitbind(env, "2", (Range){0, 0});

	emit(env, OP_CONS, (Range){0, 0});
	emitcons(env, TO_DUB(2137), (Range){0, 0});
	emitbind(env, "3", (Range){0, 0});

	emitload(env, "2", (Range){0, 0});
	emit(env, OP_RET, (Range){0, 0});
	return env->chunk;
}
