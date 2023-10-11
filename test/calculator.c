#include <stdio.h>
#include <string.h>
#include <math.h> /* for pow */

#include "standard.h"
#include "ma.h"
#include "str.h"
#include "gs.h"
#include "parser.h"
#include "regex.h"
#include "lexer.h"

const char* lexer_spec =
    "num \\d+(.\\d+)?\n"
    "@@ ( ) + - * /\n"
    "@! \\w";
const char* grammar_spec =
    "num ( ) + - * /\n"
    "@@nonterminals\n"
    "E F G S\n"
    "@@productions\n"
    "S -> E\n"
    "E -> F\n"
    "F -> G\n"
    "@paranthesis G -> ( E )\n"
    "@plus E -> E + F\n"
    "@minus E -> E - F\n"
    "@mul F -> F * G\n"
    "@div F -> F / G\n"
    "@unary_minus G -> - G\n"
    "G -> num";

typedef double attribute_t;

typedef struct extra_t {
    gsStack* val_table;
    prGrammar* grammar;
} extra_t;

double _string_to_double(const char* string_start, const char* string_end) {
    double result = 0.0;
    double exp = 1.0;
    unsigned counter = 0;
    boolean is_minus = false;
    boolean encountered_dot = false;
    const char* cursor = string_start;
    if (*cursor == '-') {
        is_minus = true;
        ++cursor;
    }
    for (; cursor < string_end; ++cursor) {
        if (*cursor == '.') {
            encountered_dot = true;
        } else {
            result += (double) (*cursor - '0') * exp;
            exp /= 10;
            if (encountered_dot == false) {
                ++counter;
            }
        }
    }
    result *= pow(10, counter-1);
    if (is_minus == true) {
        result = -result;
    }
    return result;
}

unsigned str_to_val_func(unsigned token, const char* string_start,
                         const char* string_end, void* extra) {
    const extra_t* const extra_casted = (extra_t*) extra;
    const prGrammar* const grammar = extra_casted->grammar;
    gsStack* const val_table = extra_casted->val_table;
    if (token == prGrammar_token_by_name(grammar, (char*) "num")) {
        GS_APPEND(val_table, _string_to_double(string_start, string_end),
                  attribute_t);
        return gsStack_length(val_table) - 1;
    }
    return 0;
}

void terminal_synth_fn(unsigned token, unsigned val, void* attribute,
		       void* extra) {
    attribute_t* const result = (attribute_t*) attribute;
    const extra_t* const extra_casted = (extra_t*) extra;
    const prGrammar* const grammar = extra_casted->grammar;
    const attribute_t* const val_table = gsStack_0(extra_casted->val_table);
    if (token == prGrammar_token_by_name(grammar, (char*) "num")) {
        *result = val_table[val];
    }
    return;
}

void production_synth_fn(unsigned production, void* attributes, void* extra) {
    attribute_t* const results = (attribute_t*) attributes;
    const prGrammar* const grammar = ((extra_t*) extra)->grammar;
    if (production == prGrammar_production_by_name(grammar, (char*) "paranthesis")) {
        results[0] = results[-2];
    } else if (production == prGrammar_production_by_name(grammar, (char*) "plus")) {
        results[0] = results[-3] + results[-1];
    } else if (production == prGrammar_production_by_name(grammar, (char*) "minus")) {
        results[0] = results[-3] - results[-1];
    } else if (production == prGrammar_production_by_name(grammar, (char*) "mul")) {
        results[0] = results[-3] * results[-1];
    } else if (production == prGrammar_production_by_name(grammar, (char*) "div")) {
        results[0] = results[-3] / results[-1];
    } else if (production == prGrammar_production_by_name(grammar, (char*) "unary_minus")) {
        results[0] = - results[-1];
    } else {
        results[0] = results[-1];
    }
    return;
}

int main(void) {
    lexLexer* lexer = NULL;
    prGrammar* grammar = NULL;
    prSLRParser* parser = NULL;
    gsStack* val_table = NULL;
    unsigned* tokens = NULL;
    unsigned* vals = NULL;
    unsigned* items = NULL;

    ma_initialize();

    lexer = lexLexer_create_from_spec(lexer_spec, NULL);
    if (lexer == NULL) {
        printf("There was an error forming the lexer.\n");
        goto end_label;
    }

    grammar = prGrammar_create_from_spec(grammar_spec, NULL);
    if (grammar == NULL) {
        printf("There was an error forming the grammar.\n");
        goto end_label;
    }

    parser = prSLRParser_create_from_grammar(grammar, NULL);
    if (parser == NULL) {
        printf("There was an error forming the SLR parser for the grammar.\n");
        goto end_label;
    }

    char string[1024];
    printf("Enter an expression to calculate (using floating point numbers, "
	   "+, -, *, / and brackets):\n");
    fgets(string, 1024, stdin);
    str_remove_trailing_newline_(string);

    val_table = gsStack_create(sizeof(attribute_t));

    extra_t extra;
    extra.val_table = val_table;
    extra.grammar = grammar;

    const char* lex_end_pos;
    tokens = lexLexer_process(lexer, string, str_to_val_func, &extra, &vals,
                              &lex_end_pos);

    if (*lex_end_pos != 0) {
        printf("There was an error lexing the expression.\n");
        goto end_label;
    }

    const unsigned* prs_end_pos;
    items = prSLRParser_parse(parser, tokens, &prs_end_pos);

    if (items == NULL || *prs_end_pos != 0) {
        printf("There was an error parsing the expression.\n");
        goto end_label;
    }

    attribute_t result;

    prSLRParser_synthesize(parser, items, vals, sizeof(attribute_t),
                           terminal_synth_fn, production_synth_fn, &extra,
                           &result);

    printf("The result of the calculation is: %.20f\n", result);

    end_label:;
    FREE(items);
    FREE(tokens);
    FREE(vals);
    gsStack_destroy(val_table);
    prSLRParser_destroy(parser);
    prGrammar_destroy(grammar);
    lexLexer_destroy(lexer);
    ma_finalize();
    return 0;
}
