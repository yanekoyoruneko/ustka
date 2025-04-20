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

Env *
envnew(Env **env)
{
	Env *new = malloc(sizeof(Env));
	new->chunk = chunknew();
	ht_ini(new->binds);
	if (env) {
		new->shadowed = *env;
		*env = new;
	} else {
		new->shadowed = nil;
	}
	return new;
}

void
envend(Env **env)
{
	ht_free((*env)->binds);
	chunkfree((*env)->chunk);
	Env *tofree = *env;
	*env = (*env)->shadowed;
	free(tofree);
}

void
emit(Env *env, uint8_t byte, Range pos)
{
	vec_push(env->chunk->code, byte);
	if (vec_len(env->chunk->where) > 0 && vec_end(env->chunk->where).range.at == pos.at) {
		vec_end(env->chunk->where).count++;
	} else {
		SerialRange range = {.range = pos, .count = 1};
		vec_push(env->chunk->where, range);
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
emitcons(Env *env, Value val, Range pos)
{
	vec_push(env->chunk->conspool, val);
	emit(env, vec_len(env->chunk->conspool) - 1, pos);
}

static size_t
findbind(Env *env, const char *name)
{
        for (Env *it = env; it; it = it->shadowed) {
		size_t hash = ht_gethash(it->binds, name);
		if (!ht_has_hash(it->binds, hash)) continue;
		return it->binds[hash];
	}
	return -1;
}

static size_t
makebind(Env *env, const char *name)
{
	size_t hash = ht_gethash(env->binds, name);
	if (ht_has_hash(env->binds, hash)) return env->binds[hash];
	int len = ht_len(env->binds);
	ht_set(env->binds, name, len);
	return len;
}

size_t
emitload(Env *env, const char *name, Range pos)
{
	size_t bind;
	if ((bind = findbind(env, name)) == SIZE_MAX) return -1;
	emit(env, OP_LOAD, pos);
	emitcons(env, TO_INT(bind), pos);
	return bind;
}

size_t
emitbind(Env *env, const char *name, Range pos)
{
	size_t bind = makebind(env, name);
	emit(env, OP_BIND, pos);
	emitcons(env, TO_INT(bind), pos);
	return bind;
}
