#include "aux.h"
#include "types/arena.h"
#include "types/vec.h"
#include "types/sexp.h"
#include "types/ht.h"
#include "compi.h"
#include "comp.h"
#include "read.h"

enum {
	ERR,
	OK,
};

static void compile_(Env *env, Value cell);

static void
compilerest(Env *env, Value cell)
{
	if (NILP(cell)) return;

	if (ATMP(CAR(cell))) {
		/* here emit push instructions for all the literal values */
		if (INTP(CAR(cell))) {
			emitpush(env, CAR(cell), (Range){0, 0});
		} else if (SYMP(CAR(cell))) {
			emitload(env, AS_PTR(CAR(cell)), (Range){0, 0});
		}
		compilerest(env, CDR(cell));
		return;
	}

	if (CELP(CAR(cell))) {
		/* if it's new list this is function call so dispatch it */
		compile_(env, CAR(cell));
		compilerest(env, CDR(cell));
		return;
	}

	assert(0 && "unreachable");
}

static void
compile_(Env *env, Value cell)
{/* TODO: typecheck */
	if (NILP(cell)) return;

	if (ATMP(cell)) {
		compilerest(env, cell);
		return;
	}

	if (ATMP(CAR(cell))) {
		if (SYMP(CAR(cell))) {
			if (!strcmp(AS_PTR(CAR(cell)), "+")) {
				compilerest(env, CDR(cell));
				emit(env, OP_ADD, (Range){0, 0});
				return;
			}

			if (!strcmp(AS_PTR(CAR(cell)), "=")) {
				compilerest(env, CDR(cell));
				emit(env, OP_EQ, (Range){0, 0});
				return;
			}

			if (!strcmp(AS_PTR(CAR(cell)), "lambda")) {
				if (!CELP(CDR(cell))) return;
				if (!CELP(CAR(CDR(cell)))) return;

				shadow(&env);
				int arrity = 0;
				for (Value arg = CAR(CDR(cell)); !NILP(arg); arg = CDR(arg), arrity++) {
					makebind(env, AS_PTR(CAR(arg)));
				}
				compile_(env, CAR(CDR(CDR(cell))));
				emit(env, OP_RET, (Range){0, 0});

				Chunk *body = retenv(&env);
				emitpush(env, makefun(body, arrity), (Range){0, 0}); /* this is leaking */
				return;
			}

			if (!strcmp(AS_PTR(CAR(cell)), "define")) {
				if (length(cell) != 3)     return;
				if (!SYMP(CAR(CDR(cell)))) return;
				compile_(env, CAR(CDR(CDR(cell))));
				emitbind(env, AS_PTR(CAR(CDR(cell))), (Range){0, 0});
				return;
			}

			if (!strcmp(AS_PTR(CAR(cell)), "if")) {
				if (length(cell) != 4)     return;
				compile_(env, CAR(CDR(cell)));
				emitjump(env, (Range){0, 0});
				compile_(env, CAR(CDR(CDR(cell))));
				compile_(env, CAR(CDR(CDR(CDR(cell)))));
				return;
			}

			compilerest(env, CDR(cell));
			if (emitload(env, AS_PTR(CAR(cell)), (Range){0, 0}) == SIZE_MAX) {
				exits("\t`%s'\n\tis unbound", AS_PTR(CAR(cell)));
			}
			emit(env, OP_CALL, (Range){0, 0});
			return;
		}

		assert(0 && "unreachable");
	}

	if (CELP(CAR(cell))) {
		for (Value arg = CDR(cell); !NILP(arg); arg = CDR(arg)) {
			compile_(env, CAR(arg));
		}
		compile_(env, CAR(cell));
		emit(env, OP_CALL, (Range){0, 0});
		return;
	}

	assert(0 && "unreachable");
}

Chunk *
compile(Env *env, Sexp *sexp)
{
	compile_(env, sexp->cell);
	emit(env, OP_RET, (Range){0, 0});
	return env->chunk;
}
