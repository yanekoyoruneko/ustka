/* compiler interface */

typedef struct SerialRange SerialRange;
typedef struct Env Env;

typedef struct {
	const char *fname;
	Vec(uint8_t) code;
	Vec(Value) conspool;
	Vec(SerialRange) where;	/* RLE */
} Chunk;

struct Env {
	Chunk *chunk;
	Ht(size_t) binds;
	struct Env *shadowed;
};


typedef enum {
	OP_RET,
	OP_BIND,
	OP_LOAD,
	OP_CONS,
	OP_NEG,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_CALL,
} OpCode;

void chunkfree(Chunk *chunk);

Chunk *chunknew(void);

void emit(Env *env, uint8_t byte, Range pos);

Range whereis(Chunk *chunk, ptrdiff_t offset);

void emitcons(Env *env, Value val, Range pos);

Env *envnew(Env **env);
void envend(Env **env);

size_t emitload(Env *env, const char *name, Range pos);
size_t emitbind(Env *env, const char *name, Range pos);
