/*
#include "types/value.h"
#include "types/sexp.h"
#include "types/vec.h"
#include "types/ht.h"
 */

char *compileerr(Env *env);
size_t compileerrat(Env *env);

Chunk *compile(Env *env, Sexp *sexp);
Range whereis(Chunk *chunk, ptrdiff_t offset);
void chunkfree(Chunk *chunk);
Chunk *chunknew(void);
