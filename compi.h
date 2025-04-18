/* compiler interface */

typedef struct SerialRange SerialRange;
typedef struct Env Env;

typedef struct {
	const char *fname;
	Vec(uint8_t) code;
	Vec(Value) conspool;
	Vec(SerialRange) where;	/* RLE */
} Chunk;

typedef struct {
	Env *env;
	size_t lexcount;
	Chunk *chunk;
} Comp;

struct Env {
	Ht(size_t) lexbind;	/* each bind reference slot on the stack */
	size_t stackp;		/* pointer to the stack slot the env starts at */
	struct Env *top;
};


typedef enum {
	OP_RET,
	OP_BIND_LEX,
	OP_BIND_DYN,
	OP_LOAD_DYN,
	OP_LOAD_LEX,
	OP_LOAD_CONS,
	OP_NEG,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
} OpCode;

void chunkfree(Chunk *chunk);

Chunk *chunknew(void);
Comp *compnew(Chunk *chunk);

void compfree(Comp *comp);
void emit(Comp *comp, uint8_t byte, Range pos);

Range whereis(Chunk *chunk, ptrdiff_t offset);

void emitcons(Comp *comp, Value val, Range pos);

void envnew(Comp *comp);
void envend(Comp *comp);

size_t emitload(Comp *comp, const char *name, Range pos);
size_t emitbind(Comp *comp, const char *name, Range pos);

void emitbind_dyn(Comp *comp, const char *name, Range pos);
void emitload_dyn(Comp *comp, const char *name, Range pos);
