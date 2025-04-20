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
#define VM_TRACE 1

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
} Frame;

typedef struct {
	uint8_t *ip;
	Chunk *chunk;
	Frame frames[FRAME_MAX];
	Value stack[STACK_MAX];
	Ht(Value) dynamic;
	Value *sp;
	Frame *fp;
} VM;

static VM vm;

#define VM_INCIP() (*vm.ip++)
#define VM_CONS() vm.chunk->conspool[VM_INCIP()]

void
enter()
{
	vm.fp++;
	vm.fp->bsp = vm.sp;
	vm.fp->retip = vm.ip;
}

void
leave()
{
	vm.sp = vm.fp->bsp;
	vm.ip = vm.fp->retip;
	vm.fp--;
}

void
vminit()
{
	vm.sp = vm.stack;
	vm.ip = nil;
	vm.fp = vm.frames - 1;
	ht_ini(vm.dynamic);
	enter();
}

void
vmfree()
{
	ht_free(vm.dynamic);
}

void push(Value value) { *vm.sp++ = value; }
Value pop(void)  { return *(--vm.sp); }
Value peek(void) { return *(vm.sp - 1); }


#define BIN_OP(op) do {							\
		Value b_ = pop();					\
		Value a_ = pop();					\
		if (DUBP(a_) || DUBP(b_))				\
			push(TO_DUB(AS_NUM(a_) op AS_NUM(b_)));	        \
		else							\
			push(TO_INT(AS_INT(a_) op AS_INT(b_)));		\
	} while (0);


void
printstack()
{
	printf(";;; STACK BEG\n");
	for (Value* slot = vm.stack; slot < vm.sp; slot++) {
		printf(" %s ", valuestr(*slot));
		if (slot + 1 < vm.sp) putchar('|');
	}
	printf("\n;;; STACK END\n");
}

EvalErr
run()
{
	int i = 0;
	for (;;) {
#ifdef VM_TRACE
		printf(";;;; [CYCLE %04i] ;;;;;\n", ++i);
		decompile_op(vm.chunk, vm.ip - vm.chunk->code);
		printstack();
		printf(";; EXECUTING...\n");
#endif
		uint8_t opcode;
		switch (opcode = VM_INCIP()) {
		case OP_BIND: {
			size_t slot = AS_INT(VM_CONS());
			vm.fp->bsp[slot] = peek();
			break;
		}
		case OP_LOAD: {
			size_t slot = AS_INT(VM_CONS());
			push(vm.fp->bsp[slot]);
			break;
		}
		case OP_CONS: {
		        Value val = VM_CONS();
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
		case OP_ADD: BIN_OP(+); break;
		case OP_SUB: BIN_OP(-); break;
		case OP_MUL: BIN_OP(*); break;
		case OP_DIV: BIN_OP(/); break;
		case OP_CALL:
			enter();
			break;
		case OP_RET: {
			leave();
			if (vm.ip == nil)  {
				printf("; TERMINATING\n");
				return OK;
			}
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
	Chunk *chunk;
	chunk = compile(sexp);
	if (!chunk) {
		err = COMPILE_ERR;
		return err;
		/* goto RET; */
	}
	vm.chunk = chunk;
	vm.ip = chunk->code;
	decompile(chunk, "EXECUTING");
	err = run();
RET:
	chunkfree(chunk);
	return err;
}

int main(int argc, char *argv[]) {
	const char *input = argc > 1 ? argv[1] : NULL;
	Reader *reader = ropen(input);
	Sexp *sexp;
	int err = 0;
	vminit();
	do {
		if (!input) printf("> ");
		sexp = reades(reader);
		if (readerr(reader)) {
			fprintf(stderr, "%ld: %s\n", readerrat(reader), readerr(reader));
			err = EX_DATAERR;
			goto EXIT;
		}
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
