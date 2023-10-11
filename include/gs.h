#ifndef GS_HEADER
#define GS_HEADER

#include <stddef.h>

#include "standard.h"

/*
 * "gs" stands for "generic stack".
 * gsStack is not necessarily used strictly as a stack,
 * might serve as a random access list.
 * access to elements and popping should not be
 * carried if length == 0.
 */

/*-------------------------*/
/* macros                  */
/*-------------------------*/

/* when allocating more than 0 bytes,
   allocate roughly no less than that many bytes */
#define GS_MINIMAL_ALLOCATION 64
/* shrink allocation only when length reduces to
   roughly allocation divided by that much */
#define GS_DEALLOCATION_FACTOR 4

/* possible macros using typeof */
/* #define GS_APPENDT(L, X) gsStack_pre_append_(L); (* ((typeof(X) *) gsStack_last(L))) = X */
/* #define GS_PEEKT(L, X) X = (* ((typeof(X) *) gsStack_last(L))) */
/* #define GS_POPT(L, X) GS_PEEKT(L, X); gsStack_post_pop_(L) */

#define GS_APPEND(L, X, T) gsStack_pre_append_(L); (* ((T *) gsStack_last(L))) = X
#define GS_PEEK(L, X, T) X = (* (T *) gsStack_last(L))
#define GS_POP(L, X, T) GS_PEEK(L, X, T); gsStack_post_pop_(L)

/*-------------------------*/
/* types                   */
/*-------------------------*/

typedef boolean     (*gsGenericComparisonFn)(const void* x, const void* y);

typedef struct gsStack {
    size_t element_size;
    size_t length;
    size_t allocated_length;
    /* pointer to 0-th element */
    void* first;
    /* pointer to length-th element (which is not part of the stack) */
    void* end;
}                   gsStack;

/*-------------------------*/
/* functions               */
/*-------------------------*/

extern  size_t      gsStack_length(const gsStack* self);
extern  void*       gsStack_0(const gsStack* self);
extern  void*       gsStack_end(const gsStack* self);
extern  void*       gsStack_last(const gsStack* self);
extern  void*       gsStack_element(const gsStack* self, size_t index);
extern  boolean     gsStack_is_nonempty(const gsStack* self);
/* return the index of the first element for which true is
   returned when compared with given element,
   if found; else return the length of the stack */
extern  size_t      gsStack_element_index(const gsStack* self,
					  const void* element,
                                          gsGenericComparisonFn f);

extern  void        gsStack_destroy_(gsStack* self);
extern  void        gsStack_destroy(gsStack* self);
extern  void        gsStack_create_(gsStack* self, size_t element_size);
extern  gsStack*    gsStack_create(size_t element_size);
extern  void        gsStack_create_with_length_(gsStack* self,
                                                size_t element_size,
                                                size_t length);
extern  gsStack*    gsStack_create_with_length(size_t element_size,
					       size_t length);
extern  void        gsStack_create_zeros_(gsStack* self, size_t element_size,
                                          size_t length);
extern  gsStack*    gsStack_create_zeros(size_t element_size, size_t length);
extern  void        gsStack_copy_(gsStack* self, const gsStack* l);
extern  gsStack*    gsStack_copy(const gsStack *l);

/* this increases length by one, allocating more memory if needed,
   with no initialization of added element */
extern  void        gsStack_pre_append_(gsStack* self);
extern  void        gsStack_pre_append_several_(gsStack* self, size_t how_many);
/* this decreases length by one, perhaps deallocating memory
   (depending on macro values above); should not be used if length is 0 */
extern  void        gsStack_post_pop_(gsStack* self);
/* is fine to use if length < how_many
   (re-sets how_many to length in that case) */
extern  void        gsStack_post_pop_several_(gsStack* self, size_t how_many);
/* keeps allocated memory intact */
extern  void        gsStack_make_empty_(gsStack* self);
extern  void        gsStack_make_room_(gsStack* self, size_t how_many);

extern  void        gsStack_concat_(gsStack* self, const gsStack* l1,
                                    const gsStack* l2);
extern  gsStack*    gsStack_concat(const gsStack* l1, const gsStack* l2);

extern  void        gsStack_of_gsStacks_destroy_(gsStack* self);
extern  void        gsStack_of_gsStacks_destroy(gsStack* self);
extern  void        gsStack_of_gsStacks_create_with_length_(gsStack* self,
                                                            size_t element_size,
                                                            size_t length);
extern  gsStack*    gsStack_of_gsStacks_create_with_length(size_t element_size,
                                                           size_t length);
extern  void        gsStack_of_gsStacks_pre_append_(gsStack* self,
                                                    size_t element_size);

#endif /* GS_HEADER */
