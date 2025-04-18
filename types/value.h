/*
#include <stdio.h>
#include <stdlib.h>
*/

typedef union {
	uint64_t as_uint;
	double as_double;
} Value;

#define NANISH	    0x7ffc000000000000 /* distinguish "our" NAN with one additional bit */
#define NANISH_MASK 0xffff000000000000 /* [SIGN/PTR_TAG] + 11*[EXP] + 2*[NANISH] + 2*[TAG] */

#define BOOL_MASK   0x7ffe000000000002  /* 2 ms + and 2 ls */
#define NULL_VALUE  0x7ffe000000000000  /* 0b*00 */
#define TRUE_VALUE  (BOOL_MASK | 3)     /* 0b*11 */
#define FALSE_VALUE (BOOL_MASK | 2)     /* 0b*10 */

#define INT_MASK 0x7ffc000000000000 /* use all of mantisa bits for integer */
#define SYM_MASK 0xfffc000000000000 /* pointers have sign bit set */
#define STR_MASK 0xfffe000000000000 /* on x86-64 ptr* is at max 48 bits long */
#define OBJ_MASK 0xfffd000000000000 /* which is small enought to put in mantysa */
#define PTR_MASK 0xf000000000000000

/* predicates */
#define DOUBLP(v) ((v.as_uint & NANISH) != NANISH)
#define NILP(v)  ((v.as_uint == NULL_VALUE)
#define BOOLP(v)  ((v.as_uint & BOOL_MASK) == BOOL_MASK)
#define PTRP(v)   ((v.as_uint & PTR_MASK) == PTR_MASK)
#define INTP(v)   ((v.as_uint & NANISH_MASK) == INT_MASK)
#define NUMP(v)   (INTP(v) || DOUBLP(v))
#define STRP(v)   ((v.as_uint & NANISH_MASK) == STR_MASK)
#define SYMP(v)   ((v.as_uint & NANISH_MASK) == SYM_MASK)
#define OBJP(v)   ((v.as_uint & NANISH_MASK) == OBJ_MASK)

/* get value */
#define AS_DOUBL(v) (v.as_double)
#define AS_BOOL(v)  ((char)(v.as_uint & 0x1))
#define AS_INT(v)   ((int32_t)(v.as_uint))
#define AS_PTR(v)   ((char *)((v).as_uint & 0xFFFFFFFFFFFF))

/* add tag mask */
#define CLEAR_TAG(p) ((uint64_t)(p) & ~NANISH_MASK)
#define TO_STR(p) ((Value){ .as_uint = (uint64_t)(p) | STR_MASK })
#define TO_SYM(p) ((Value){ .as_uint = (uint64_t)(p) | SYM_MASK })
/* negative ints have the upper bits set so tag must be cleared */
#define TO_INT(i) ((Value){ .as_uint = CLEAR_TAG((uint64_t)(i)) | INT_MASK })
#define TO_DOUBL(d) ((Value){ .as_double = d })


#define TYPE_ERR(type, val)						\
	printf("; The value\n;\t%s\n; doesn't satisfy predicate\n;\t%s\n", valuestr(val), type)

#define AS_NUM(val) (INTP(val) ? AS_INT(val) :		\
		     DOUBLP(val) ? AS_DOUBL(val) :	\
		     (assert(0 && "unreachable"), 0))

#define ASSERTV(type, val) (!type(val) ? (TYPE_ERR(#type, val), 1) : 0)


static inline const char *
valuestr(Value val)
{
	static char buff[BUFSIZ];
	if      (INTP(val))   snprintf(buff, BUFSIZ, "%d",     AS_INT(val));
	else if (DOUBLP(val)) snprintf(buff, BUFSIZ, "%f",     AS_DOUBL(val));
	else if (STRP(val))   snprintf(buff, BUFSIZ, "\"%s\"", AS_PTR(val));
	else if (SYMP(val))   snprintf(buff, BUFSIZ, "%s",     AS_PTR(val));
	else if (NILP(val))   snprintf(buff, BUFSIZ, "nil");
	else assert(0 && "valuestr: invalid type; unreachable");
	return buff;
}
