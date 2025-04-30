/*;; The Sexp Reader For More Civilized Age ;;*/
/* todo: add macro suppprt */
#include "aux.h"
#include "types/vec.h"
#include "types/arena.h"
#include "types/sexp.h"
#include "read.h"


typedef struct {
	enum {
		OK,
		EOF_ERR,
		UNEXPECTED_EOF_ERR,
		DOT_MANY_FOLLOW_ERR,
		NOTHING_BEFORE_DOT_ERR,
		NOTHING_AFTER_DOT_ERR,
		DOT_CONTEXT_ERR,
		UNMATCHED_BRA_ERR,
		UNMATCHED_KET_ERR,
		UNMATCHED_SBRA_ERR,
		UNMATCHED_SKET_ERR,
	} type;
	size_t at;
} ReadErr;

const char *READ_ERR[] = {
	[OK]                  	 = "READER SAYS OK :)",
	[EOF_ERR]                = "end of file",
	[UNEXPECTED_EOF_ERR]   	 = "unexpected end of file",
	[DOT_MANY_FOLLOW_ERR]    = "more than one object follows . in list",
	[NOTHING_BEFORE_DOT_ERR] = "nothing appears before . in list.",
	[NOTHING_AFTER_DOT_ERR]  = "nothing appears after . in list.",
	[DOT_CONTEXT_ERR]        = "dot context error",
	[UNMATCHED_BRA_ERR]      = "unmatched opening parenthesis",
	[UNMATCHED_KET_ERR]      = "unmatched close parenthesis",
	[UNMATCHED_SBRA_ERR]     = "unmatched opening square bracket",
	[UNMATCHED_SKET_ERR]     = "unmatched close square bracket",
};

/* BRA is opening, KET is closing parenthesis */
typedef enum {
	EOF2 = EOF,
	ERR,
	BRA,
	KET,
	SBRA,
	SKET,
	HASH,
	DOT,
	QUOTE,
	BSTICK,
	VAL,
} Token;


/* ;; TOKENIZER ;; */
struct Reader {
	FILE *input;
	const char *fname;
	size_t cursor;
	ReadErr err;
};

const char *
readerr(Reader *reader)
{
	return reader->err.type ? READ_ERR[reader->err.type] : NULL;
}

size_t
readerrat(Reader *reader)
{
	return reader->err.at;
}

Reader *
ropen(const char *fname)
{
	Reader *reader = calloc(1, sizeof(Reader));
	if (!fname) {
		reader->input = stdin;
		fname = "<stdin>";
	}
	else if (!(reader->input = fopen(fname, "r"))) {
		perror("ropen:");
		return NULL;
	}
	reader->fname = fname;
	return reader;
}

int
readeof(Reader *reader)
{
	return reader->err.type == EOF_ERR;
}

void
rclose(Reader *reader)
{
	fclose(reader->input);
	free(reader);
}

static int
istermsexp(char chr)
{
	return (strchr(")(`'.#", chr) || isspace(chr) || chr == EOF);
}

static int
rgetc(Reader *reader)
{
	char ch = fgetc(reader->input);
	if (ch != EOF) reader->cursor++;
	return ch;
}

static void
rungetc(Reader *reader, char chr)
{
	reader->cursor--; ungetc(chr, reader->input);
}

static size_t
readstr(Reader *reader, Vec(char) *str)
{
	char chr;
	int esc = 0;
	size_t pos = reader->cursor;
	while ((chr = rgetc(reader)) != EOF) {
		if (esc) { esc = 0; switch (chr) {
			case 'n':  vec_push(*str, '\n'); continue;
			case 'r':  vec_push(*str, '\r'); continue;
			case 't':  vec_push(*str, '\t'); continue;
			default:   vec_push(*str, chr);  continue;
			}
		} else if (chr == '"') break;
		if ((esc = (chr == '\\'))) continue;
		vec_push(*str, chr);
	}
	vec_push(*str, '\0');
	return reader->cursor - pos;
}

static size_t
readsym(Reader *reader, Vec(char) *str)
{
	char chr;
	size_t pos = reader->cursor;
	while (!istermsexp(chr = rgetc(reader))) {
		vec_push(*str, chr);
	}
	rungetc(reader, chr);
	vec_push(*str, '\0');
	return reader->cursor - pos;
}

static char
skipspace(Reader *reader)
{
	char chr;
	while (isspace(chr = rgetc(reader)) && chr != EOF);
	return chr;
}

static char
skipcom(Reader *reader)
{
	char chr;
	chr = skipspace(reader);
	if (chr == ';') {
		while ((chr = rgetc(reader)) != '\n' && chr != EOF);
		chr = skipspace(reader);
	}
	return chr;
}

static Token
nextitem(Arena *arena, Reader *reader, Value *val, Range *loc)
{
	char chr;
	switch (chr = skipcom(reader)) {
        case '(':  return BRA;
	case ')':  return KET;
	case '[':  return SBRA;
        case ']':  return SKET;
        case '\'': return QUOTE;
        case '`':  return BSTICK;
        case '.':  return DOT;
        case '#':  return HASH;
	case EOF:
		reader->err = (ReadErr){EOF_ERR, reader->cursor};
		return EOF;
	case '"': {		/* todo: test string */
		VEC(char, str);
		loc->at = reader->cursor - 1;
		loc->len = readstr(reader, &str) + 1; /* 1+ for quote */
		if (feof(reader->input)) {
			vec_free(str);
			reader->err = (ReadErr){EOF_ERR, reader->cursor};
			return ERR;
		}
		*val = TO_STR(enew(arena, strlen(str) + 1));
		strcpy(AS_PTR(*val), str);
		vec_free(str);
		return VAL;
	}
	default:		/* todo: add double */
		rungetc(reader, chr);
		VEC(char, sym);
		loc->at = reader->cursor;
		loc->len = readsym(reader, &sym);
		char *endptr;
		/* if looks like number it's number */
		*val = TO_INT(strtol(sym, &endptr, 10));
		if (*endptr == '\0') {
			vec_free(sym);
			return VAL;
		}
		*val = TO_SYM(enew(arena, strlen(sym) + 1));
		strcpy(AS_PTR(*val), sym);
		vec_free(sym);
		return VAL;
	}
}


/* ;; READ SEXP ;; */
/* -es stands for (e)S-expression */
static Value reades_(Arena *arena, Reader *reader, Range *loc);

static void
discardsexp(Arena *arena, Reader *reader)
{ /* Discard everything until the current sexp ends; closing KET is found. */
	Value item;
	Token tk;
	Range loc;
	ReadErr olderr = reader->err;
	int depth = 0;
	/* because NOTHING_AFTER_DOT_ERR consumes KET synchronization can be omited */
	if (reader->err.type == EOF_ERR ||
	    reader->err.type == UNEXPECTED_EOF_ERR ||
	    reader->err.type == NOTHING_AFTER_DOT_ERR)
		return;
	while (depth >= 0 && (tk = nextitem(arena, reader, &item, &loc)) != EOF) {
		switch (tk) {
		case BRA: depth++; break;
		case KET: depth--; break;
		default: break;
		}
	}
	reader->err = olderr;
}

static Value /* todo: implement all tokens / make area size fixed? */
readrest(Arena *arena, Reader *reader, Range *sexploc)
{
	Value tl = NIL;
	Value ltl = NIL;	/* location tail */
	Value hd = NIL;
	Value errel = NIL;
	Range loc;
	Token tk;
	Value item;
	sexploc->at = reader->cursor - 1;
	while ((tk = nextitem(arena, reader, &item, &loc)) != KET) {
		switch (tk) {
		case EOF:	/* this is unexpected EOF, make it unexpected */
			reader->err.type = UNEXPECTED_EOF_ERR;
			return NIL;
		case BRA:
			item = readrest(arena, reader, &loc);
			if (reader->err.type) {
				discardsexp(arena, reader);
				return NIL;
			}
			break;
		case DOT:  /* expected is DOT <sexp> KET, otherwise error */
			if (NILP(tl)) {
				reader->err = (ReadErr){
					NOTHING_BEFORE_DOT_ERR,
					reader->cursor - 1
				};
				return NIL;
			}
			size_t prevcur = reader->cursor - 1;
			CDR(tl) = reades_(arena, reader, &loc);
			Value r = cons(arena, TO_INT(loc.at), TO_INT(loc.len));
			CDR(ltl) = r;
			if (reader->err.type) {
				if (reader->err.type == UNMATCHED_KET_ERR) {
					reader->err = (ReadErr){
						NOTHING_AFTER_DOT_ERR,
						prevcur,
					};
				}
				return NIL;
			}
			/*
			 * `reades_' is used here because it doesn't consume KET
			 * In case of error hanging KET is expected to be
			 * consumed by `discardsexp' on synchronization
			 */
			prevcur = reader->err.at;
			if (!NILP(errel = reades_(arena, reader, &loc))) {
				reader->err = (ReadErr){
					DOT_MANY_FOLLOW_ERR,
					loc.at
				};
				return NIL;
			}
			if (reader->err.type != UNMATCHED_KET_ERR)
				return NIL; /* propagate further same error */
			reader->err = (ReadErr){OK, prevcur}; /* proper sexp */
			if (!NILP(hd)) sexploc->len = reader->cursor - sexploc->at;
			return hd;
		default: break;
		}
		Value cell, cloc;
		Value r = cons(arena, TO_INT(loc.at), TO_INT(loc.len));
		if (NILP(hd)) {
			cell = econs(arena, item, NIL);
			cloc = CEL_LOC(cell);
			CAR(cloc) = r;
			CDR(cloc) = NIL;
		}
		else {
			cell = cons(arena, item, NIL);
			cloc = cons(arena, r, NIL);
		}

		if (NILP(hd))  hd = cell;
		else {
			CDR(tl)  = cell;
			CDR(ltl) = cloc;
		}
		tl  = cell;
		ltl = cloc;
	}
	if (!NILP(hd)) sexploc->len = reader->cursor - sexploc->at;
	return hd;
}

static Value
reades_(Arena *arena, Reader *reader, Range *loc)
{
	Value item;
	Token tk;
	switch (tk = nextitem(arena, reader, &item, loc)) {
	case BRA: {
		Value cell = readrest(arena, reader, loc);
		if (reader->err.type) discardsexp(arena, reader);
		return cell;
	}
	case KET:  reader->err = (ReadErr){UNMATCHED_KET_ERR,  reader->cursor - 1}; return NIL;
	case SKET: reader->err = (ReadErr){UNMATCHED_SBRA_ERR, reader->cursor - 1}; return NIL;
	case DOT:  reader->err = (ReadErr){DOT_CONTEXT_ERR,    reader->cursor - 1}; return NIL;
	case EOF:  return NIL;
	case ERR:  return NIL;
	default:   return item;
	}
}

Sexp *
reades(Reader *reader)
{
	Sexp *sexp = malloc(sizeof(Sexp));
	sexp->arena = aini();	/* imagine trying to clear memory without arena */
	sexp->fname = reader->fname;
	reader->err = (ReadErr){OK, reader->cursor};
	sexp->cell = reades_(sexp->arena, reader, &sexp->loc);
	return sexp;
}

void
sexpfree(Sexp *sexp)
{
	deinit(sexp->arena);	/* I fucking love arena */
	free(sexp);
}


/* ;; PRINTER ;; */
