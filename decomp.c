#include "aux.h"
#include "types/arena.h"
#include "types/sexp.h"
#include "types/vec.h"
#include "types/ht.h"
#include "read.h"
#include "compi.h"
#include "comp.h"
#include "decomp.h"

static ptrdiff_t
op_basic(const char *name, ptrdiff_t offset)
{
	printf("%-"CODE_COL"s\n", name);
	return offset + 1;
}

static ptrdiff_t
op_comp(Chunk *chunk, const char *name, ptrdiff_t offset, size_t arglen)
{
	printf("%-"CODE_COL"s ; ", name);
	for (int i = 0; arglen - i > 0; i++) {
		char buff[atoi(ARGS_COL)];
		size_t idx = chunk->code[offset + i + 1];
		snprintf(buff, atoi(ARGS_COL), "%s", valuestr(chunk->conspool[idx]));
		printf("%s", buff);
		if (i - 1 > 0) printf(", ");
	}
	putchar('\n');
	return offset + arglen + 1;
}

static void
printcol(const char *width)
{
	printf(";-");
	for (int i = 0; i < atoi(width); i++) putchar('-');
	printf("-");
}

static void
decompile_header(const char *name)
{
	printf(DECOMP_HEADER"\n", name, "BYTE", "WHERE", "OPCODE", "ARGS");
	printcol(BYTE_COL);
	printcol(WHERE_COL);
	printcol(CODE_COL);
	printcol(ARGS_COL);
	printf("\n");
}

static ptrdiff_t
decompile_op_(Chunk *chunk, ptrdiff_t offset)
{
	static Range lastrange;
	printf("; %0"BYTE_COL"ld ; ", offset);
        Range where = whereis(chunk, offset);
	if (offset > 0 && lastrange.at == where.at) {
		printf("%-"WHERE_COL"s ", "-");
	} else {
		char buff[BUFSIZ];
		snprintf(buff, BUFSIZ, "%-4ld %ld ", where.at, where.len);
		printf("%-8s", buff);
	}
	printf("; ");
	lastrange = where;

	uint8_t instr = chunk->code[offset];
	switch (instr) {
	case OP_LOAD_DYN: return op_comp(chunk, "LOAD_DYN", offset, 1);
	case OP_BIND_DYN: return op_comp(chunk, "BIND_DYN", offset, 1);
	case OP_LOAD_LEX: return op_comp(chunk, "LOAD_LEX", offset, 1);
	case OP_BIND_LEX: return op_comp(chunk, "BIND_LEX", offset, 1);
	case OP_LOAD_CONS:return op_comp(chunk, "LOAD_CONS", offset, 1);
	case OP_RET:      return op_basic("RET", offset);
	case OP_NEG:      return op_basic("NEG", offset);
	case OP_ADD:      return op_basic("ADD", offset);
	case OP_SUB:      return op_basic("SUB", offset);
	case OP_MUL:      return op_basic("MUL", offset);
	case OP_DIV:      return op_basic("DIV", offset);
	default:
		printf("; Unknown opcode %d\n", instr);
		return offset + 1;
	}

}

ptrdiff_t
decompile_op(Chunk *chunk, ptrdiff_t offset)
{
	decompile_header("OPCODE");
	return decompile_op_(chunk, offset);
}


void
decompile(Chunk *chunk, const char *name)
{
	decompile_header(name);
	for (size_t offset = 0; offset < vec_len(chunk->code);)
		offset = decompile_op_(chunk, offset);
	printf(";\n");
	printf(";;; [END]\n");
}
