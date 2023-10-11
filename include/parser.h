#ifndef PARSER_HEADER
#define PARSER_HEADER

#include "standard.h"

/*
 * "pr" stands for "parser".
 */

/*-------------------------*/
/* macros                  */
/*-------------------------*/

/*-------------------------*/
/* types                   */
/*-------------------------*/

typedef struct prGrammar    prGrammar;
typedef struct prSLRParser  prSLRParser;

typedef void                (*prTerminalSynthFn)(unsigned token,
                                                 unsigned val,
                                                 void* attribute,
                                                 void* extra);
typedef void                (*prProductionSynthFn)(unsigned production,
                                                   void* attributes,
                                                   void* extra);

/*-------------------------*/
/* functions               */
/*-------------------------*/

/* prGrammar */

extern  unsigned        prGrammar_token_by_name(const prGrammar* self,
                                                const char *name);

extern  unsigned        prGrammar_production_by_name(const prGrammar* self,
                                                     const char *name);

extern  void            prGrammar_destroy(prGrammar* self);
extern  prGrammar*      prGrammar_create_from_spec(const char* spec,
						   char* out_diagnostic);
extern  prGrammar*      prGrammar_create_from_spec_given_terminal_names(
    const char* spec,
    unsigned terminal_names_list_length,
    const char* const* terminal_names_list,
    char* out_diagnostic);

/* prSLRParser */

extern  void            prSLRParser_destroy(prSLRParser* self);
extern  prSLRParser*    prSLRParser_create_from_grammar(const prGrammar* grammar,
							char* out_diagnostic);

extern  unsigned*       prSLRParser_parse(const prSLRParser* self,
                                          const unsigned* tokens,
                                          const unsigned** out_end_pos);

/* an "item" here, item, encodes a token
   with serial number item if
   item < num_of_tokens of the grammar,
   and otherwise it encodes the production
   with serial number item - num_of_tokens */
extern  void            prSLRParser_synthesize(const prSLRParser* self,
                                               const unsigned* items,
                                               const unsigned* vals,
                                               unsigned element_size,
                                               prTerminalSynthFn terminal_synth_fn,
                                               prProductionSynthFn production_synth_fn,
                                               void* extra,
                                               void* result);

#ifdef TESTING_PRINTS
extern  void            prGrammar_print(const prGrammar* self);
#endif /* TESTING_PRINTS */

#endif /* PARSER_HEADER */
