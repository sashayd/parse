#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "standard.h"
#include "ma.h"
#include "str.h"
#include "gs.h"
#include "./ss.h"
#include "./fa.h"

/*-------------------------*/

typedef struct prProduction {
    unsigned head;
    unsigned body_length;
    unsigned* body;
}                          prProduction;

struct prGrammar {
    unsigned num_of_productions;
    unsigned max_production_body_length;
    unsigned num_of_tokens;
    unsigned num_of_terminals;
    char** token_names;
    char** production_names;
    prProduction* productions;
};

struct prSLRParser {
    faDfaBt self_as_faDfaBt;
};

/*-------------------------*/

static const prProduction* prGrammar_production(const prGrammar* self,
                                                unsigned id) {
    return self->productions + id;
}

unsigned prGrammar_token_by_name(const prGrammar* self, const char *name) {
    return str_equal_to_one_of(name, self->num_of_tokens,
			       (const char**) self->token_names);
}

unsigned prGrammar_production_by_name(const prGrammar* self, const char *name) {
    return str_equal_to_one_of(name, self->num_of_productions,
                               (const char**) self->production_names);
}

void prGrammar_destroy_(prGrammar* self) {
    if (self == NULL) {
        return;
    }
    for (unsigned i = 0; i < self->num_of_tokens; ++i) {
        FREE(self->token_names[i]);
    }
    FREE(self->token_names);
    for (unsigned i = 0; i < self->num_of_productions; ++i) {
        FREE(self->production_names[i]);
    }
    FREE(self->production_names);
    for (unsigned i = 0; i < self->num_of_productions; ++i) {
        FREE(self->productions[i].body);
    }
    FREE(self->productions);
    return;
}

void prGrammar_destroy(prGrammar* self) {
    if (self == NULL) {
        return;
    }
    prGrammar_destroy_(self);
    FREE(self);
    return;
}

static prGrammar* prGrammar_create_from_spec_given_terminal_names_(
    prGrammar* self,
    const char* spec,
    unsigned terminal_names_list_length,
    const char* const* terminal_names_list,
    char* out_diagnostic) {

    const char str_nonterminals[] = "@@nonterminals";
    const char str_productions[] = "@@productions";
    const char str_formal[] = "<formal>";
    const char str_to[] = "->";

    self->num_of_productions = 0;
    self->max_production_body_length = 0;
    self->num_of_tokens = 0;
    self->num_of_terminals = terminal_names_list_length;
    self->token_names = NULL;
    self->productions = NULL;
    self->production_names = NULL;

    gsStack token_names, production_names, productions, body;
    gsStack_create_(&token_names, sizeof(char*));
    gsStack_create_(&production_names, sizeof(char*));
    gsStack_create_(&productions, sizeof(prProduction));
    gsStack_create_(&body, sizeof(unsigned));

    /* read terminals names */

    for (unsigned i = 0; i < self->num_of_terminals; ++i) {
        GS_APPEND (&token_names, str_create_copy(terminal_names_list[i]),
                   char*);
    }

    /* skip to non-terminals names */

    const char* cursor = spec;
    const char* cursor_next;
    for(;;) {
        cursor_next = str_starts_with(cursor, str_nonterminals);
        if (cursor_next != NULL) {
            break;
        }
        cursor_next = str_while_visible(cursor);
        if (cursor_next == cursor) {
            goto error_label;
        }
        cursor = str_while_not_visible(cursor_next);
    }
    self->num_of_terminals = gsStack_length(&token_names);
    cursor = str_while_not_visible(cursor_next);

    /* read non-terminals names */

    for(;;) {
        cursor_next = str_starts_with(cursor, str_productions);
        if (cursor_next != NULL) {
            break;
        }
        cursor_next = str_while_visible(cursor);
        if (cursor_next == cursor) {
            goto error_label;
        }
        GS_APPEND(&token_names, str_create_copy_from_to(cursor, cursor_next),
                  char*);
        cursor = str_while_not_visible(cursor_next);
    }
    cursor = str_while_not_visible(cursor_next);

    /* add formal non-terminal */
    GS_APPEND(&token_names, str_create_copy(str_formal), char*);

    /* add formal production */
    gsStack_pre_append_(&productions);
    prProduction* const first_production = gsStack_last(&productions);
    first_production->head = gsStack_length(&token_names) - 1;
    first_production->body_length = 1;
    GS_APPEND(&body, gsStack_length(&token_names) - 2, unsigned);
    first_production->body = gsStack_0(&body);
    gsStack_create_(&body, sizeof(unsigned));
    GS_APPEND(&production_names, str_create_copy(str_formal), char*);

    /* read productions */

    const char* query_result;
    while (*cursor != 0) {
        /* just since the str.h function uses size_t and I use unsigned... */
        size_t temp;

        gsStack_pre_append_(&productions);
        prProduction* production = gsStack_last(&productions);
        production->body = NULL;

        GS_APPEND(&production_names, NULL, char*);

        if (*cursor == '@') {
            ++cursor;
            if (*cursor == '@') {
                goto error_label;
            }
            cursor = str_while_not_visible(cursor);
            cursor_next = str_while_visible(cursor);
            if (cursor_next == cursor) {
                goto error_label;
            }
            * (char**) gsStack_last(&production_names) =
                str_create_copy_from_to(cursor, cursor_next);
            cursor = str_while_not_visible(cursor_next);
        }

        query_result = str_starts_with_one_of(cursor,
                                              gsStack_length(&token_names),
                                              gsStack_0(&token_names),
                                              &temp);
        if (query_result == NULL) {
	    if (out_diagnostic != NULL) {
		char* what_was_not_found =
		    str_create_copy_from_to(cursor, str_while_visible(cursor));
		sprintf(out_diagnostic,
			"Error parsing grammar file: did not find the "
			"token %s encountered in a production.\n",
			what_was_not_found);
		FREE(what_was_not_found);
	    }
            goto error_label;
        } else {
            cursor = query_result;
        }
        production->head = temp;
        cursor = str_while_not_visible(cursor);
        cursor = str_starts_with(cursor, str_to);
        if (cursor == NULL) {
            goto error_label;
        }
        cursor = str_while_not_visible(cursor);
        while (*cursor != 0 && *cursor != '\n') {
            gsStack_pre_append_(&body);
            query_result = str_starts_with_one_of(cursor,
                                                  gsStack_length(&token_names),
                                                  gsStack_0(&token_names),
                                                  &temp);
            if (query_result == NULL) {
                if (out_diagnostic != NULL) {
		    char* what_was_not_found =
			str_create_copy_from_to(cursor, str_while_visible(cursor));
		    sprintf(out_diagnostic,
			    "Error parsing grammar file: did not find the "
			    "token %s encountered in a production.\n",
			    what_was_not_found);
		    FREE(what_was_not_found);
		}
                goto error_label;
            } else {
                cursor = query_result;
            }
            * (unsigned*) gsStack_last(&body) = temp;
            cursor = str_while_not_visible_except_newline(cursor);
        }
        cursor = str_while_not_visible(cursor);
        production->body_length = gsStack_length(&body);
        production->body = gsStack_0(&body);
        gsStack_create_(&body, sizeof(unsigned));
    }

    /* finalize */

    self->num_of_tokens = gsStack_length(&token_names);
    self->token_names = gsStack_0(&token_names);
    self->num_of_productions = gsStack_length(&productions);
    self->productions = gsStack_0(&productions);
    self->production_names = gsStack_0(&production_names);
    return self;

    error_label:;
    gsStack_destroy_(&body);
    unsigned length = gsStack_length(&productions);
    prProduction* production = gsStack_0(&productions);
    for (unsigned i = 0; i < length; ++i) {
        FREE(production->body);
        ++production;
    }
    gsStack_destroy_(&productions);
    length = gsStack_length(&token_names);
    char** string = gsStack_0(&token_names);
    for (unsigned i = 0; i < length; ++i) {
        FREE(*string);
        ++string;
    }
    gsStack_destroy_(&token_names);
    length = gsStack_length(&production_names);
    string = gsStack_0(&production_names);
    for (unsigned i = 0; i < length; ++i) {
        FREE(*string);
        ++string;
    }
    gsStack_destroy_(&production_names);
    prGrammar_destroy_(self);
    return NULL;
}

prGrammar* prGrammar_create_from_spec_given_terminal_names(
    const char* spec,
    unsigned terminal_names_list_length,
    const char* const* terminal_names_list,
    char* out_diagnostic) {
    prGrammar* self = MALLOC(sizeof(prGrammar));
    prGrammar* result =
        prGrammar_create_from_spec_given_terminal_names_(
            self, spec, terminal_names_list_length,
	    terminal_names_list, out_diagnostic);
    if (result == NULL) {
        FREE(self);
        return NULL;
    }
    return self;
}

static prGrammar* prGrammar_create_from_spec_(prGrammar* self, const char* spec,
					      char* out_diagnostic) {
    const char str_empty[] = "<empty>";
    const char str_nonterminals[] = "@@nonterminals";
    const char str_productions[] = "@@productions";
    const char str_formal[] = "<formal>";
    const char str_to[] = "->";

    self->num_of_productions = 0;
    self->max_production_body_length = 0;
    self->num_of_tokens = 0;
    self->num_of_terminals = 0;
    self->token_names = NULL;
    self->productions = NULL;
    self->production_names = NULL;

    gsStack token_names, production_names, productions, body;
    gsStack_create_(&token_names, sizeof(char*));
    gsStack_create_(&production_names, sizeof(char*));
    gsStack_create_(&productions, sizeof(prProduction));
    gsStack_create_(&body, sizeof(unsigned));

    GS_APPEND(&token_names, str_create_copy(str_empty), char*);

    /* read terminals names */

    const char* cursor = spec;
    const char* cursor_next;
    for (;;) {
        cursor_next = str_starts_with(cursor, str_nonterminals);
        if (cursor_next != NULL) {
            break;
        }
        cursor_next = str_while_visible(cursor);
        if (cursor_next == cursor) {
            goto error_label;
        }
        GS_APPEND(&token_names, str_create_copy_from_to(cursor, cursor_next),
                  char*);
        cursor = str_while_not_visible(cursor_next);
    }
    self->num_of_terminals = gsStack_length(&token_names);
    cursor = str_while_not_visible(cursor_next);

    /* read non-terminals names */

    for(;;) {
        cursor_next = str_starts_with(cursor, str_productions);
        if (cursor_next != NULL) {
            break;
        }
        cursor_next = str_while_visible(cursor);
        if (cursor_next == cursor) {
            goto error_label;
        }
        GS_APPEND(&token_names, str_create_copy_from_to(cursor, cursor_next),
                  char*);
        cursor = str_while_not_visible(cursor_next);
    }
    cursor = str_while_not_visible(cursor_next);

    /* add formal non-terminal */
    GS_APPEND(&token_names, str_create_copy(str_formal), char*);

    /* add formal production */
    gsStack_pre_append_(&productions);
    prProduction* const first_production = gsStack_last(&productions);
    first_production->head = gsStack_length(&token_names) - 1;
    first_production->body_length = 1;
    GS_APPEND(&body, gsStack_length(&token_names) - 2, unsigned);
    first_production->body = gsStack_0(&body);
    gsStack_create_(&body, sizeof(unsigned));
    GS_APPEND(&production_names, str_create_copy(str_formal), char*);

    /* read productions */

    const char* query_result;
    while (*cursor != 0) {
        /* just since the str.h function uses size_t and I use unsigned... */
        size_t temp;

        gsStack_pre_append_(&productions);
        prProduction* production = gsStack_last(&productions);
        production->body = NULL;

        GS_APPEND(&production_names, NULL, char*);

        if (*cursor == '@') {
            ++cursor;
            if (*cursor == '@') {
                goto error_label;
            }
            cursor = str_while_not_visible(cursor);
            cursor_next = str_while_visible(cursor);
            if (cursor_next == cursor) {
                goto error_label;
            }
            * (char**) gsStack_last(&production_names) =
                str_create_copy_from_to(cursor, cursor_next);
            cursor = str_while_not_visible(cursor_next);
        }

        query_result = str_starts_with_one_of(cursor,
                                              gsStack_length(&token_names),
                                              gsStack_0(&token_names),
                                              &temp);
        if (query_result == NULL) {
            if (out_diagnostic != NULL) {
		char* what_was_not_found =
		    str_create_copy_from_to(cursor, str_while_visible(cursor));
		sprintf(out_diagnostic,
			"Error parsing grammar file: did not find the "
			"token %s encountered in a production.\n",
			what_was_not_found);
		FREE(what_was_not_found);
	    }
            goto error_label;
        } else {
            cursor = query_result;
        }
        production->head = temp;
        cursor = str_while_not_visible(cursor);
        cursor = str_starts_with(cursor, str_to);
        if (cursor == NULL) {
            goto error_label;
        }
        cursor = str_while_not_visible(cursor);
        while (*cursor != 0 && *cursor != '\n') {
            gsStack_pre_append_(&body);
            query_result = str_starts_with_one_of(cursor,
                                                  gsStack_length(&token_names),
                                                  gsStack_0(&token_names),
                                                  &temp);
            if (query_result == NULL) {
                if (out_diagnostic != NULL) {
		    char* what_was_not_found =
			str_create_copy_from_to(cursor, str_while_visible(cursor));
		    sprintf(out_diagnostic,
			    "Error parsing grammar file: did not find the "
			    "token %s encountered in a production.\n",
			    what_was_not_found);
		    FREE(what_was_not_found);
		}
                goto error_label;
            } else {
                cursor = query_result;
            }
            * (unsigned*) gsStack_last(&body) = temp;
            cursor = str_while_not_visible_except_newline(cursor);
        }
        cursor = str_while_not_visible(cursor);
        production->body_length = gsStack_length(&body);
        production->body = gsStack_0(&body);
        gsStack_create_(&body, sizeof(unsigned));
    }

    /* finalize */

    self->num_of_tokens = gsStack_length(&token_names);
    self->token_names = gsStack_0(&token_names);
    self->num_of_productions = gsStack_length(&productions);
    self->productions = gsStack_0(&productions);
    self->production_names = gsStack_0(&production_names);
    return self;

    error_label:;
    gsStack_destroy_(&body);
    unsigned length = gsStack_length(&productions);
    prProduction* production = gsStack_0(&productions);
    for (unsigned i = 0; i < length; ++i) {
        FREE(production->body);
        ++production;
    }
    gsStack_destroy_(&productions);
    length = gsStack_length(&token_names);
    char** string = gsStack_0(&token_names);
    for (unsigned i = 0; i < length; ++i) {
        FREE(*string);
        ++string;
    }
    gsStack_destroy_(&token_names);
    length = gsStack_length(&production_names);
    string = gsStack_0(&production_names);
    for (unsigned i = 0; i < length; ++i) {
        FREE(*string);
        ++string;
    }
    gsStack_destroy_(&production_names);
    prGrammar_destroy_(self);
    return NULL;
}

prGrammar* prGrammar_create_from_spec(const char* spec, char* out_diagnostic) {
    prGrammar* self = MALLOC(sizeof(prGrammar));
    prGrammar* result = prGrammar_create_from_spec_(self, spec, out_diagnostic);
    if (result == NULL) {
        FREE(self);
        return NULL;
    }
    return self;
}

void prSLRParser_destroy_(prSLRParser* self) {
    faDfaBt_destroy_((faDfaBt*) self);
}

void prSLRParser_destroy(prSLRParser* self) {
    faDfaBt_destroy((faDfaBt*) self);
}

static faNfa* _prGrammar_construct_nfa(const prGrammar* self) {
    unsigned length = 0;
    for (unsigned i = 0; i < self->num_of_productions; ++i) {
        const prProduction* const production = prGrammar_production(self, i);
        length += production->body_length + 1;
    }
    faNfa* nfa = faNfa_create(length, 0, 1);
    unsigned production_start_state = 0;
    for (unsigned i = 0; i < self->num_of_productions; ++i) {
        const prProduction* const production = prGrammar_production(self, i);
        for (unsigned j = 0; j < production->body_length; ++j) {
            const unsigned token = production->body[j];
            const unsigned source = production_start_state + j;
            faNfa_add_edge_(nfa, source, source+1, token);
            unsigned production1_start_state = 0;
            for (unsigned k = 0; k < self->num_of_productions; ++k) {
                const prProduction* const production1 =
                    prGrammar_production(self, k);
                if (production1->head == token) {
                    const unsigned target = production1_start_state;
                    faNfa_add_edge_(nfa, source, target, 0);
                }
                production1_start_state += production1->body_length + 1;
            }
        }
        production_start_state += production->body_length + 1;
    }
    return nfa;
}

typedef struct _prSLRHelper {
    unsigned num_of_tokens;
    ssSubset appearing_tokens;
    ssSubset* first;
    ssSubset* follow;
    unsigned* state_to_production;
    unsigned* state_to_production_position;
    boolean is_slr;
} _prSLRHelper;

static void _prSLRHelper_destroy_(_prSLRHelper* self) {
    if (self == NULL) {
        return;
    }
    ssSubset_destroy_(&self->appearing_tokens);
    for (unsigned ii = 0; ii < self->num_of_tokens; ++ii) {
        const unsigned i = self->num_of_tokens - 1 - ii;
        ssSubset_destroy_(self->first+i);
        ssSubset_destroy_(self->follow+i);
    }
    FREE(self->first);
    FREE(self->follow);
    FREE(self->state_to_production);
    FREE(self->state_to_production_position);
    return;
}

static void _prSLRHelper_fill_(_prSLRHelper* self, const prGrammar* grammar) {
    boolean got_something_new;
    boolean query_result;

    /* just so that we know how much to destruct... */
    self->num_of_tokens = grammar->num_of_tokens;

    /* compute necessary */
    ssSubset_create_(&self->appearing_tokens, sizeof(grammar->num_of_tokens));
    ssSubset_add_(&self->appearing_tokens, grammar->num_of_tokens - 2);
    do {
        got_something_new = false;
        for (unsigned j = 0; j < grammar->num_of_productions; ++j) {
            const prProduction* const production =
                prGrammar_production(grammar, j);
            query_result = ssSubset_is_in(&self->appearing_tokens,
                                          production->head);
            if (query_result == true) {
                for (unsigned i = 0; i < production->body_length; ++i) {
                    query_result = ssSubset_add_(&self->appearing_tokens,
                                                 production->body[i]);
                    if (query_result == false) {
                        got_something_new = true;
                    }
                }
            }
        }
    } while (got_something_new == true);

    /* compute first */
    self->first = MALLOC(grammar->num_of_tokens * sizeof(*self->first));
    for (unsigned i = 0; i < grammar->num_of_tokens; ++i) {
        ssSubset_create_(self->first + i, grammar->num_of_terminals);
    }
    for (unsigned i = 0; i < grammar->num_of_terminals; ++i) {
        ssSubset_add_(self->first + i, i);
    }
    do {
        got_something_new = false;
        for (unsigned j = 0; j < grammar->num_of_productions; ++j) {
            const prProduction* const production =
                prGrammar_production(grammar, j);
            ssSubset* const head_subset = self->first + production->head;
            boolean all_can_derive_empty = true;
            for (unsigned i = 0; i < production->body_length; ++i) {
                const ssSubset* const subset =
                    self->first + production->body[i];
                const unsigned length = ssSubset_length(subset);
                for (unsigned k = 0; k < length; ++k) {
                    const unsigned element = ssSubset_element(subset, k);
                    if (ssSubset_add_(head_subset, element) == false) {
                        got_something_new = true;
                    }
                }
                if (ssSubset_is_in(subset, 0) == false) {
                    all_can_derive_empty = false;
                    break;
                }
            }
            if (all_can_derive_empty == true) {
                if (ssSubset_add_(head_subset, 0) == false) {
                    got_something_new = true;
                }
            }
        }
    } while (got_something_new == true);

    /* compute follow */
    self->follow = MALLOC(grammar->num_of_tokens * sizeof(*self->follow));
    for (unsigned i = 0; i < grammar->num_of_tokens; ++i) {
        ssSubset_create_(self->follow + i, grammar->num_of_terminals);
    }
    ssSubset_add_(self->follow + (grammar->num_of_tokens - 2), 0);
    do {
        got_something_new = false;
        for (unsigned j = 0; j < grammar->num_of_productions; ++j) {
            const prProduction* const production =
                prGrammar_production(grammar, j);
            query_result = ssSubset_is_in(&self->appearing_tokens,
                                          production->head);
            if (query_result == false) {
                continue;
            }
            if (production->body_length == 0) {
                continue;
            }
            boolean all_following_can_derive_empty = true;
            for (unsigned ii = 0; ii < production->body_length; ++ii) {
                const unsigned i = production->body_length - 1 - ii;
                ssSubset* const subset = self->follow + production->body[i];
                if (all_following_can_derive_empty == true) {
                    const ssSubset* const head_subset =
                        self->follow + production->head;
                    const unsigned length = ssSubset_length(head_subset);
                    for (unsigned k = 0; k < length; ++k) {
                        const unsigned element =
                            ssSubset_element(head_subset, k);
                        if (ssSubset_add_(subset, element) == false) {
                            got_something_new = true;
                        }
                    }
                }
                for (unsigned l = i+1; l < production->body_length; ++l) {
                    const ssSubset* const next_first_subset =
                        self->first + production->body[l];
                    const unsigned length = ssSubset_length(next_first_subset);
                    for (unsigned k = 0; k < length; ++k) {
                        const unsigned element =
                            ssSubset_element(next_first_subset, k);
                        if (element == 0) {
                            continue;
                        }
                        if (ssSubset_add_(subset, element) == false) {
                            got_something_new = true;
                        }
                    }
                    query_result =
                        ssSubset_is_in(self->first + production->body[l], 0);
                    if (query_result == false) {
                        break;
                    }
                }
                query_result =
                    ssSubset_is_in(self->first + production->body[i], 0);
                if (query_result == false) {
                    all_following_can_derive_empty = false;
                }
            }
        }
    } while (got_something_new == true);

    /* compute state_to_production and state_to_production_position*/
    gsStack state_to_production, state_to_production_position;
    gsStack_create_(&state_to_production, sizeof(unsigned));
    gsStack_create_(&state_to_production_position, sizeof(unsigned));
    for (unsigned i = 0; i < grammar->num_of_productions; ++i) {
        const prProduction* const production = prGrammar_production(grammar, i);
        for (unsigned j = 0; j < production->body_length + 1; ++j) {
            GS_APPEND(&state_to_production, i, unsigned);
            GS_APPEND(&state_to_production_position, j, unsigned);
        }
    }
    self->state_to_production = gsStack_0(&state_to_production);
    self->state_to_production_position =
        gsStack_0(&state_to_production_position);

    return;
}

prSLRParser* prSLRParser_create_from_grammar(const prGrammar* grammar,
					     char* out_diagnostic) {
    faDfaBt* self = MALLOC(sizeof(*self));

    _prSLRHelper slr_helper;
    _prSLRHelper_fill_(&slr_helper, grammar);

    faNfa* const nfa = _prGrammar_construct_nfa(grammar);
    if (nfa == NULL) {
        _prSLRHelper_destroy_(&slr_helper);
        FREE(self);
        return NULL;
    }

    gsStack subsets;
    faDfa_create_nfaec_with_subsets_(&self->dfa, &subsets, nfa,
                                     unsignedMaybe_from_unsigned(grammar->num_of_tokens));
    faNfa_destroy(nfa);

    self->bt_list_length = 0;
    self->bt_list = NULL;
    self->bt_table = CALLOC(self->dfa.num_of_tokens * self->dfa.num_of_states,
                            sizeof(*self->bt_table));

    boolean is_slr = true;

    ssSubset completed_productions;
    ssSubset terminals_next;
    ssSubset_create_(&completed_productions, grammar->num_of_productions);
    ssSubset_create_(&terminals_next, grammar->num_of_terminals);
    for (unsigned i = 0; i < self->dfa.num_of_states; ++i) {
        ssSubset_make_empty_(&completed_productions);
        ssSubset_make_empty_(&terminals_next);
        const ssSubset* const subset = gsStack_element(&subsets, i);
        const unsigned subset_length = ssSubset_length(subset);
        for (unsigned j = 0; j < subset_length; ++j) {
            const unsigned item = ssSubset_element(subset, j);
            const unsigned production_id = slr_helper.state_to_production[item];
            const prProduction* const production =
                prGrammar_production(grammar, production_id);
            const unsigned position =
                slr_helper.state_to_production_position[item];
            if (position == production->body_length) {
                ssSubset_add_(&completed_productions, production_id);
            } else if (production->body[position] < grammar->num_of_terminals) {
                ssSubset_add_(&terminals_next, production->body[position]);
            }
        }
        for (unsigned j = 0; j < grammar->num_of_terminals; ++j) {
            const boolean is_in_terminals_next =
                ssSubset_is_in(&terminals_next, j);
            if (self->dfa.sinks[i] == true && is_in_terminals_next == true) {
                is_slr = false;
		if (out_diagnostic != NULL) {
		    sprintf(out_diagnostic,
			    "The given grammar is not SLR!\n"
			    "Encountered problem with:\n"
			    "  * terminal %s\n"
			    "  * the state containing the sink "
			    "completed production\n",
			    grammar->token_names[j]);
		}
                goto exit_all_fors;
            }
            boolean found_completed_production = false;
            const unsigned length = ssSubset_length(&completed_productions);
            for (unsigned k = 0; k < length; ++k) {
                const unsigned production_id =
                    ssSubset_element(&completed_productions, k);
                const prProduction* const production =
                    prGrammar_production(grammar, production_id);
                if (ssSubset_is_in(slr_helper.follow + production->head, j)
                    == true) {
                    if (self->dfa.sinks[i] == true) {
                        is_slr = false;
			if (out_diagnostic != NULL) {
			    sprintf(out_diagnostic,
				    "The given grammar is not SLR!\n"
				    "Encountered problem with:"
				    "\n  * completed production %s -> ",
				    grammar->token_names[production->head]);
			    out_diagnostic = str_goto_end(out_diagnostic);
			    for (unsigned l = 0; l < production->body_length; ++l) {
				sprintf(out_diagnostic,
					"%s ",
					grammar->token_names[production->body[l]]);
				out_diagnostic = str_goto_end(out_diagnostic);
			    }
			    sprintf(out_diagnostic,
				    "\n    which lies in the state containing "
				    "the sink completed production but can be "
				    "followed by terminal %s\n",
				    grammar->token_names[j]);
			}
                        goto exit_all_fors;
                    }
                    if (found_completed_production == true ||
                        is_in_terminals_next == true) {
                        is_slr = false;
			if (out_diagnostic != NULL) {
			    sprintf(out_diagnostic,
				    "The given grammar is not SLR!\n"
				    "Encountered problem with:\n"
				    "  * terminal %s\n"
				    "  * completed production %s -> ",
				    grammar->token_names[j],
				    grammar->token_names[production->head]);
			    out_diagnostic = str_goto_end(out_diagnostic);
			    for (unsigned l = 0; l < production->body_length; ++l) {
				sprintf(out_diagnostic,
					"%s ",
					grammar->token_names[production->body[l]]);
				out_diagnostic = str_goto_end(out_diagnostic);
			    }
			    sprintf(out_diagnostic, "\n");
			}
                        goto exit_all_fors;
                    }
                    found_completed_production = true;
                    self->bt_table[i * self->dfa.num_of_tokens + j] =
                        production_id + 1;
                }
            }
        }
    }
    exit_all_fors:;
    ssSubset_destroy_(&terminals_next);
    ssSubset_destroy_(&completed_productions);
    if (gsStack_length(&subsets) != 0) {
	ssSubset* const s0 = gsStack_0(&subsets);
	for (ssSubset* s = gsStack_end(&subsets); s > s0;) {
	    ssSubset_destroy_(--s);
	}
    }
    gsStack_destroy_(&subsets);
    _prSLRHelper_destroy_(&slr_helper);

    if (is_slr == false) {
	faDfaBt_destroy(self);
        return NULL;
    }

    self->bt_list_length = grammar->num_of_productions;
    self->bt_list = MALLOC(self->bt_list_length * sizeof(*self->bt_list));
    for (unsigned i = 0; i < grammar->num_of_productions; ++i) {
        const prProduction* const production =
	    prGrammar_production(grammar, i);
        self->bt_list[i].num_of_steps = production->body_length;
        self->bt_list[i].replacing_token = production->head;
    }

    return (prSLRParser*) self;
}

unsigned* prSLRParser_parse(const prSLRParser* self, const unsigned* tokens,
                            const unsigned** out_end_pos) {
    return faDfaBt_parse((faDfaBt*) self, tokens, out_end_pos);
}

void prSLRParser_synthesize(const prSLRParser* self, const unsigned* items,
                            const unsigned* vals, unsigned element_size,
                            prTerminalSynthFn terminal_synth_fn,
                            prProductionSynthFn production_synth_fn,
                            void* extra, void* result) {
    faDfaBt_synthesize((faDfaBt*) self, items, vals, element_size,
		       terminal_synth_fn, production_synth_fn, extra, result);
    return;
}

/*-------------------------*//*-------------------------*/
/*-------------------------*//*-------------------------*/
/*-------------------------*//*-------------------------*/

#ifdef TESTING_PRINTS

#include <stdio.h>

void prGrammar_print(const prGrammar* self) {
    printf("----\n");
    printf("The grammar has:\n");
    printf("  * %u terminals:\n    ", self->num_of_terminals);
    for (unsigned i = 0; i < self->num_of_terminals; ++i) {
        printf("%s ", self->token_names[i]);
    }
    printf("\n");
    printf(
        "  * %u non-terminals:\n    ",
        self->num_of_tokens - self->num_of_terminals
        );
    for (unsigned i = self->num_of_terminals; i < self->num_of_tokens; ++i) {
        printf("%s ", self->token_names[i]);
    }
    printf("\n");
    printf("  * %u productions:\n", self->num_of_productions);
    for (unsigned i = 0; i < self->num_of_productions; ++i) {
        if (self->production_names[i] != NULL) {
            printf("    - production %u (%s): ", i, self->production_names[i]);
        } else {
            printf("    - production %u: ", i);
        }
        const prProduction* const production = prGrammar_production(self, i);
        printf("%s -> ", self->token_names[production->head]);
        for (unsigned j = 0; j < production->body_length; ++j) {
            printf("%s ", self->token_names[production->body[j]]);
        }
        printf("\n");
    }
    printf("----\n");
    return;
}

#endif /* TESTING_PRINTS */
