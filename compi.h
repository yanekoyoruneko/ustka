/* compiler interface */

typedef struct Env {
	Chunk *chunk;
	Ht(size_t) binds;
	struct Env *shadowed;
} Env;

typedef enum {
	OP_RET,
	OP_BIND,
	OP_LOAD,
	OP_PUSH,
	OP_NEG,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_EQ,
	OP_JF,
	OP_JMP,
	OP_DIV,
	OP_POP,
	OP_CALL,
} OpCode;

Chunk   *chunknew(void);
void     chunkfree(Chunk *chunk);
Range    whereis(Chunk *chunk, ptrdiff_t offset);
void     emitcons(Env *env, Value val, Range pos);
Env     *shadow(Env **env);
Chunk   *retenv(Env **env);
void     emit(Env *env, uint8_t byte, Range pos);
size_t   makebind(Env *env, const char *name);
size_t   emitload(Env *env, const char *name, Range pos);
size_t   emitbind(Env *env, const char *name, Range pos);
void     emitpush(Env *env, Value val, Range pos);
size_t  emitjump(Env *env, int jmp_op, Range pos);
void     setjump(Env *env, size_t jmp);
