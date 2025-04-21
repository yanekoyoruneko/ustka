#include "aux.h"
#include "types/vec.h"
#include "types/arena.h"
#include "types/sexp.h"
#include "types/ht.h"
#include "read.h"
#include "compi.h"
#include "decomp.h"
#include "comp.h"

#define STACK_MAX 4096
#define FRAME_MAX 4096
#define VM_TRACE

/*;; Glorious Lisp Virtual Machine (GLVM) ;;*/
typedef enum {
	OK,
	COMPILE_ERR,
	RUNTIME_ERR,
} EvalErr;

typedef struct {
	const char *name;
	size_t idx;
} Bind;

typedef struct {
	Value *bsp;
	uint8_t *retip;
	Chunk *retchunk;
} Frame;

typedef struct {
	uint8_t *ip;
	Env *env;
	Frame frames[FRAME_MAX];
	Value stack[STACK_MAX];
	Ht(Value) dynamic;
	Value *sp;
	Frame *fp;
} VM;

static VM vm;

static void
enter()
{
	vm.fp++;
	vm.fp->bsp = vm.sp;
	vm.fp->retip = vm.ip;
	vm.fp->retchunk = vm.env->chunk;
}

static void
leave()
{
	vm.sp = vm.fp->bsp;
	vm.ip = vm.fp->retip;
	vm.env->chunk = vm.fp->retchunk;
	vm.fp--;
}

static void
vminit()
{
	vm.sp = vm.stack;
	vm.ip = nil;
	vm.fp = vm.frames - 1;
	vm.env = nil;
	ht_ini(vm.dynamic);
	shadow(&vm.env);
	enter();
}

static void
vmfree()
{
	ht_free(vm.dynamic);
}

inline static void push(Value v) { *vm.sp++ = v; }
inline static Value pop(void)    { return *(--vm.sp); }
inline static Value peek(void)   { return *(vm.sp - 1); }
inline static uint8_t fetch()    { return *vm.ip++; }
inline static Value imm()        { return vm.env->chunk->conspool[fetch()]; }
inline static int toplevel()     { return vm.fp == vm.frames; }


#define BIN_OP(op) do {							\
		Value b_ = pop();					\
		Value a_ = pop();					\
		if (DUBP(a_) || DUBP(b_))				\
			push(TO_DUB(AS_NUM(a_) op AS_NUM(b_)));	        \
		else							\
			push(TO_INT(AS_INT(a_) op AS_INT(b_)));		\
	} while (0);


static void
printstack()
{
	printf(";;; STACK BEG\n");
	for (Value* slot = vm.stack; slot < vm.sp; slot++) {
		printf(" %s ", valuestr(*slot));
		if (slot + 1 < vm.sp) putchar('|');
	}
	printf("\n;;; STACK END\n");
}
/*
  (define x 3)
  x ;; should be delete from stack or other locals won't work
  (define x
  (lambda (x)
      x))


 */

EvalErr
run()
{
#ifdef VM_TRACE
	int i = 0;
#endif
	for (;;) {
#ifdef VM_TRACE
		printf(";;;; [CYCLE %04i] ;;;;;\n", ++i);
		decompile_op(vm.env->chunk, vm.ip - vm.env->chunk->code);
		printstack();
		printf(";; EXECUTING...\n");
#endif
		uint8_t opcode;
		switch (opcode = fetch()) {
		case OP_BIND: {	/* this doesn't really do anything because 'slot' is already stack top */
			/* size_t slot = AS_INT(imm()); */
			/* vm.fp->bsp[slot] = peek(); */
			break;
		}
		case OP_LOAD: {
			size_t slot = AS_INT(imm());
			push(vm.fp->bsp[slot]);
			break;
		}
		case OP_PUSH: {
		        Value val = imm();
			push(val);
			break;
		}
		case OP_NEG:{
			Value val = pop();
			if (ASSERTV(NUMP, val)) break;
			if INTP(val) push(TO_INT(-AS_INT(val)));
			else if DUBP(val) push(TO_DUB(-AS_DUB(val)));
			break;
		}
		case OP_POP: pop(); break;
		case OP_JMP: {
			int off = fetch();
			vm.ip += off;
			break;
		} case OP_JF: {
			Value a = pop();
			if (IS_F(a)) {
				int off = fetch();
				vm.ip += off;
				break;
			}
			fetch();
			break;
		} case OP_EQ: {
			Value a = pop();
			Value b = pop();
			if (AS_INT(a) == AS_INT(b)) {
				push(T);
				break;
			}
			push(F);
			break;
		} case OP_ADD: BIN_OP(+); break;
		case OP_SUB: BIN_OP(-); break;
		case OP_MUL: BIN_OP(*); break;
		case OP_DIV: BIN_OP(/); break;
		case OP_CALL: {
		        Value fun = pop();
			enter();
			vm.ip = AS_FUN(fun)->body->code;
			vm.env->chunk = AS_FUN(fun)->body;
			vm.fp->bsp -= AS_FUN(fun)->arrity;
			break;
		}
		case OP_RET: {
			if (toplevel())  {
				printf("; OK\n");
				puts(valuestr(peek()));
				return OK;
			}
			printf("; RETURNING\n");
			Value res = peek();
			leave();
			push(res);
			break;
		}
		default: assert(0 && "unreachable");
		}
#ifdef VM_TRACE
		printf("\n;; OK\n");
		printstack();
#endif
	}
}

EvalErr
eval(Sexp *sexp)
{
	EvalErr err;
	chunkfree(vm.env->chunk);
	vm.env->chunk = chunknew();
	compile(vm.env, sexp);
	if (!vm.env->chunk) {
		err = COMPILE_ERR;
		return err;
	}
	/* there is bug because chunk->code is reallocated this points to wrong address */
	/* if (!vm.ip) vm.ip = chunk->code; */
	vm.ip = vm.env->chunk->code;
#ifdef VM_TRACE
	decompile(vm.env->chunk, "EXECUTING");
#endif
	err = run();
	return err;
}


int main(int argc, char *argv[]) {
	const char *input = argc > 1 ? argv[1] : NULL;
	Reader *reader = ropen(input);
	Sexp *sexp;
	int err = 0;
	vminit();
	do {
		do {
			if (!input) printf("> ");
			sexp = reades(reader);
			if (readeof(reader)) goto EXIT;
			if (readerr(reader)) {
				fprintf(stderr, "; %ld: %s\n", readerrat(reader), readerr(reader));
				sexpfree(sexp);
			}
		} while (readerr(reader));
#ifdef VM_TRACE
		printf(";;; INPUT BEG\n");
		printes(sexp);
		printf("\n;;; INPUT END\n");
#endif
		fflush(stdout);
		switch (eval(sexp)) {
		case COMPILE_ERR: err = EX_DATAERR;  goto EXIT; break;
		case RUNTIME_ERR: err = EX_SOFTWARE; goto EXIT; break;
		case OK: break;
		}
		sexpfree(sexp);
	} while (!readeof(reader));
EXIT:
	sexpfree(sexp);
	rclose(reader);
	vmfree();
	return err;
}
