#include "aux.h"
#include "types/arena.h"
#include "types/sexp.h"
#include "types/vec.h"
#include "types/ht.h"
#include "compi.h"
#include "comp.h"
#include "read.h"


static void compile_(Cell *cell, Chunk *chunk);

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
	if (!cell) return;

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

}
#endif

Chunk *
compile(Sexp *sexp)
{
	Value cell = sexp->cell;
	Chunk *chunk = chunknew();
	Comp *comp = compnew(chunk);
	/* compile_(cell, chunk); */
	emit(comp, OP_LOAD_CONS, (Range){0, 0});
	emitcons(comp, TO_INT(3), (Range){0, 0});
	/*
	ht_set(comp->env->lexbind, "val",3);
	printf(";;; VALUE: %d\n", ht_find_idx(comp->env->lexbind, "val"));
	*/
	emitbind(comp, "val", (Range){0, 0});
	emitload(comp, "val", (Range){0, 0});
	emit(comp, OP_RET, (Range){0, 0});
	compfree(comp);
	return chunk;
}
