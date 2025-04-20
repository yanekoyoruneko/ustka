/*
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <stdalign.h>
#include "types/arena.h"

 *
 * NaN is 12 bits (11 exponent and 1 mantysa MSB) set to 1
 * QNAN is 13
 * DOUBLE is any not QNAN value
 * POINTER has 48 bits of payload; there is 64 - 13 - 48 = 3 left for tag
 * one of these is used to tag other types that don't use pointer payload
 *
 * | NOT QNAN       | DOUBLE
 * | 0[N]00........ | CELL
 * | 0[N]01........ | SYMBOL
 * | 0[N]10........ | STRING
 * | 0[N]11........ | VECTOR
 * | 1[N]00........ | TABLE
 * | 1[N]01........ | FUNCTION
 * | 1[N]10........ | (reserved)
 *
 * | 1[N]11|00..... | INTEGER
 * | 1[N]11|01..... | NIL (empty list)
 * | 1[N]11|10...XX | CHARACTER
 * | 1[N]11|11....X | BOOLEAN
 *              ..1 - T
 *              ..0 - F
 */
#define PRINTES_LOC   1

#define QNAN	      0x7ffc000000000000
#define PTR_TAG_MASK  0xffff000000000000
#define CEL_MASK      0x7ffc000000000000
#define SYM_MASK      0x7ffd000000000000
#define STR_MASK      0x7ffe000000000000
#define VEC_MASK      0x7fff000000000000
#define TAB_MASK      0xfffc000000000000
#define FUN_MASK      0xfffd000000000000
/* RESERVED           0xfffe000000000000 */

#define NPTR_TAG_MASK 0xfffff00000000000
#define INT_MASK      0xffff000000000000
#define NIL_MASK      0xffff100000000000
#define BOL_MASK      0xffff200000000000
#define T_MASK        0xffff200000000001
#define F_MASK        0xffff200000000000


/* predicates */
#define DUBP(v) ((v.as_uint64 & QNAN) != QNAN)
#define CELP(v) ((v.as_uint64 &  PTR_TAG_MASK) == CEL_MASK)
#define SYMP(v) ((v.as_uint64 &  PTR_TAG_MASK) == SYM_MASK)
#define STRP(v) ((v.as_uint64 &  PTR_TAG_MASK) == STR_MASK)
#define VECP(v) ((v.as_uint64 &  PTR_TAG_MASK) == VEC_MASK)
#define TABP(v) ((v.as_uint64 &  PTR_TAG_MASK) == TAB_MASK)
#define FUNP(v) ((v.as_uint64 &  PTR_TAG_MASK) == FUN_MASK)
#define INTP(v) ((v.as_uint64 & NPTR_TAG_MASK) == INT_MASK)
#define BOLP(v) ((v.as_uint64 & NPTR_TAG_MASK) == BOL_MASK)
#define METP(v) ((v.as_uint64 & NPTR_TAG_MASK) == MET_MASK)
#define NUMP(v) (INTP(v) || DUBP(v))
#define NILP(v) ((v).as_uint64 == NIL_MASK)
#define ATMP(v) (!CELP(v))

/* get value */
#define AS_PTR(v) ((char*)((v).as_uint64 & 0xFFFFFFFFFFFF))
#define AS_CEL(v) ((Cell*)AS_PTR(v))
#define AS_SYM(v) ((char*)AS_PTR(v))
#define AS_STR(v) ((char*)AS_PTR(v))
#define AS_VEC(v) ((Vec*) AS_PTR(v))
#define AS_TAB(v) ((ht*)  AS_PTR(v))
#define AS_FUN(v) ((Fun*) AS_PTR(v))

#define AS_DUB(v) (v.as_double)
#define AS_BOL(v) (v.as_byte)
#define AS_INT(v) (v.as_int32)

#define NIL       ((Value){.as_uint64 = NIL_MASK})
#define T         ((Value){.as_uint64 = T_MASK})
#define F         ((Value){.as_uint64 = F_MASK})

/* cell */
#define CAR(p)    (AS_CEL(p)->car_)
#define CDR(p)    (AS_CEL(p)->cdr_)
#define CEL_LOC(p) (*(Range*)(AS_CEL(p)+1))
#define CEL_LEN(p) (CEL_LOC(p).len)
#define CEL_AT(p)  (CEL_LOC(p).at)

/* add tag mask */
#define TO_DUB(d) ((Value){.as_double = d})
#define TO_CEL(p) ((Value){.as_uint64 = (uint64_t)(p) | CEL_MASK})
#define TO_SYM(p) ((Value){.as_uint64 = (uint64_t)(p) | SYM_MASK})
#define TO_STR(p) ((Value){.as_uint64 = (uint64_t)(p) | STR_MASK})
#define TO_VEC(p) ((Value){.as_uint64 = (uint64_t)(p) | VEC_MASK})
#define TO_TAB(p) ((Value){.as_uint64 = (uint64_t)(p) | TAB_MASK})
#define TO_FUN(p) ((Value){.as_uint64 = (uint64_t)(p) | FUN_MASK})
#define TO_INT(i) ((Value){.as_uint64 = (uint64_t)(uint32_t)(i) | INT_MASK}) /* zero-extend negative */

#define TYPE_ERR(type, val)						\
	printf("; The value\n;\t%s\n; doesn't satisfy predicate\n;\t%s\n", valuestr(val), type)

#define AS_NUM(val) (INTP(val) ? AS_INT(val) :	      \
		     DUBP(val) ? AS_DUB(val) :	      \
		     (assert(0 && "unreachable"), 0))

#define ASSERTV(type, val) (!type(val) ? (TYPE_ERR(#type, val), 1) : 0)

#define CHECK_TYPE(type, val) (!type(val) ? (TYPE_ERR(#type, val), 1) : 0)


#define RANGEFMT "|%lu:%lu|"
#define RANGEP(range) range.at, range.len

typedef struct {
	size_t at;
	size_t len;
} Range;

typedef union {
	uint8_t  as_byte;
	int32_t  as_int32;
	uint32_t as_uint32;
	uint64_t as_uint64;
	double   as_double;
} Value;

typedef struct  {
	Value car_;
	Value cdr_;
} Cell;

typedef struct {
	Range range;
	size_t count;
} SerialRange;

typedef struct {
	const char *fname;
	Vec(uint8_t) code;
	Vec(Value) conspool;
	Vec(SerialRange) where;	/* RLE */
} Chunk;

typedef struct {
	int arrity;
	Chunk *body;
} Fun;

typedef struct {
	const char *fname;
	Arena *arena;
	Value cell;
} Sexp;

static inline Value
cons(Arena *arena, Value a, Value b)
{
	Value cell = TO_CEL(new(arena, sizeof(Cell)));
	CAR(cell) = a;
	CDR(cell) = b;
	return cell;
}

static inline Value
makefun(Chunk *body, int arrity)
{ /* let's have fun */
	Fun *fun = malloc(sizeof(Fun));
	fun->body = body;
	fun->arrity = arrity;
	return TO_FUN(fun);
}

static inline Value
econs(Arena *arena, Value a, Value b)
{
	Value cell = TO_CEL(new(arena, sizeof(Cell) + sizeof(Range)));
	CAR(cell) = a;
	CDR(cell) = b;
	return cell;
}

static inline const char *
valuestr(Value val)
{
	static char buff[BUFSIZ];
	if      (INTP(val)) snprintf(buff, BUFSIZ, "%d", AS_INT(val));
	else if (DUBP(val)) snprintf(buff, BUFSIZ, "%f", AS_DUB(val));
	else if (SYMP(val)) snprintf(buff, BUFSIZ, "%s", AS_PTR(val));
	else if (FUNP(val)) snprintf(buff, BUFSIZ, "<FUN>");
	else if (STRP(val)) snprintf(buff, BUFSIZ, "\"%s\"", AS_STR(val));
	else if (BOLP(val)) snprintf(buff, BUFSIZ, "%s", AS_BOL(val) ? "T" : "F");
	else if (NILP(val)) snprintf(buff, BUFSIZ, "()");
	else assert(0 && "valuestr: invalid type; unreachable");
	return buff;
}

inline static void
printes_(Value cell) {
	if (CELP(cell)) {
#ifdef PRINTES_LOC
		printf(RANGEFMT, RANGEP(CEL_LOC(cell)));
#endif
		putchar('(');
		printes_(CAR(cell));
		while (CELP(CDR(cell)))  {
			putchar(' ');
			cell = CDR(cell);
			printes_(CAR(cell));
		};
		if (ATMP(CDR(cell)) && !NILP(CDR(cell))) {
			fputs(" . ", stdout);
			printes_(CDR(cell));
		}
		putchar(')');
		return;
	}
	printf("%s", valuestr(cell));
}

inline static void printes(Sexp *sexp) { printes_(sexp->cell); }
