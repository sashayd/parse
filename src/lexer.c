#include "lexer.h"

#include <string.h>

#include "standard.h"
#include "ma.h"
#include "str.h"
#include "gs.h"
#include "regex.h"

/*-------------------------*/
/* structs                 */
/*-------------------------*/

struct lexLexer {
    boolean is_regex_slr_parser_borrowed;
    /* this is const only if previous bool is true;
       otherwise almost, except we must destruct it in the end... */
    const rexRegexSLRParser* regex_slr_parser;
    unsigned num_of_tokens;
    /* this is also the length of token_names */
    unsigned num_of_nonignored_tokens;
    char** token_names;
    rexCompiledRegexList* compiled_regexes;
};

/*-------------------------*/

static void lexLexer_destroy_(lexLexer* self) {
    if (self == NULL) {
        return;
    }
    rexCompiledRegexList_destroy(self->compiled_regexes);
    for (unsigned i = 0; i < self->num_of_nonignored_tokens; ++i) {
        FREE(self->token_names[i]);
    }
    FREE(self->token_names);
    if (self->is_regex_slr_parser_borrowed == false) {
        /* cast to non-const; see note in struct definition */
        rexRegexSLRParser_destroy((rexRegexSLRParser*) self->regex_slr_parser);
    }
    return;
}

void lexLexer_destroy(lexLexer* self) {
    if (self == NULL) {
        return;
    }
    lexLexer_destroy_(self);
    FREE(self);
    return;
}

static lexLexer* lexLexer_create_from_spec_(lexLexer* self, const char *spec,
                                            const rexRegexSLRParser* regex_slr_parser) {
    const char str_empty[] = "<empty>";

    if (regex_slr_parser == NULL) {
        self->is_regex_slr_parser_borrowed = false;
        self->regex_slr_parser = rexRegexSLRParser_create();
    } else {
        self->is_regex_slr_parser_borrowed = true;
        self->regex_slr_parser = regex_slr_parser;
    }
    self->num_of_tokens = 0;
    self->num_of_nonignored_tokens = 0;
    self->token_names = NULL;
    self->compiled_regexes = NULL;

    gsStack token_names, compiled_regexes;
    gsStack_create_(&token_names, sizeof(const char*));
    gsStack_create_(&compiled_regexes, sizeof(rexCompiledRegex*));

    GS_APPEND(&token_names, str_create_copy(str_empty), char*);
    GS_APPEND(&compiled_regexes, NULL, rexCompiledRegex*);

    boolean got_to_ignored_tokens = false;
    const char* cursor = spec;
    const char* cursor_next;
    cursor = str_while_not_visible(cursor);
    for (;;) {
        cursor_next = str_while_visible(cursor);
        if (cursor_next == cursor) {
            break;
        } else if (
            cursor_next == cursor + 2 &&
            *cursor == '@' &&
            *(cursor+1) == '!'
            ) {
            got_to_ignored_tokens = true;
            if (*cursor_next != ' ') {
                goto error_label;
            }
            cursor = cursor_next + 1;
            cursor_next = str_while_not_newline(cursor);
            rexCompiledRegex* compiled_regex =
                rexCompiledRegex_create_from_regex(self->regex_slr_parser,
						   cursor, cursor_next);
            if (compiled_regex == NULL) {
                goto error_label;
            }
            GS_APPEND(&compiled_regexes, compiled_regex, rexCompiledRegex*);
            cursor = str_while_not_visible(cursor_next);
        } else if (
            cursor_next == cursor + 2 &&
            *cursor == '@' &&
            *(cursor+1) == '@'
            ) {
            if (got_to_ignored_tokens == true) {
                goto error_label;
            }
            for (;;) {
                cursor = str_while_not_visible_except_newline(cursor_next);
                cursor_next = str_while_visible(cursor);
                if (cursor_next == cursor) {
                    cursor = str_while_not_visible(cursor_next);
                    break;
                }
                GS_APPEND(&token_names,
                          str_create_copy_from_to(cursor, cursor_next),
                          char*);
                rexCompiledRegex* compiled_regex =
                    rexCompiledRegex_create_from_raw_str(self->regex_slr_parser,
							 cursor, cursor_next);
                if (compiled_regex == NULL) {
                    goto error_label;
                }
                GS_APPEND(&compiled_regexes, compiled_regex, rexCompiledRegex*);
            }
        } else {
            if (got_to_ignored_tokens == true) {
                goto error_label;
            }
            GS_APPEND(&token_names,
		      str_create_copy_from_to(cursor, cursor_next),
		      char*);
            if (*cursor_next != ' ') {
                goto error_label;
            }
            cursor = cursor_next + 1;
            cursor_next = str_while_not_newline(cursor);
            rexCompiledRegex* compiled_regex =
                rexCompiledRegex_create_from_regex(self->regex_slr_parser,
						   cursor, cursor_next);
            if (compiled_regex == NULL) {
                goto error_label;
            }
            GS_APPEND(&compiled_regexes, compiled_regex, rexCompiledRegex*);
            cursor = str_while_not_visible(cursor_next);
        }
    }

    self->num_of_tokens = gsStack_length(&compiled_regexes);
    self->compiled_regexes =
	rexCompiledRegexList_create_from_compiled_regex_list__(
	    gsStack_length(&compiled_regexes),
	    gsStack_0(&compiled_regexes));
    self->num_of_nonignored_tokens = gsStack_length(&token_names);
    self->token_names = gsStack_0(&token_names);
    return self;

    error_label:;

    const unsigned token_names_length = gsStack_length(&token_names);
    for (unsigned i = 0; i < token_names_length; ++i) {
        FREE(* (char**) gsStack_element(&token_names, i));
    }
    gsStack_destroy_(&token_names);
    const unsigned compiled_regexes_length = gsStack_length(&compiled_regexes);
    for (unsigned i = 0; i < compiled_regexes_length; ++i) {
        rexCompiledRegex_destroy(
	    * (rexCompiledRegex**) gsStack_element(&compiled_regexes, i));
    }
    gsStack_destroy_(&compiled_regexes);
    lexLexer_destroy_(self);
    return NULL;
}

lexLexer* lexLexer_create_from_spec(const char *spec,
                                    const rexRegexSLRParser* regex_slr_parser) {
    lexLexer* self = MALLOC(sizeof(lexLexer));
    lexLexer* result = lexLexer_create_from_spec_(self, spec, regex_slr_parser);
    if (result == NULL) {
        FREE(self);
        return NULL;
    }
    return self;
}

unsigned* lexLexer_process(const lexLexer* self, const char* str,
                           lexStrToValFn str_to_val_fn, void* extra,
                           unsigned** out_vals,
                           const char** out_end_pos) {
    gsStack tokens, vals;
    gsStack_create_(&tokens, sizeof(unsigned));
    if (out_vals != NULL) {
        gsStack_create_(&vals, sizeof(unsigned));
    }
    while (*str != 0) {
        const char* end_pos;
        const unsigned winner =
            rexCompiledRegexList_race(self->compiled_regexes, str, &end_pos);
        if (winner == self->num_of_tokens) {
            break;
        }
        if (winner < self->num_of_nonignored_tokens) {
            GS_APPEND(&tokens, winner, unsigned);
            if (out_vals != NULL) {
                GS_APPEND(&vals, str_to_val_fn(winner, str, end_pos, extra),
                          unsigned);
            }
        }
        str = end_pos;
    }
    GS_APPEND(&tokens, 0, unsigned);
    if (out_vals != NULL) {
        *out_vals = gsStack_0(&vals);
    }
    *out_end_pos = str;
    return gsStack_0(&tokens);
}

/*-------------------------*//*-------------------------*/
/*-------------------------*//*-------------------------*/
/*-------------------------*//*-------------------------*/

#ifdef TESTING_PRINTS

#include <stdio.h>

void lexLexer_print(const lexLexer* self) {
    printf("----\n");
    printf("The lexer has:\n");
    printf("  * %u actual tokens:\n", self->num_of_nonignored_tokens);
    for (unsigned i = 0; i < self->num_of_nonignored_tokens; ++i) {
        printf("    (%u) %s\n", i, self->token_names[i]);
    }
    printf("  * %u ignored tokens.\n",
           self->num_of_tokens - self->num_of_nonignored_tokens);
    printf("----\n");
    return;
}

void lexLexer_print_process_result(const lexLexer* self, const unsigned* tokens,
                                   const unsigned* vals) {
    printf("----\n");
    unsigned length = 0;
    for (const unsigned* t = tokens; *t != 0; ++t) {
        ++length;
    }
    if (vals != NULL) {
        printf("The produced (token_value, pair) sequence, of length %u, is:\n",
               length);
    } else {
        printf("The produced token sequence, of length %u, is:\n", length);
    }
    for (unsigned i = 0; i < length; ++i) {
        const unsigned token = tokens[i];
        if (token <= self->num_of_nonignored_tokens) {
            if (vals != NULL) {
                printf("%s_%u ", self->token_names[token], vals[i]);
            } else {
                printf("%s ", self->token_names[token]);
            }
        } else {
            if (vals != NULL) {
                printf("<ignore>_%u ", vals[i]);
            } else {
                printf("<ignore> ");
            }
        }
    }
    printf("\n");
    printf("----\n");
}

#endif /* TESTING_PRINTS */
