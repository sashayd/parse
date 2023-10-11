#ifndef FA_HEADER
#define FA_HEADER

#include "standard.h"
#include "gs.h"

/*
 * "fa" stands for "finite automaton".
 */

/*-------------------------*/
/* macros                  */
/*-------------------------*/

/*-------------------------*/
/* types                   */
/*-------------------------*/

typedef struct faNfaEdge {
    unsigned target;
    unsigned token;
}                               faNfaEdge;
typedef gsStack                 faNfaEdgeList;
typedef struct faNfa            faNfa;

typedef struct faDfa {
    unsigned num_of_states;
    unsigned num_of_tokens;
    unsigned cosink;
    /* of length num_of_states */
    boolean* sinks;
    unsigned reject;
    /* transition_table[source_state * num_of_tokens + token] = target_state */
    unsigned* transition_table;
}                               faDfa;

typedef struct faDfaOfChars {
    /* mapping from ASCII characters to tokens */
    unsigned char_to_token_table[128];
    faDfa dfa;
}                               faDfaOfChars;

typedef struct faBtItem {
    unsigned num_of_steps;
    unsigned replacing_token;
}                               faBtItem;

typedef struct faDfaBt {
    faDfa dfa;
    unsigned bt_list_length;
    faBtItem* bt_list;
    /* bt_table[source_state * num_of_tokens + token] = bt_id */
    unsigned* bt_table;
}                               faDfaBt;

typedef void                    (*faTokenSynthFn)(unsigned token, unsigned val,
                                                  void* attribute, void* extra);
typedef void                    (*faBtSynthFn)(unsigned bt, void* attributes,
                                               void* extra);

typedef struct faDfaOfCharsRaceAux {
    unsigned state;
    boolean rejected;
    unsigned reject;
    const char* accept;
}                               faDfaOfCharsRaceAux;

/*-------------------------*/
/* functions               */
/*-------------------------*/


/* faNfa */

extern  unsigned        faNfa_length(const faNfa* self);
extern  unsigned        faNfa_cosink(const faNfa* self);
extern  unsigned        faNfa_sink(const faNfa* self);
extern  faNfaEdgeList*  faNfa_edge_list(const faNfa* self, unsigned source);
extern  unsigned        faNfaEdgeList_length(const faNfaEdgeList* self);
extern  faNfaEdge*      faNfaEdgeList_edge(const faNfaEdgeList* self,
					   unsigned index);

extern  void            faNfa_destroy_(faNfa* self);
extern  void            faNfa_destroy(faNfa* self);
extern  void            faNfa_create_(faNfa* self, unsigned length,
				      unsigned cosink, unsigned sink);
/* creates nfa with no edges */
extern  faNfa*          faNfa_create(unsigned length, unsigned cosink,
				     unsigned sink);
/* "token" means nfa with L(nfa) = {token} */
extern  void            faNfa_create_token_(faNfa* self, unsigned token);
extern  faNfa*          faNfa_create_token(unsigned token);
/* returns nfa with L(nfa) = {} */
extern  faNfa*          faNfa_create_none(void);
/* returns nfa with L(nfa) = {empty_string} */
extern  faNfa*          faNfa_create_empty(void);

extern  void            faNfa_copy_(faNfa* self, const faNfa* nfa);
extern  faNfa*          faNfa_copy(const faNfa* nfa);

extern  void            faNfa_add_state_(faNfa* self);
extern  void            faNfa_add_edge_(faNfa* self, unsigned source,
					unsigned target, unsigned token);

/*
  functions in this group accept "floating" objects
  (object itself allocated in heap), and responsibility for destruction migrates
  from inputs to output (hence the suffix "__").
  be careful with the binary functions, can't have the two nfa's using
  same memory, for example instead of faNfa_sum__(nfa, nfa)
  should do faNfa_sum__(nfa, faNfa_copy(nfa))
*/

/* returns nfa with L(nfa) = L(nfa1)? */
extern  faNfa*          faNfa_question__(faNfa* nfa);
/* returns nfa with L(nfa) = L(nfa1)+ */
extern  faNfa*          faNfa_plus__(faNfa* nfa);
/* returns nfa with L(nfa) = L(nfa1)* */
extern  faNfa*          faNfa_star__(faNfa* nfa);
/* concats two nfas, no edges between them, source & sink of first one */
extern  faNfa*          faNfa_concat__(faNfa* nfa1, faNfa* nfa2);
/* returns nfa with L(nfa) = L(nfa1)|L(nfa2) */
extern  faNfa*          faNfa_sum__(faNfa* nfa1, faNfa* nfa2);
/* returns nfa with L(nfa) = L(nfa1)L(nfa2) */
extern  faNfa*          faNfa_prod__(faNfa* nfa1, faNfa* nfa2);

/* faDfa */

extern  unsigned        faDfa_length(const faDfa* self);
extern  unsigned        faDfa_num_of_tokens(const faDfa* self);
extern  unsigned        faDfa_goto(const faDfa* self, unsigned source,
				   unsigned token);

extern  void            faDfa_destroy_(faDfa* self);
extern  void            faDfa_destroy(faDfa* self);

extern  boolean         faDfa_accepts(const faDfa* self,
				      const unsigned* token_seqeunce);

/*
  create epsilon closure construction dfa from nfa,
  0-th state of the dfa is the empty set, serving as a reject state
  1-th state of the dfa is the epsilon closure of the cosink, serving as a cosink,
  sinks of the dfa are the subsets which contain the sink of the nfa.
  this gives a very memory-inefficient DFA if the tokens are sparse,
  they should more or less form an interval [0, num_of_tokens)...
*/
extern  void            faDfa_create_nfaec_with_subsets_(faDfa* self,
							 gsStack* subsets,
							 const faNfa* nfa,
							 unsignedMaybe num_of_tokens);
extern  void            faDfa_create_nfaec_(faDfa* self, const faNfa* nfa,
					    unsignedMaybe num_of_tokens);
extern  faDfa*          faDfa_create_nfaec(const faNfa* nfa,
					   unsignedMaybe num_of_tokens);

/* faDfaOfChars */

extern  void            faDfaOfChars_destroy_(faDfaOfChars* self);
extern  void            faDfaOfChars_destroy(faDfaOfChars* self);

extern  boolean         faDfaOfChars_accepts(const faDfaOfChars* self,
					     const char* str);
extern  unsigned        faDfaOfChars_race(unsigned length,
					  const faDfaOfChars* const* selves,
					  const char* str,
					  faDfaOfCharsRaceAux* aux,
					  const char** out_end_pos);

/* faDfaBt */

extern  void            faDfaBt_destroy_(faDfaBt* self);
extern  void            faDfaBt_destroy(faDfaBt* self);

extern  unsigned*       faDfaBt_parse(const faDfaBt* self,
				      const unsigned* tokens,
				      const unsigned** out_end_pos);
extern  void            faDfaBt_synthesize(const faDfaBt* self,
					   const unsigned* items,
					   const unsigned* vals,
					   unsigned element_size,
					   faTokenSynthFn token_synth_fn,
					   faBtSynthFn bt_synth_fn,
					   void* extra, void* result);

#ifdef TESTING_PRINTS
extern  void            faNfa_print(const faNfa* self);
extern  void            faDfa_print(const faDfa* self);
extern  void            faDfaOfChars_print(const faDfaOfChars* self);
#endif /* TESTING_PRINTS */

#endif /* FA_HEADER */
