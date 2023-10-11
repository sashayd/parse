#ifndef REGEX_HEADER
#define REGEX_HEADER

#include "standard.h"

/*
 * "rex" stands for "regex" (which stands for "regular expression")
 */

/*-------------------------*/
/* macros                  */
/*-------------------------*/

/*-------------------------*/
/* types                   */
/*-------------------------*/

typedef struct rexRegexSLRParser    rexRegexSLRParser;
typedef struct rexCompiledRegex     rexCompiledRegex;
typedef struct rexCompiledRegexList rexCompiledRegexList;

/*-------------------------*/
/* functions               */
/*-------------------------*/

/* rexRegexSLRParser */

extern  void                    rexRegexSLRParser_destroy(rexRegexSLRParser* self);
extern  rexRegexSLRParser*      rexRegexSLRParser_create(void);

/* rexCompiledRegex */

extern  void                    rexCompiledRegex_destroy(rexCompiledRegex* self);
extern  rexCompiledRegex*       rexCompiledRegex_create_from_regex(const rexRegexSLRParser* regex_slr_parser,
                                                                   const char* regex,
                                                                   const char* regex_end);
/* just a (very non efficient) utility it was convenient to have here */
extern  rexCompiledRegex*       rexCompiledRegex_create_from_raw_str(
    const rexRegexSLRParser* regex_slr_parser,
    const char* str_start,
    const char* str_end);

extern  boolean                 rexCompiledRegex_accepts(const rexCompiledRegex* self,
                                                         const char* str);

/* rexCompiledRegexList */

extern  void                    rexCompiledRegexList_destroy(rexCompiledRegexList* self);
extern  rexCompiledRegexList*   rexCompiledRegexList_create_from_compiled_regex_list__(
    unsigned length,
    rexCompiledRegex** copmiled_regexes);

extern  unsigned                rexCompiledRegexList_race(const rexCompiledRegexList* self,
                                                          const char* str,
                                                          const char** out_end_pos);

#endif /* REGEX_HEADER */
