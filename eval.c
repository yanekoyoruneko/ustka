#include "aux.h"
#include "types/vec.h"
#include "types/value.h"
#include "types/arena.h"
#include "types/sexp.h"
#include "types/ht.h"
#include "read.h"
#include "compi.h"
#include "decomp.h"
#include "comp.h"

#define STACK_MAX 4096
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
	uint8_t *ip;
	Chunk *chunk;
	Value stack[STACK_MAX];
	Ht(Value) dynamic;
	Value *bsp;
	Value *sp;
} VM;

static VM vm;

#define VM_INCIP() (*vm.ip++)
#define VM_CONS() vm.chunk->conspool[VM_INCIP()]

void
vminit()
{
	vm.sp = vm.stack;
	ht_ini(vm.dynamic);
}

void
vmfree()
{
	ht_free(vm.dynamic);
}

void push(Value value) { *vm.sp++ = value; }
Value pop(void)  { return *(--vm.sp); }
Value peek(void) { return *(vm.sp); }

void
framenew()
{
	push(TO_INT(vm.bsp - vm.stack)); /* right now the vm only supports int32 */
	vm.bsp = vm.sp;
}

void
framedel()
{
	vm.sp = vm.bsp;
	vm.bsp = vm.stack + AS_INT(pop());
}


#define BIN_OP(op) do {							\
		Value b_ = pop();					\
		Value a_ = pop();					\
		if (DOUBLP(a_) || DOUBLP(b_))				\
			push(TO_DOUBL(AS_NUM(a_) op AS_NUM(b_)));	\
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
		case OP_BIND_DYN: {
			const char *bind = AS_PTR(VM_CONS());
			ht_set(vm.dynamic, bind, pop());
			break;
		}
		case OP_LOAD_DYN: {
			const char *bind = AS_PTR(VM_CONS());
			push(ht_get(vm.dynamic, bind));
			break;
		}
		case OP_BIND_LEX: {
			size_t slot = AS_INT(VM_CONS());
			vm.sp[slot] = pop();
			break;
		}
		case OP_LOAD_LEX: {
			size_t slot = AS_INT(VM_CONS());
			push(vm.sp[slot]);
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
			else if DOUBLP(val) push(TO_DOUBL(-AS_DOUBL(val)));
			break;
		}
		case OP_ADD: BIN_OP(+); break;
		case OP_SUB: BIN_OP(-); break;
		case OP_MUL: BIN_OP(*); break;
		case OP_DIV: BIN_OP(/); break;
		case OP_RET: {
			printf(";; STACK TOP: %s\n", valuestr(pop()));
			printf("; TERMINATING\n");
			return OK;
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
		goto RET;
	}
	vm.chunk = chunk;
	vm.ip = chunk->code;
	decompile(chunk, "EXECUTING");
	err = run();
RET:
	chunkfree(chunk);
	return err;
}

int main_(int argc, char *argv[]) {
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
