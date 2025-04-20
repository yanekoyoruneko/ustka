#include "aux.h"
#include "types/arena.h"
#include "types/vec.h"
#include "types/sexp.h"
#include "types/ht.h"
#include "compi.h"
#include "comp.h"
#include "read.h"


static void compile_(Env *env, Value cell);

static void
compilerest(Env *env, Value cell)
{
	if (NILP(cell)) return;
	if (ATMP(CAR(cell))) {
		/* here emit push instructions for all the literal values */
		if (INTP(CAR(cell))) {
			emit(env, OP_CONS, (Range){0, 0});
			emitcons(env, CAR(cell), (Range){0, 0});
		} else if (SYMP(CAR(cell))) {
			emitload(env, AS_PTR(CAR(cell)), (Range){0, 0});
		}
	} else if (CELP(CAR(cell))) {
		/* if it's new list this is function call so dispatch it */
		compile_(env, CAR(cell));
	}
	compilerest(env, CDR(cell));
}

static void
compile_(Env *env, Value cell)
{
	if (ATMP(cell)) {
		if (SYMP(cell)) {
			emitload(env, AS_PTR(cell), (Range){0, 0});
		} else if (INTP(cell)) {
			emit(env, OP_CONS, (Range){0, 0});
			emitcons(env, cell, (Range){0, 0});
		}
	} else if (ATMP(CAR(cell))) {
		if (SYMP(CAR(cell))) {
			if (!strcmp(AS_PTR(CAR(cell)), "+")) {
				compilerest(env, CDR(cell));
				emit(env, OP_ADD, (Range){0, 0});
				return;
			}
			if (!strcmp(AS_PTR(CAR(cell)), "lambda")) {
				envnew(&env);
				int arrity = 0;
				CHECK_TYPE(CELP, CDR(cell));
				CHECK_TYPE(CELP, CAR(CDR(cell)));
				for (Value arg = CAR(CDR(cell)); !NILP(arg); arg = CDR(arg), arrity++) {
					makebind(env, AS_PTR(CAR(arg)));
				}
				printf("arrity = %d\n", arrity);
				printes_(CDR(CDR(cell)));
				compile_(env, CAR(CDR(CDR(cell))));
				emit(env, OP_RET, (Range){0, 0});
				Chunk *body = envend(&env);

				emit(env, OP_CONS, (Range){0, 0});
				emitcons(env, makefun(body, arrity), (Range){0, 0}); /* this is leaking */
				return;
			} else {
				compilerest(env, CDR(cell));
				if (emitload(env, AS_PTR(CAR(cell)), (Range){0, 0}) == SIZE_MAX) {
					exits("\t`%s'\n\tis unbound", AS_PTR(CAR(cell)));
				}
				emit(env, OP_CALL, (Range){0, 0});
			}
		}
	} else if (CELP(CAR(cell))) {
		for (Value arg = CDR(cell); !NILP(arg); arg = CDR(arg)) {
			compile_(env, CAR(arg));
		}
		compile_(env, CAR(cell));
		emit(env, OP_CALL, (Range){0, 0});
	} else {

		/* this is unreachable case right now */
		assert(0 && "unreachable");
	}
}

Chunk *
compile(Sexp *sexp)
{
	Value cell = sexp->cell;
	Env *env = envnew(nil);
	compile_(env, cell);

	/* emit(env, OP_CONS, (Range){0, 0}); */
	/* emitcons(env, TO_INT(3), (Range){0, 0}); */
	/* emitbind(env, "1", (Range){0, 0}); */

	/* emit(env, OP_CONS, (Range){0, 0}); */
	/* emitcons(env, TO_INT(-190), (Range){0, 0}); */
	/* emitbind(env, "2", (Range){0, 0}); */

	/* emit(env, OP_CONS, (Range){0, 0}); */
	/* emitcons(env, TO_DUB(2137), (Range){0, 0}); */
	/* emitbind(env, "3", (Range){0, 0}); */

	/* emitload(env, "2", (Range){0, 0}); */
	emit(env, OP_RET, (Range){0, 0});
	return envend(&env);
}
