#include "aux.h"
#include "types/arena.h"
#include "types/sexp.h"
#include "types/vec.h"
#include "types/ht.h"
#include "compi.h"

struct SerialRange {
	Range range;
	size_t count;		/* count of repetitive ranges */
};


void
chunkfree(Chunk *chunk)
{
	vec_free(chunk->code);
	vec_free(chunk->where);
	vec_free(chunk->conspool);
	free(chunk);
}

Chunk *
chunknew(void)
{
	Chunk *chunk = malloc(sizeof(Chunk));
	vec_ini(chunk->code);
	vec_ini(chunk->where);
	vec_ini(chunk->conspool);
	return chunk;
}

Comp *
compnew(Chunk *chunk)
{
	Comp *comp = malloc(sizeof(Comp));
	comp->env = nil;
	comp->lexcount = 0;
	comp->chunk = chunk;
	envnew(comp);
	return comp;
}

void
compfree(Comp *comp)
{
	envend(comp);
	free(comp);
}

void
emit(Comp *comp, uint8_t byte, Range pos)
{
	vec_push(comp->chunk->code, byte);
	if (vec_len(comp->chunk->where) > 0 && vec_end(comp->chunk->where).range.at == pos.at) {
		vec_end(comp->chunk->where).count++;
	} else {
		SerialRange range = {.range = pos, .count = 1};
		vec_push(comp->chunk->where, range);
	}
}

Range
whereis(Chunk *chunk, ptrdiff_t offset)
{
	size_t i = 0;
	ptrdiff_t cursor = chunk->where[0].count;
	while (cursor <= offset)
		cursor += chunk->where[++i].count;
	return chunk->where[i].range;
}

void
emitcons(Comp *comp, Value val, Range pos)
{
	vec_push(comp->chunk->conspool, val);
	emit(comp, vec_len(comp->chunk->conspool) - 1, pos);
}

static size_t
findbind(Comp *comp, const char *name)
{
        for (Env *env = comp->env; env; env = env->top) {
		size_t idx = ht_find_idx(env->lexbind, name);
		if (!ht_idxp(env->lexbind, idx)) continue;
		return env->lexbind[idx];
	}
	return -1;
}

static size_t
makebind(Comp *comp, const char *name)
{
	size_t idx = ht_find_idx(comp->env->lexbind, name);
	if (ht_idxp(comp->env->lexbind, idx)) return comp->env->lexbind[idx];
	ht_set(comp->env->lexbind, name, comp->lexcount++);
	return comp->lexcount - 1;
}

void
envnew(Comp *comp)
{
	Env *new = malloc(sizeof(Env));
	ht_ini(new->lexbind);
	new->stackp = comp->lexcount;
	new->top = comp->env;
	comp->env = new;
}

void
envend(Comp *comp)
{
	ht_free(comp->env->lexbind);
	comp->lexcount = comp->env->stackp;
	Env *env = comp->env;
	comp->env = comp->env->top;
	free(env);
}

size_t
emitload(Comp *comp, const char *name, Range pos)
{
	size_t bind;
	if ((bind = findbind(comp, name)) == SIZE_MAX) return -1;
	emit(comp, OP_LOAD_LEX, pos);
	emitcons(comp, TO_INT(bind), pos);
	return bind;
}

size_t
emitbind(Comp *comp, const char *name, Range pos)
{
	size_t bind = makebind(comp, name);
	emit(comp, OP_BIND_LEX, pos);
	emitcons(comp, TO_INT(bind), pos);
	return bind;
}

void
emitbind_dyn(Comp *comp, const char *name, Range pos)
{
	emit(comp, OP_BIND_DYN, pos);
	emitcons(comp, TO_STR(name), pos);
}

void
emitload_dyn(Comp *comp, const char *name, Range pos)
{
	emit(comp, OP_LOAD_DYN, pos);
	emitcons(comp, TO_STR(name), pos);
}
