/* depends:
#include <stdlib.h>
*/

/* TODO: vec_pop, vec_del and shrink */
#define VEC_INI_CAP 16

/* because vec can be of any type these have to be macros */
/* dont question my decisions, i know what im doing */
typedef struct {
	size_t cap;
	size_t len;
} Vec_;

#define Vec(type) type *
#define VEC(type, name) type *name = vec_ini(name) /* shorthand */
#define vecptr(data) ((Vec_ *)(data)-1)		   /* internals */
#define vecdata(vec) ((void *)((Vec_ *)(vec) + 1))

/* I'm using functions because they can't be used as accesors */
static inline size_t vec_len(void *data) { return vecptr(data)->len; }
static inline size_t vec_cap(void *data) { return vecptr(data)->cap; }
#define vec_siz(data) (sizeof(Vec_) + sizeof(*data) + vecptr(data)->cap)
#define vec_end(data) (data[vec_len(data) - 1])


#define vec_ini(data) vec_init(data, VEC_INI_CAP)
#define vec_init(data, siz) vec_init_((void **)&data, sizeof(*data), siz)
static inline void *		/* function because returns the new vector */
vec_init_(void **data, size_t els, size_t siz) /* for assigment operation in VEC */
{
	*data = vecdata(malloc(sizeof(Vec_) + els * siz));
	vecptr(*data)->cap = siz;
	vecptr(*data)->len = 0;
	return *data;
}


#define vec_free(data) do {                                                    \
	free(vecptr(data));					               \
	data = NULL;                                                           \
} while (0)

/* At the end the `data' is reasigned with new buffer
 * because of this you shouldn't pass vec directly in function arguments
 * it will override only the Vec pinter inside fun but not the passed parameter
 */
#define vec_ensure(data, siz) do {                                             \
	if (vec_len(data) + (siz) > vec_cap(data)) {                           \
		vecptr(data)->cap = vec_len(data) << 1;                        \
		if (vec_len(data) + (siz) > vec_cap(data))                     \
			vecptr(data)->cap = vec_len(data) + (siz);             \
		data = vecdata(realloc(vecptr(data), vec_siz(data)));          \
	}                                                                      \
} while (0)


/* I would like it to return the inserted index */
/* but because it's more convenient to insert literal elements it is a macro */
#define vec_push(data, el) do {					               \
	vec_ensure(data, 1);		                                       \
	(data)[vecptr(data)->len++] = el;                             	       \
} while (0)
