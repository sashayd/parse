#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "standard.h"
#include "ma.h"
#include "../src/fa.h"

int main(void) {
    ma_initialize();

    srand(time(NULL));

    unsigned num_of_operations;
    const unsigned max_token = 5;

    printf("How many random operations to do in the NFA creation?\n");
    scanf("%u", &num_of_operations);

    faNfa* nfa = faNfa_create(2, 0, 1);

    for (unsigned i = 0; i < num_of_operations; ++i) {
	const unsigned r = rand() % 7;
	unsigned t1, t2, source, target, token;
	switch (r) {
	case 0:;
	    nfa = faNfa_question__(nfa);
	    break;
	case 1:;
	    nfa = faNfa_plus__(nfa);
	    break;
	case 2:;
	    nfa = faNfa_star__(nfa);
	    break;
	case 3:;
	    t1 = rand() % max_token;
	    t2 = rand() % max_token;
	    nfa = faNfa_sum__(nfa, faNfa_create_token(t1+1));
	    nfa = faNfa_sum__(faNfa_create_token(t2+1), nfa);
	    nfa = faNfa_sum__(nfa, faNfa_create_none());
	    nfa = faNfa_sum__(faNfa_create_empty(), nfa);
	    break;
	case 4:;
	    t1 = rand() % max_token;
	    t2 = rand() % max_token;
	    nfa = faNfa_prod__(nfa, faNfa_create_token(t1+1));
	    nfa = faNfa_prod__(faNfa_create_token(t2+1), nfa);
	    break;
	case 5:;
	    t1 = rand() % max_token;
	    t2 = rand() % max_token;
	    nfa = faNfa_concat__(nfa, faNfa_create_token(t1));
	    nfa = faNfa_concat__(faNfa_create_token(t2), nfa);
	    break;
	case 6:;
	    source = rand() % faNfa_length(nfa);
	    target = rand() % faNfa_length(nfa);
	    token = rand() % max_token;
	    faNfa_add_edge_(nfa, 0, faNfa_sink(nfa), token+1);
	    faNfa_add_edge_(nfa, source, faNfa_sink(nfa), token+1);
	    faNfa_add_edge_(nfa, target, faNfa_sink(nfa), token+1);
	    faNfa_add_edge_(nfa, target, faNfa_sink(nfa), token+1);
	}
    }

    const unsigned length = faNfa_length(nfa);
    size_t num_of_edges = 0;
    for (unsigned i = 0; i < length; ++i) {
	const faNfaEdgeList* edge_list = faNfa_edge_list(nfa, i);
	num_of_edges += faNfaEdgeList_length(edge_list);
    }

    printf("The resulting NFA has %u states and %lu edges.\n",
	   length, num_of_edges);

    if (length < 32) {
	faNfa_print(nfa);
    }

    printf("Will now produce a DFA out of the NFA using "
	   "the epsilon closure subset construction, "
	   "and see how it goes.\n");

    faDfa* dfa = faDfa_create_nfaec(nfa, unsignedMaybe_from_false());

    printf("The resulting DFA has %u states and %u tokens.\n",
	   faDfa_length(dfa), faDfa_num_of_tokens(dfa));

    if (faDfa_length(dfa) < 32 && faDfa_num_of_tokens(dfa) < 32) {
	faDfa_print(dfa);
    }

    faNfa_destroy(nfa);
    faDfa_destroy(dfa);

    ma_finalize();
    return 0;
}
