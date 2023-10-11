#include "./fa.h"

#include <stdlib.h>
#include <string.h>

#include "standard.h"
#include "ma.h"
#include "gs.h"
#include "./ss.h"

/*-------------------------*/
/* structs                 */
/*-------------------------*/

struct faNfa {
    faNfaEdgeList edge_lists;
    /* I use the terminology "cosink" instead of "source",
       because I use "source" for edge source */
    unsigned cosink;
    unsigned sink;
};

/*-------------------------*/

unsigned faNfa_length(const faNfa* self) {
    return gsStack_length(&self->edge_lists);
}

unsigned faNfa_cosink(const faNfa* self) {
    return self->cosink;
}

unsigned faNfa_sink(const faNfa* self) {
    return self->sink;
}

faNfaEdgeList* faNfa_edge_list(const faNfa* self, unsigned source) {
    return (faNfaEdgeList*) gsStack_element(&self->edge_lists, source);
}

unsigned faNfaEdgeList_length(const faNfaEdgeList* self) {
    return gsStack_length(self);
}

faNfaEdge* faNfaEdgeList_edge(const faNfaEdgeList* self, unsigned index) {
    return (faNfaEdge*) gsStack_element(self, index);
}

void faNfa_destroy_(faNfa* self) {
    if (self == NULL) {
        return;
    }
    gsStack_of_gsStacks_destroy_(&self->edge_lists);
    return;
}

void faNfa_destroy(faNfa* self) {
    if (self == NULL) {
        return;
    }
    faNfa_destroy_(self);
    FREE(self);
    return;
}

void faNfa_create_(faNfa* self, unsigned length, unsigned cosink,
		   unsigned sink) {
    gsStack_of_gsStacks_create_with_length_(&self->edge_lists,
					    sizeof(faNfaEdge), length);
    self->cosink = cosink;
    self->sink = sink;
    return;
}

faNfa* faNfa_create(unsigned length, unsigned cosink, unsigned sink) {
    faNfa* const self = MALLOC(sizeof(*self));
    faNfa_create_(self, length, cosink, sink);
    return self;
}

void faNfa_create_token_(faNfa* self, unsigned token) {
    faNfa_create_(self, 2, 0, 1);
    faNfa_add_edge_(self, self->cosink, self->sink, token);
    return;
}

faNfa* faNfa_create_token(unsigned token) {
    faNfa* const self = MALLOC(sizeof(*self));
    faNfa_create_token_(self, token);
    return self;
}

faNfa* faNfa_create_none(void) {
    faNfa* const self = faNfa_create(2, 0, 1);
    return self;
}

faNfa* faNfa_create_empty(void) {
    faNfa* const self = faNfa_create(1, 0, 0);
    return self;
}

void faNfa_copy_(faNfa* self, const faNfa* nfa) {
    const unsigned length = faNfa_length(nfa);
    faNfa_create_(self, length, nfa->cosink, nfa->sink);
    for (unsigned i = 0; i < faNfa_length(self); ++i) {
	faNfaEdgeList* const self_edge_list = faNfa_edge_list(self, i);
	const faNfaEdgeList* const nfa_edge_list = faNfa_edge_list(nfa, i);
	gsStack_copy_(self_edge_list, nfa_edge_list);
    }
    return;
}

faNfa* faNfa_copy(const faNfa* nfa) {
    faNfa* self = MALLOC(sizeof(faNfa));
    faNfa_copy_(self, nfa);
    return self;
}

void faNfa_add_state_(faNfa* self) {
    gsStack_of_gsStacks_pre_append_(&self->edge_lists, sizeof(faNfaEdge));
    return;
}

void faNfa_add_edge_(faNfa* self, unsigned source, unsigned target,
                     unsigned token) {
    faNfaEdge edge;
    edge.target = target;
    edge.token = token;
    GS_APPEND(gsStack_element(&self->edge_lists, source), edge, faNfaEdge);
    return;
}

faNfa* faNfa_question__(faNfa* nfa) {
    faNfa_add_edge_(nfa, nfa->cosink, nfa->sink, 0);
    return nfa;
}

faNfa* faNfa_plus__(faNfa* nfa) {
    faNfa_add_edge_(nfa, nfa->sink, nfa->cosink, 0);
    return nfa;
}

faNfa* faNfa_star__(faNfa* nfa) {
    faNfa_add_edge_(nfa, nfa->cosink, nfa->sink, 0);
    faNfa_add_edge_(nfa, nfa->sink, nfa->cosink, 0);
    return nfa;
}

faNfa* faNfa_concat__(faNfa* nfa1, faNfa* nfa2) {
    faNfa* const nfa = MALLOC(sizeof(*nfa));
    nfa->cosink = nfa1->cosink;
    nfa->sink = nfa1->sink;
    gsStack_concat_(&nfa->edge_lists, &nfa1->edge_lists, &nfa2->edge_lists);

    const unsigned nfa2_length = faNfa_length(nfa2);
    for (unsigned i = 0; i < nfa2_length; ++i) {
        const faNfaEdgeList* const edge_list = faNfa_edge_list(nfa2, i);
        const unsigned edge_list_length = faNfaEdgeList_length(edge_list);
        for (unsigned j = 0; j < edge_list_length; ++j) {
            faNfaEdge *edge = faNfaEdgeList_edge(edge_list, j);
            edge->target += faNfa_length(nfa1);
        }
    }

    gsStack_destroy_(&nfa2->edge_lists);
    FREE(nfa2);
    gsStack_destroy_(&nfa1->edge_lists);
    FREE(nfa1);

    return nfa;
}

faNfa* faNfa_sum__(faNfa* nfa1, faNfa* nfa2) {
    const unsigned nfa1_source = nfa1->cosink + 2;
    const unsigned nfa2_source = nfa2->cosink + faNfa_length(nfa1) + 2;
    const unsigned nfa1_sink = nfa1->sink + 2;
    const unsigned nfa2_sink =  nfa2->sink + faNfa_length(nfa1) + 2;
    faNfa* const nfa =
        faNfa_concat__(faNfa_create_none(), faNfa_concat__(nfa1, nfa2));
    faNfa_add_edge_(nfa, 0, nfa1_source, 0);
    faNfa_add_edge_(nfa, 0, nfa2_source, 0);
    faNfa_add_edge_(nfa, nfa1_sink, 1, 0);
    faNfa_add_edge_(nfa, nfa2_sink, 1, 0);
    return nfa;
}

faNfa* faNfa_prod__(faNfa *nfa1, faNfa *nfa2) {
    const unsigned nfa2_source = nfa2->cosink + faNfa_length(nfa1);
    const unsigned nfa1_sink = nfa1->sink;
    const unsigned nfa2_sink = nfa2->sink + faNfa_length(nfa1);
    faNfa* const nfa = faNfa_concat__(nfa1, nfa2);
    nfa->sink = nfa2_sink;
    faNfa_add_edge_(nfa, nfa1_sink, nfa2_source, 0);
    return nfa;
}

inline unsigned faDfa_length(const faDfa* self) {
    return self->num_of_states;
}

inline unsigned faDfa_num_of_tokens(const faDfa* self) {
    return self->num_of_tokens;
}

unsigned faDfa_goto(const faDfa* self, unsigned source, unsigned token) {
    return self->transition_table[source * self->num_of_tokens + token];
}

void faDfa_destroy_(faDfa* self) {
    if (self == NULL) {
        return;
    }
    FREE(self->transition_table);
    FREE(self->sinks);
    return;
}

void faDfa_destroy(faDfa* self) {
    if (self == NULL) {
        return;
    }
    faDfa_destroy_(self);
    FREE(self);
    return;
}

boolean faDfa_accepts(const faDfa* self, const unsigned* tokens) {
    unsigned state = self->cosink;
    for (const unsigned* token = tokens; *token != 0; ++token) {
        state = faDfa_goto(self, state, *token);
        if (state == self->reject) {
            return false;
        }
    }
    if (self->sinks[state] == true) {
        return true;
    }
    return false;
}

/* used in functions _fa_aux_produce_subset_edge_candidates
   and faDfa_create_nfaec_with_subsets_ */
typedef struct _faAuxSubsetEdge {
    ssSubset target_subset;
    unsigned token;
} _faAuxSubsetEdge;

/* used in the function faDfa_create_nfaec_with_subsets_ */
static boolean _fa_aux_equal_subsets(const void* x, const void* y) {
    return ssSubset_are_equal((ssSubset*) x, (ssSubset*) y);
}

/* used in the function faDfa_create_nfaec_with_subsets_ */
static void _faNfa_do_epsilon_closure_(const faNfa* nfa, ssSubset* states) {
    gsStack* s = gsStack_copy(&(states->elements_list));
    gsStack* t = gsStack_create(sizeof(unsigned));
    for (;;) {
        while (gsStack_is_nonempty(s) == true) {
            unsigned source;
            GS_POP(s, source, unsigned);
            const faNfaEdgeList* const edge_list = faNfa_edge_list(nfa, source);
            const unsigned edge_list_length = faNfaEdgeList_length(edge_list);
            for (unsigned i = 0; i < edge_list_length; ++i) {
                faNfaEdge* const edge = faNfaEdgeList_edge(edge_list, i);
                if (
                    edge->token != 0 ||
                    (ssSubset_is_in(states, edge->target) == true)
                    ) {
                    continue;
                }
                GS_APPEND(t, edge->target, unsigned);
                ssSubset_add_(states, edge->target);
            }
        }
        if (gsStack_is_nonempty(t) == false) {
            break;
        }
        gsStack_destroy(s);
        s = t;
        t = gsStack_create(sizeof(unsigned));
    }
    gsStack_destroy(s);
    gsStack_destroy(t);
    return;
}

/* used in the function faDfa_create_nfaec_with_subsets_ */
static gsStack _fa_aux_produce_subset_edge_candidates(const faNfa* nfa,
                                                      const ssSubset* source_subset) {
    gsStack candidate_subset_edges;
    gsStack_create_(&candidate_subset_edges, sizeof(_faAuxSubsetEdge));
    const unsigned source_subset_length = ssSubset_length(source_subset);
    for (unsigned j = 0; j < source_subset_length; ++j) {
        const unsigned source = ssSubset_element(source_subset, j);
        const faNfaEdgeList* const edge_list = faNfa_edge_list(nfa, source);
        const unsigned edge_list_length = faNfaEdgeList_length(edge_list);
        for (unsigned k = 0; k < edge_list_length; ++k) {
            const faNfaEdge* const edge = faNfaEdgeList_edge(edge_list, k);
            if (edge->token == 0) {
                continue;
            }
            boolean token_already_appeared = false;
            _faAuxSubsetEdge* subset_edge;
            const unsigned candidate_subset_edges_length =
                gsStack_length(&candidate_subset_edges);
            for (unsigned m = 0; m < candidate_subset_edges_length; ++m) {
                subset_edge = (_faAuxSubsetEdge*) gsStack_element(
                    &candidate_subset_edges, m);
                if (edge->token == subset_edge->token) {
                    token_already_appeared = true;
                    break;
                }
            }
            if (token_already_appeared == true) {
                ssSubset_add_(&subset_edge->target_subset, edge->target);
            } else {
                gsStack_pre_append_(&candidate_subset_edges);
                _faAuxSubsetEdge* const new_subset_edge =
                    (_faAuxSubsetEdge*) gsStack_last(&candidate_subset_edges);
                ssSubset_create_(&new_subset_edge->target_subset,
				 faNfa_length(nfa));
                ssSubset_add_(&new_subset_edge->target_subset, edge->target);
                new_subset_edge->token = edge->token;
            }
        }
    }
    return candidate_subset_edges;
}

void faDfa_create_nfaec_with_subsets_(faDfa* self, gsStack* subsets,
                                      const faNfa* nfa,
				      unsignedMaybe num_of_tokens) {
    gsStack_create_(subsets, sizeof(ssSubset));
    boolean is_in;

    self->num_of_states = 0;

    /* calculate number of tokens if not provided */
    if (unsignedMaybe_is(num_of_tokens) == true) {
        self->num_of_tokens = unsignedMaybe_value(num_of_tokens);
    } else {
        unsigned maximal_token = 0;
        const unsigned nfa_length = faNfa_length(nfa);
        for (unsigned i = 0; i < nfa_length; ++i) {
            const faNfaEdgeList* const edge_list = faNfa_edge_list(nfa, i);
            const unsigned edge_list_length = faNfaEdgeList_length(edge_list);
            for (unsigned j = 0; j < edge_list_length; ++j) {
                const faNfaEdge* const edge = faNfaEdgeList_edge(edge_list, j);
                if (edge->token > maximal_token) {
                    maximal_token = edge->token;
                }
            }
        }
        self->num_of_tokens = maximal_token + 1;
    }

    gsStack transition_table, sinks;
    gsStack_create_(&transition_table, self->num_of_tokens * sizeof(unsigned));
    gsStack_create_(&sinks, sizeof(boolean));

    /* add the empty set, which will serve as a reject state */
    gsStack_pre_append_(subsets);
    ssSubset_create_((ssSubset*) gsStack_last(subsets), faNfa_length(nfa));
    is_in = ssSubset_is_in((ssSubset*) gsStack_last(subsets), nfa->sink);
    GS_APPEND(&sinks, is_in, boolean);
    self->reject = self->num_of_states++;
    gsStack_pre_append_(&transition_table);
    for (unsigned i = 0; i < self->num_of_tokens; ++i) {
        ((unsigned*) gsStack_last(&transition_table))[i] = self->reject;
    }

    /* add the epsilon closure of the cosink, which will serve as a cosink */
    gsStack_pre_append_(subsets);
    ssSubset_create_((ssSubset*) gsStack_last(subsets), faNfa_length(nfa));
    ssSubset_add_((ssSubset*) gsStack_last(subsets), nfa->cosink);
    _faNfa_do_epsilon_closure_(nfa, (ssSubset*) gsStack_last(subsets));
    GS_APPEND(&sinks,
	      ssSubset_is_in((ssSubset*) gsStack_last(subsets), nfa->sink),
	      boolean);
    self->cosink = self->num_of_states++;
    gsStack_pre_append_(&transition_table);
    for (unsigned i = 0; i < self->num_of_tokens; ++i) {
        ((unsigned*) gsStack_last(&transition_table))[i] = self->reject;
    }
    
    unsigned previous_length = 0;
    unsigned current_length = gsStack_length(subsets);
    while (previous_length < current_length) {
        for (unsigned i = previous_length; i < current_length; ++i) {
            const ssSubset* const source_subset =
                (ssSubset*) gsStack_element(subsets, i);
            gsStack candidate_subset_edges =
                _fa_aux_produce_subset_edge_candidates(nfa, source_subset);
            const unsigned candidate_subset_edges_length =
                gsStack_length(&candidate_subset_edges);
            for (unsigned j = 0; j < candidate_subset_edges_length; ++j) {
                _faAuxSubsetEdge* const candidate_subset_edge =
                    (_faAuxSubsetEdge *) gsStack_element(&candidate_subset_edges, j);
                unsigned source, target, token;
                _faNfa_do_epsilon_closure_(nfa, &candidate_subset_edge->target_subset);
                unsigned index =
                    gsStack_element_index(subsets,
                                          &candidate_subset_edge->target_subset,
                                          _fa_aux_equal_subsets);
                if (index != gsStack_length(subsets)) {
                    source = i;
                    target = index;
                    token = candidate_subset_edge->token;
                    ((unsigned*) gsStack_0(&transition_table))
			[source * self->num_of_tokens + token] = target;
                    ssSubset_destroy_(&candidate_subset_edge->target_subset);
                } else {
                    GS_APPEND(subsets, candidate_subset_edge->target_subset,
                              ssSubset);
                    GS_APPEND(&sinks,
			      ssSubset_is_in((ssSubset*) gsStack_last(subsets),
					     nfa->sink), boolean);
                    ++self->num_of_states;
                    gsStack_pre_append_(&transition_table);
                    for (unsigned i = 0; i < self->num_of_tokens; ++i) {
                        ((unsigned*) gsStack_last(&transition_table))[i] =
                            self->reject;
                    }
                    source = i;
                    target = gsStack_length(subsets) - 1;
                    token = candidate_subset_edge->token;
                    ((unsigned*) gsStack_0(&transition_table))
			[source * self->num_of_tokens + token] = target;
                }
            }
            gsStack_destroy_(&candidate_subset_edges);
        }
        previous_length = current_length;
        current_length = gsStack_length(subsets);
    }

    self->transition_table = gsStack_0(&transition_table);
    self->sinks = gsStack_0(&sinks);
    return;
}

void faDfa_create_nfaec_(faDfa* self, const faNfa* nfa,
                         unsignedMaybe num_of_tokens) {
    gsStack subsets;
    faDfa_create_nfaec_with_subsets_(self, &subsets, nfa, num_of_tokens);
    if (gsStack_length(&subsets) != 0) {
        ssSubset* const s0 = gsStack_0(&subsets);
	for (ssSubset* s = gsStack_end(&subsets); s > s0;) {
	    ssSubset_destroy_(--s);
	}	
    }
    gsStack_destroy_(&subsets);
    return;
}

faDfa* faDfa_create_nfaec(const faNfa* nfa, unsignedMaybe num_of_tokens) {
    faDfa* self = MALLOC(sizeof(*self));
    faDfa_create_nfaec_(self, nfa, num_of_tokens);
    return self;
}

void faDfaOfChars_destroy_(faDfaOfChars* self) {
    if (self == NULL) {
        return;
    }
    FREE(self->dfa.transition_table);
    FREE(self->dfa.sinks);
    return;
}

void faDfaOfChars_destroy(faDfaOfChars* self) {
    if (self == NULL) {
        return;
    }
    faDfaOfChars_destroy_(self);
    FREE(self);
    return;
}

boolean faDfaOfChars_accepts(const faDfaOfChars* self,
                             const char* str) {
    unsigned state = self->dfa.cosink;
    for (const char* token = str; *token != 0; ++token) {
        state =
            faDfa_goto(&self->dfa, state,
		       self->char_to_token_table[(unsigned char) *token]);
        if (state == self->dfa.reject) {
            return false;
        }
    }
    if (self->dfa.sinks[state] == true) {
        return true;
    }
    return false;
}

unsigned faDfaOfChars_race(unsigned length, const faDfaOfChars* const* selves,
                           const char* str,
                           faDfaOfCharsRaceAux* aux,
                           const char** out_end_pos) {
    unsigned num_of_rejected = 0;
    boolean found_someone = false;
    unsigned winner;
    for (unsigned i = 0; i < length; ++i) {
        if (selves[i] != NULL) {
            aux[i].state = selves[i]->dfa.cosink;
            aux[i].rejected = false;
        } else {
            aux[i].rejected = true;
        }
    }
    const char* cursor = str;
    for(;;) {
        for (unsigned i = 0; i < length; ++i) {
            if (aux[i].rejected == false) {
                if (aux[i].state == selves[i]->dfa.reject) {
                    ++num_of_rejected;
                    if (num_of_rejected == length) {
                        goto exit_all_fors;
                    }
                    aux[i].rejected = true;
                } else if (selves[i]->dfa.sinks[aux[i].state] == true) {
                    found_someone = true;
                    aux[i].accept = cursor;
                    winner = i;
                }
            }
        }
        if (*cursor == 0) {
            break;
        }
        for (unsigned i = 0; i < length; ++i) {
            if (aux[i].rejected == false) {
                aux[i].state =
		    faDfa_goto(&selves[i]->dfa, aux[i].state,
			       selves[i]->char_to_token_table[
				   (unsigned char) *cursor]);
            }
        }
        ++cursor;
    }

    exit_all_fors:;
    if (found_someone == true) {
        *out_end_pos = aux[winner].accept;
        return winner;
    } else {
        return length;
    }
}

void faDfaBt_destroy_(faDfaBt* self) {
    if (self == NULL) {
        return;
    }
    faDfa_destroy_(&self->dfa);
    FREE(self->bt_list);
    FREE(self->bt_table);
    return;
}

void faDfaBt_destroy(faDfaBt* self) {
    if (self == NULL) {
        return;
    }
    faDfaBt_destroy_(self);
    FREE(self);
    return;
}

unsigned* faDfaBt_parse(const faDfaBt* self, const unsigned* tokens,
                        const unsigned** out_end_pos) {
    boolean successful;
    gsStack path, parse_items;
    gsStack_create_(&path, sizeof(unsigned));
    gsStack_create_(&parse_items, sizeof(unsigned));
    GS_APPEND(&path, self->dfa.cosink, unsigned);

    const unsigned* cursor = tokens;
    for (;;) {
        unsigned next_state;
        if (self->dfa.sinks[* (unsigned*) gsStack_last(&path)] == true) {
            successful = true;
            break;
        }
        if (* (unsigned*) gsStack_last(&path) == self->dfa.reject) {
            successful = false;
            break;
        }
        if (*cursor != 0) {
            next_state = faDfa_goto(&self->dfa,
                                    * (unsigned*) gsStack_last(&path), *cursor);
            if (next_state != self->dfa.reject) {
                GS_APPEND(&parse_items, *cursor, unsigned);
                GS_APPEND(&path, next_state, unsigned);
                ++cursor;
                continue;
            }
        }
        unsigned bt_id = self->bt_table[(* (unsigned*) gsStack_last(&path)) *
                                        self->dfa.num_of_tokens + *cursor];
        if (bt_id == 0) {
            successful = false;
            break;
        }
        /*
          perform a shift,
          since the 0th item for bt_table is just a reject item,
          which the bt_list does not contain
        */
        --bt_id;
        const faBtItem* const bt = self->bt_list + bt_id;
        GS_APPEND(&parse_items, self->dfa.num_of_tokens + bt_id, unsigned);
        gsStack_post_pop_several_(&path, bt->num_of_steps);
        next_state = faDfa_goto(&self->dfa, * (unsigned*) gsStack_last(&path),
                                bt->replacing_token);
        GS_APPEND(&path, next_state, unsigned);
    }

    gsStack_destroy_(&path);
    *out_end_pos = cursor;
    if (successful == true) {
        GS_APPEND(&parse_items, 0, unsigned);
        return gsStack_0(&parse_items);
    } else {
        gsStack_destroy_(&parse_items);
        return NULL;
    }
}

void faDfaBt_synthesize(const faDfaBt* self, const unsigned* items,
                        const unsigned* vals, unsigned element_size,
                        faTokenSynthFn token_synth_fn,
                        faBtSynthFn bt_synth_fn, void* extra, void* result) {
    gsStack attributes;
    gsStack_create_(&attributes, element_size);
    const unsigned* current_val = vals;
    for (const unsigned* item = items; *item != 0; ++item) {
        if (*item < self->dfa.num_of_tokens) {
            gsStack_pre_append_(&attributes);
            token_synth_fn(*item, *current_val, gsStack_last(&attributes),
			   extra);
            ++current_val;
        } else {
            const unsigned bt = *item - self->dfa.num_of_tokens;
            const unsigned num_of_steps = self->bt_list[bt].num_of_steps;
            gsStack_pre_append_(&attributes);
            bt_synth_fn(bt, gsStack_last(&attributes), extra);
            memcpy((char*) gsStack_last(&attributes)
		   - num_of_steps * element_size,
                   gsStack_last(&attributes), element_size);
            gsStack_post_pop_several_(&attributes, num_of_steps);
        }
    }
    memcpy(result, gsStack_last(&attributes), element_size);
    gsStack_destroy_(&attributes);
    return;
}

/*-------------------------*//*-------------------------*/
/*-------------------------*//*-------------------------*/
/*-------------------------*//*-------------------------*/

#ifdef TESTING_PRINTS

#include <stdio.h>

static void print2digits(unsigned u) {
    if (u < 10) {
        printf("%u ", u);
    } else {
        printf("%u", u);
    }
}

void faNfa_print(const faNfa* self) {
    printf("----\n");
    printf("The NFA has:\n");
    const unsigned length = faNfa_length(self);
    printf("  * %u states (cosink %u, sink %u)\n", length, self->cosink,
	   self->sink);
    printf("  * edges: \n");
    for (unsigned i = 0; i < length; ++i) {
        const faNfaEdgeList* const edge_list = faNfa_edge_list(self, i);
        const unsigned edge_list_length = faNfaEdgeList_length(edge_list);
        for (unsigned j = 0; j < edge_list_length; ++j) {
            faNfaEdge* edge = faNfaEdgeList_edge(edge_list, j);
            printf("     %u --(%u)--> %u\n", i, edge->token, edge->target);
        }
    }
    printf("----\n");
    return;
}

void faDfa_print(const faDfa* self) {
    printf("----\n");
    printf("The DFA has:\n");
    printf("  * %u states (cosink %u, reject %u) and %u tokens\n",
	   self->num_of_states, self->cosink, self->reject,
	   self->num_of_tokens);
    printf("  * sinks");
    for (unsigned i = 0; i < self->num_of_states; ++i) {
        if (self->sinks[i] == true) {
            printf(" %u", i);
        }
    }
    printf("\n");
    printf("  * The following goto table "
           "(rows for tokens, columns for states):\n");
    printf("     ");
    for (unsigned j = 0; j < self->num_of_states; ++j) {
        print2digits(j);
        printf(" ");
    }
    printf("\n");
    for (unsigned i = 0; i < self->num_of_tokens; ++i) {
        print2digits(i);
        printf(" | ");
        for (unsigned j = 0; j < self->num_of_states; ++j) {
            print2digits(faDfa_goto(self, j, i));
            printf(" ");
        }
        printf("\n");
    }
    printf("----\n");
    return;
}

void faDfaOfChars_print(const faDfaOfChars* self) {
    printf("----\n");
    printf("The DFA has:\n");
    printf("  * %u states (cosink %u, reject %u) and %u tokens\n",
	   self->dfa.num_of_states, self->dfa.cosink, self->dfa.reject,
	   self->dfa.num_of_tokens);
    printf("  * sinks");
    for (unsigned i = 0; i < self->dfa.num_of_states; ++i) {
        if (self->dfa.sinks[i] == true) {
            printf(" %u", i);
        }
    }
    printf("\n");
    printf("  * The following goto table "
           "(rows for tokens, columns for states):\n");
    printf("     ");
    for (unsigned j = 0; j < self->dfa.num_of_states; ++j) {
        print2digits(j);
        printf(" ");
    }
    printf("\n");
    for (unsigned i = 0; i < self->dfa.num_of_tokens; ++i) {
        print2digits(i);
        printf(" | ");
        for (unsigned j = 0; j < self->dfa.num_of_states; ++j) {
            print2digits(faDfa_goto(&self->dfa, j, i));
            printf(" ");
        }
        printf("\n");
    }
    printf("  * The following char to token correspondence:\n");
    for (unsigned i = 0; i < self->dfa.num_of_tokens; ++i) {
        printf("    token %u: ", i);
        for (unsigned j = 0; j < 128; ++j) {
            if (self->char_to_token_table[j] == i) {
                if (j <= 32 || j == 127) {
                    printf("(%u) ", j);
                } else {
                    printf("%c ", (char) j);
                }
            }
        }
        printf("\n");
    }
    printf("----\n");
    return;
}

#endif /* TESTING_PRINTS */
