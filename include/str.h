#ifndef STR_HEADER
#define STR_HEADER

#include <stddef.h>

#include "standard.h"

/*
 * string utilities
 */

/*-------------------------*/
/* macros                  */
/*-------------------------*/

/*-------------------------*/
/* types                   */
/*-------------------------*/

/*-------------------------*/
/* functions               */
/*-------------------------*/

extern  char*          str_read_file(const char* file_name);

extern  char*          str_create_copy(const char* str);
extern  char*          str_create_copy_from_to(const char* str_start,
					       const char* str_end);

extern  const char*    str_while_visible(const char* str);
extern  const char*    str_while_not_visible(const char* str);
extern  const char*    str_while_not_visible_except_newline(const char* str);
extern  const char*    str_while_not_newline(const char* str);

extern  boolean        str_equal_to(const char* str, const char* comparand);
/* return comparand_list_length if not found... */
extern  size_t         str_equal_to_one_of(const char* str,
					   size_t comparand_list_length,
					   const char* const* comparand_list);
/* the following two functions return the cursor advancement after a
   successful match, or NULL if there is no successful match */
extern  const char*    str_starts_with(const char* str, const char* prefix);
extern  const char*    str_starts_with_one_of(const char* str,
					      size_t prefix_list_length,
					      const char* const* prefix_list,
					      size_t* out_index);

extern  void           str_remove_trailing_newline_(char* str);

/* return the position  of the next 0 */
extern  char*          str_goto_end(char* str);

#endif /* STR_HEADER */
