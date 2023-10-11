#ifndef LEXER_HEADER
#define LEXER_HEADER

#include "standard.h"
#include "regex.h"

/*
 * "lex" stands for "lexical analyzer", or "lexer".
 */

/*-------------------------*/
/* macros                  */
/*-------------------------*/

/*-------------------------*/
/* types                   */
/*-------------------------*/

typedef struct lexLexer lexLexer;

typedef unsigned        (*lexStrToValFn)(unsigned token,
                                         const char* str_start,
                                         const char* str_end,
                                         void* extra);

/*-------------------------*/
/* functions               */
/*-------------------------*/

/* lexLexer */

extern  void        lexLexer_destroy(lexLexer* self);
extern  lexLexer*   lexLexer_create_from_spec(const char *spec,
                                              const rexRegexSLRParser* regex_slr_parser);

extern  unsigned*   lexLexer_process(const lexLexer* self,
                                     const char* str,
                                     lexStrToValFn str_to_val_func,
                                     void* extra,
                                     unsigned** out_vals,
                                     const char** out_end_pos);

#ifdef TESTING_PRINTS
extern  void        lexLexer_print(const lexLexer* self);
extern  void        lexLexer_print_process_result(const lexLexer* self,
                                                  const unsigned* tokens,
                                                  const unsigned* vals);
#endif /* TESTING_PRINTS */

#endif /* LEXER_HEADER */
