/* single probing resizable hash table / FNV-la hash */
/* depends:
#include <stdlib.h>
*/
/* NOTE:
 * This hash map is made for easy handling of primities without storing them in heap.
 * This is achieved by using macros and = operator inside `ht_set' which copies
 * the value directly.
 *
 * Althought it's easier to store primities the user has to remember to never
 * pass the hash table pointer directly because the `ht_ensure' macro will
 * override it when resizing the hash map. User has to box the hash table
 * so the newly assigned address will be not lost.
 *
 * If you want to understand this code look first into vec.h which uses the same
 * idea for storing the buffer for values and metadata.
 *
 * To minimalize to number of hash recalculations use the _idx functions with
 * already generated index from `ht_find_idx'
 */
#define HT_INI_CAP 16		/* have to be power of 2 */
#define HT_MIN_LOAD_FAC 0.65f
#define HT_OVERLOAD(ht) ((ht->len / (float)ht->cap) > HT_MIN_LOAD_FAC)

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
static inline uint64_t
hash_key(const char *key)
{
    uint64_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(uchar)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

typedef struct {
	size_t cap;
	size_t len;
	void (*del)(void *);	/* set this for eventual free */
	const char **keys;	/* if keys[idx] != nil -> ht[idx] exists */
} HT_;

#define Ht(type) type *
#define HT(type, name) type *name = ht_ini(name)

#define htptr(data) ((HT_ *)(data)-1)		   /* internals */
#define htdata(vec) ((void *)((HT_ *)(vec)+1))

#define ht_ini(data)       ht_init(data, HT_INI_CAP)
#define ht_init(data, siz) ht_init_((void **)&data, sizeof(*data), siz)
#define ht_len(data)       (htptr(data)->len)
static inline void *		/* function because returns the new vector */
ht_init_(void **data, size_t els, size_t cap) /* for assigment operation in VEC */
{
	*data = htdata(malloc(sizeof(HT_) + cap * els));
	htptr(*data)->keys = calloc(cap, sizeof(char *));
	htptr(*data)->cap = cap;
	htptr(*data)->del = (void*)0;
	htptr(*data)->len = 0;
	return *data;
}

/* gettable (element exists) predicate */
#define ht_has(ht, key)      (htptr(ht)->keys[ht_gethash(ht, key)] != (void*)0)
#define ht_has_hash(ht, idx) (htptr(ht)->keys[idx] != (void*)0)
#define ht_get(ht, key)      (ht[ht_gethash(ht, key)])
static inline size_t
ht_gethash(void *data, const char *key)
{
	uint64_t hash = hash_key(key);
	size_t idx = (size_t)(hash & (uint64_t)(htptr(data)->cap - 1)); /* fast modulo */
	while (htptr(data)->keys[idx]) {
		if (!strcmp(key, htptr(data)->keys[idx])) return idx;
		if (++idx >= htptr(data)->cap) idx = 0;
	}
	return idx;
}

#define ht_del(ht, key)      ht_del_hash(ht, ht_gethash(ht, key))
#define ht_del_hash(ht, idx) ht_del_hash_(ht, sizeof(*ht), idx)
static inline void *
ht_del_hash_(void *data, size_t els, size_t idx)
{
	if (!htptr(data)->keys[idx]) return (void*)0;
	free((char*)htptr(data)->keys[idx]);
	htptr(data)->keys[idx] = (void*)0; /* unmark key -> delete */
	void *entry = (uint8_t*)data + els * idx;
	if (htptr(data)->del) htptr(data)->del(entry);
	htptr(data)->len--;
	return entry;
}


#define ht_ensure(ht) do {						       \
	if (!HT_OVERLOAD(htptr(ht))) break;			               \
	void *nht = ht_init_((void **)&nht, sizeof(*ht), htptr(ht)->cap << 1); \
	for (size_t i = 0; i < htptr(ht)->cap; i++) {		               \
		if (!htptr(ht)->keys[i]) continue;		               \
		size_t dest = ht_gethash(nht, htptr(ht)->keys[i]);             \
		htptr(nht)->keys[dest] = htptr(ht)->keys[i];		       \
		memcpy((char*)nht + dest * sizeof(*ht), ht + i, sizeof(*ht));  \
	}							               \
	htptr(nht)->len = htptr(ht)->len;				       \
	free(htptr(ht)->keys);						       \
	free(htptr(ht));					               \
	ht = nht;						               \
} while (0)


#define ht_set(ht, key, val) do {                                              \
	size_t idx = ht_gethash(ht, key);		                       \
	if (!htptr(ht)->keys[idx]) htptr(ht)->len += 1;			       \
	ht_del_hash(ht, idx);						       \
	htptr(ht)->keys[idx] = strdup(key);				       \
	ht[idx] = val;					                       \
	ht_ensure(ht);							       \
} while (0)


#define ht_free(ht) do {						       \
	for (size_t i = 0; i < htptr(ht)->cap; i++) ht_del_hash(ht, i);        \
	free(htptr(ht)->keys);						       \
	free(htptr(ht));						       \
} while (0)
