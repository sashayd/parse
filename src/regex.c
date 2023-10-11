#include "regex.h"

#include <stdlib.h>

#include "standard.h"
#include "ma.h"
#include "str.h"
#include "gs.h"
#include "./fa.h"
#include "parser.h"

/*-------------------------*/

struct rexRegexSLRParser {
    faDfaBt self_as_faDfaBt;
};

struct rexCompiledRegex {
    faDfaOfChars self_as_faDfaOfChars;
};

struct rexCompiledRegexList {
    unsigned length;
    rexCompiledRegex** compiled_regexes;
    faDfaOfCharsRaceAux* aux;
};

/*-------------------------*/
/* regex grammar spec      */
/*-------------------------*/

static const char rex_spec[] =
    "char letter digit whitespace specific_char ( ) * + ? | \n"
    "@@nonterminals \n"
    "E F G S \n"
    "@@productions \n"
    "S -> E \n E -> F \n F -> G \n"
    "G -> ( E ) \n E -> E | F \n F -> F G \n"
    "G -> G * \n G -> G + \n G -> G ? \n"
    "G -> char \n G -> letter \n G -> digit \n"
    "G -> whitespace \n G -> specific_char";

/* the tokens in the regular expression grammar */

#define REX_UNDEFINED 0 /* 0 is always undefined token */
#define REX_C 1 /* a specific character */
#define REX_A 2 /* a letter */
#define REX_D 3 /* a digit */
#define REX_W 4 /* a white space */
#define REX_c 5 /* any charater */
#define REX_LP 6 /* ( */
#define REX_RP 7 /* ) */
#define REX_STAR 8 /* * */
#define REX_PLUS 9 /* + */
#define REX_QUESTION 10 /* ? */
#define REX_OR 11 /* | */
#define REX_S 12 /* a statement */
#define REX_SS 13 /* a formally added token, producing REX_S */

/* the productions in the regular expression grammar */

#define REX_PR_FORMAL 0 /* 0 is a formally added production REX_SS -> REX_S */
#define REX_PR_RED1 1
#define REX_PR_RED2 2
#define REX_PR_RED3 3
#define REX_PR_P 4
#define REX_PR_OR 5
#define REX_PR_AND 6
#define REX_PR_STAR 7
#define REX_PR_PLUS 8
#define REX_PR_QUESTION 9
#define REX_PR_C 10
#define REX_PR_A 11
#define REX_PR_D 12
#define REX_PR_W 13
#define REX_PR_c 14

/*-------------------------*/

void rexRegexSLRParser_destroy(rexRegexSLRParser* self) {
    faDfaBt_destroy((faDfaBt*) self);
    return;
}

rexRegexSLRParser* rexRegexSLRParser_create(void) {
    prGrammar* grammar = prGrammar_create_from_spec(rex_spec, NULL);
    if (grammar == NULL) {
	return NULL;
    }
    rexRegexSLRParser* regex_slr_parser =
	(rexRegexSLRParser*) prSLRParser_create_from_grammar(grammar, NULL);
    prGrammar_destroy(grammar);
    if (regex_slr_parser == NULL) {
	return NULL;
    }
    return regex_slr_parser;
}

void rexCompiledRegex_destroy(rexCompiledRegex* self) {
    faDfaOfChars_destroy((faDfaOfChars*) self);
}

typedef struct _rexPreprocessResult {
    const rexRegexSLRParser* regex_slr_parser;
    unsigned* char_to_token_table;
    unsigned* tokens;
    unsigned* ids;
    unsigned* tokens_C;
    unsigned* tokens_A;
    unsigned* tokens_D;
    unsigned* tokens_W;
    unsigned num_of_tokens;
} _rexPreprocessResult;

static void _rexPreprocessResult_destroy_(_rexPreprocessResult* self) {
    FREE(self->tokens);
    FREE(self->ids);
    FREE(self->tokens_W);
    FREE(self->tokens_D);
    FREE(self->tokens_A);
    FREE(self->tokens_C);
    return;
}

static _rexPreprocessResult rex_preprocess_regex(const char* regex,
						 const char *regex_end,
						 unsigned* char_to_token_table) {
    gsStack token_sequence, id_sequence;
    _rexPreprocessResult result;
    gsStack_create_(&token_sequence, sizeof(unsigned));
    gsStack_create_(&id_sequence, sizeof(unsigned));
    for (; *regex != 0 && regex != regex_end; ++regex) {
	unsigned new_token;
	unsigned new_id = 0;
	switch (*regex) {
	case '(':
	    new_token = REX_LP;
	    break;
	case ')':
	    new_token = REX_RP;
	    break;
	case '*':
	    new_token = REX_STAR;
	    break;
	case '+':
	    new_token = REX_PLUS;
	    break;
	case '?':
	    new_token = REX_QUESTION;
	    break;
	case '|':
	    new_token = REX_OR;
	    break;
	case '\\':
	    ++regex;
	    if (*regex == 0) {
		gsStack_destroy_(&token_sequence);
		gsStack_destroy_(&id_sequence);
		result.tokens = NULL;
		result.ids = NULL;
		return result;
	    }
	    switch (*regex) {
	    case 'a':
		new_token = REX_A;
		break;
	    case 'd':
		new_token = REX_D;
		break;
	    case 'w':
		new_token = REX_W;
		break;
	    case 'c':
		new_token = REX_C;
		break;
	    case 't':
		new_token = REX_c;
		new_id = '\t';
		break;
	    case 'n':
		new_token = REX_c;
		new_id = '\n';
		break;
	    case '(':
	    case ')':
	    case '*':
	    case '+':
	    case '?':
	    case '|':
	    case '\\':
		new_token = REX_c;
		new_id = *regex;
		break;
	    }
	    break;
	default:
	    new_token = REX_c;
	    new_id = *regex;
	}
	GS_APPEND(&token_sequence, new_token, unsigned);
	GS_APPEND(&id_sequence, new_id, unsigned);
    }
    GS_APPEND(&token_sequence, 0, unsigned);
    result.tokens = gsStack_0(&token_sequence);
    result.ids = gsStack_0(&id_sequence);

    for (unsigned i = 0; i < 128; ++i) {
	char_to_token_table[i] = 0;
    }
    unsigned token_counter = REX_c;
    gsStack tokens_C, tokens_A, tokens_D, tokens_W;
    gsStack_create_(&tokens_C, sizeof(unsigned));
    gsStack_create_(&tokens_A, sizeof(unsigned));
    gsStack_create_(&tokens_D, sizeof(unsigned));
    gsStack_create_(&tokens_W, sizeof(unsigned));
    unsigned* id = result.ids;
    for (unsigned* token = result.tokens; *token != 0; ++token, ++id) {
	if (*token != REX_c) {
	    continue;
	}
	if (char_to_token_table[*id] == 0) {
	    char_to_token_table[*id] = token_counter++;
	}
	GS_APPEND(&tokens_C, *id, unsigned);
	if ((*id >= 'a' && *id <= 'z') || (*id >= 'A' && *id <= 'Z')) {
	    GS_APPEND(&tokens_A, *id, unsigned);
	} else if (*id >= '0' && *id <= '9') {
	    GS_APPEND(&tokens_D, *id, unsigned);
	} else if (*id == ' ' || *id == '\t' || *id == '\n') {
	    GS_APPEND(&tokens_W, *id, unsigned);
	}
    }
    char_to_token_table[0] = REX_UNDEFINED;
    for (unsigned i = 1; i < 128; ++i) {
	if (char_to_token_table[i] == 0) {
	    if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z')) {
		char_to_token_table[i] = REX_A;
	    } else if (i >= '0' && i <= '9') {
		char_to_token_table[i] = REX_D;
	    } else if (i == ' ' || i == '\t' || i == '\n') {
		char_to_token_table[i] = REX_W;
	    } else {
		char_to_token_table[i] = REX_C;
	    }
	}
    }
    GS_APPEND(&tokens_C, 0, unsigned);
    GS_APPEND(&tokens_A, 0, unsigned);
    GS_APPEND(&tokens_D, 0, unsigned);
    GS_APPEND(&tokens_W, 0, unsigned);
    result.tokens_C = gsStack_0(&tokens_C);
    result.tokens_A = gsStack_0(&tokens_A);
    result.tokens_D = gsStack_0(&tokens_D);
    result.tokens_W = gsStack_0(&tokens_W);

    result.num_of_tokens = token_counter;

    return result;
}

static void _rex_terminal_synth_fn(unsigned token, unsigned val,
				   void* attribute, void* extra) {
    _rexPreprocessResult* const preprocess_result = extra;
    faNfa** nfa = attribute;
    switch (token) {
        case REX_C:;
	    *nfa = faNfa_create_token(REX_C);
	    for (unsigned* t = preprocess_result->tokens_C; *t != 0; ++t) {
		faNfa_add_edge_(*nfa, 0, 1,
				preprocess_result->char_to_token_table[*t]);
	    }
	    break;
        case REX_A:;
	    *nfa = faNfa_create_token(REX_A);
	    for (unsigned* t = preprocess_result->tokens_A; *t != 0; ++t) {
	        faNfa_add_edge_(*nfa, 0, 1,
				preprocess_result->char_to_token_table[*t]);
	    }
	    break;
        case REX_D:;
	    *nfa = faNfa_create_token(REX_D);
	    for (unsigned* t = preprocess_result->tokens_D; *t != 0; ++t) {
	        faNfa_add_edge_(*nfa, 0, 1,
				preprocess_result->char_to_token_table[*t]);
	    }
	    break;
        case REX_W:;
	    *nfa = faNfa_create_token(REX_W);
	    for (unsigned* t = preprocess_result->tokens_W; *t != 0; ++t) {
	        faNfa_add_edge_(*nfa, 0, 1,
				preprocess_result->char_to_token_table[*t]);
	    }
	    break;
        case REX_c:;
	    *nfa =
		faNfa_create_token(preprocess_result->char_to_token_table[val]);
	    break;
        default:;
	    *nfa = NULL;
    }
    return;
}

static void _rex_production_synth_fn(unsigned production, void* attributes,
				     void* extra) {
    faNfa** nfas = attributes;
    switch (production) {
        case REX_PR_P:;
	    *nfas = *(nfas-2);
	    break;
        case REX_PR_OR:;
	    *nfas = faNfa_sum__(*(nfas-3), *(nfas-1));
	    break;
        case REX_PR_AND:;
	    *nfas = faNfa_prod__(*(nfas-2), *(nfas-1));
	    break;
        case REX_PR_STAR:;
	    *nfas = faNfa_star__(*(nfas-2));
	    break;
        case REX_PR_PLUS:;
	    *nfas = faNfa_plus__(*(nfas-2));
	    break;
        case REX_PR_QUESTION:;
	    *nfas = faNfa_question__(*(nfas-2));
	    break;
        default:;
	    *nfas = *(nfas-1);
    }
    return;
}

rexCompiledRegex* rexCompiledRegex_create_from_regex(const rexRegexSLRParser* regex_slr_parser,
						     const char* regex,
						     const char* regex_end) {
    faDfaOfChars* const self = MALLOC(sizeof(*self));
    _rexPreprocessResult preprocess_result =
	rex_preprocess_regex(regex, regex_end,
			     self->char_to_token_table);
    if (preprocess_result.tokens == NULL) {
	FREE(self);
	return NULL;
    }
    preprocess_result.regex_slr_parser = regex_slr_parser;
    preprocess_result.char_to_token_table = self->char_to_token_table;
    const unsigned* out_end_point;
    unsigned* parse_items =
	faDfaBt_parse((faDfaBt*) regex_slr_parser, preprocess_result.tokens,
		      &out_end_point);
    if (parse_items == NULL) {
	_rexPreprocessResult_destroy_(&preprocess_result);
	FREE(self);
	return NULL;
    }
    faNfa* nfa;
    prSLRParser_synthesize((prSLRParser*) regex_slr_parser, parse_items,
			   preprocess_result.ids, sizeof(faNfa*),
			   _rex_terminal_synth_fn, _rex_production_synth_fn,
			   &preprocess_result, &nfa);
    FREE(parse_items);
    _rexPreprocessResult_destroy_(&preprocess_result);
    faDfa_create_nfaec_(&self->dfa, nfa,
			unsignedMaybe_from_unsigned(
			    preprocess_result.num_of_tokens));
    faNfa_destroy(nfa);
    return (rexCompiledRegex*) self;
}

static char* rex_raw_str_to_regex(const char *str_start, const char *str_end) {
    char* const result = MALLOC(2 * (str_end - str_start) + 1);
    char* cursor = result;
    for (const char* s = str_start; s < str_end; ++s) {
	if (*s == '(' || *s == ')' || *s == '*' || *s == '+'
	    || *s == '?' ||  *s == '|' || *s == '\\') {
	    *cursor++ = '\\';
	}
	*cursor++ = *s;
    }
    *cursor = 0;
    return result;
}

rexCompiledRegex* rexCompiledRegex_create_from_raw_str(const rexRegexSLRParser* regex_slr_parser,
						       const char *str_start,
						       const char *str_end) {
    char* const regex = rex_raw_str_to_regex(str_start, str_end);
    rexCompiledRegex* const result =
	rexCompiledRegex_create_from_regex(regex_slr_parser, regex, NULL);
    FREE(regex);
    return result;
}

boolean rexCompiledRegex_accepts(const rexCompiledRegex* self,
				 const char* str) {
    return faDfaOfChars_accepts((faDfaOfChars*) self, str);
}

void rexCompiledRegexList_destroy(rexCompiledRegexList* self) {
    if (self == NULL) {
	return;
    }
    for (unsigned i = 0; i < self->length; ++i) {
	rexCompiledRegex_destroy(self->compiled_regexes[i]);
    }
    FREE(self->compiled_regexes);
    FREE(self->aux);
    FREE(self);
    return;
}

extern rexCompiledRegexList* rexCompiledRegexList_create_from_compiled_regex_list__(
    unsigned length,
    rexCompiledRegex** compiled_regexes) {
    rexCompiledRegexList* const self = MALLOC(sizeof(rexCompiledRegexList*));
    self->length = length;
    self->compiled_regexes = compiled_regexes;
    self->aux = MALLOC(self->length * sizeof(faDfaOfCharsRaceAux));
    return self;
}

unsigned rexCompiledRegexList_race(const rexCompiledRegexList* self,
				   const char* str,
				   const char** out_end_pos) {
    return faDfaOfChars_race(self->length,
			     (const faDfaOfChars* const*) self->compiled_regexes,
			     str, self->aux, out_end_pos);
}
