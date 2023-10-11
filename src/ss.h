#ifndef SS_HEADER
#define SS_HEADER

#include "standard.h"
#include "gs.h"

/*
 * "ss" stands for "subset".
 * ssSubset describes a subset of the set
 * [0, ..., containing_set_length-1].
 * Finding whether an element lies in the subset is O(1)
 * (but the storage cost of the subset is O(containing_set_length)
 * rather than O(the length of the subset)).
 */

/*-------------------------*/
/* macros                  */
/*-------------------------*/

/*-------------------------*/
/* types                   */
/*-------------------------*/

typedef struct ssSubset {
        /* the subset is of the set [0, ..., containing_set_length-1] */
        unsigned containing_set_length;
        gsStack elements_list;
        unsigned char* elements_bitmask;
}                       ssSubset;

/*-------------------------*/
/* functions               */
/*-------------------------*/

extern  unsigned        ssSubset_length(const ssSubset* self);
extern  unsigned        ssSubset_element(const ssSubset* self, unsigned index);
extern  boolean         ssSubset_is_nonempty(const ssSubset* self);
extern  boolean         ssSubset_is_in(const ssSubset* self, unsigned element);

extern  void            ssSubset_destroy_(ssSubset *self);
extern  void            ssSubset_destroy(ssSubset *self);
extern  void            ssSubset_create_(ssSubset *self,
					 unsigned containing_set_length);
extern  ssSubset*       ssSubset_create(unsigned containing_set_length);

/* considered not equal if containing_set_length differ */
extern  boolean         ssSubset_are_equal(const ssSubset* s1,
					   const ssSubset* s2);

extern  void            ssSubset_make_empty_(ssSubset* self);
extern  boolean         ssSubset_add_(ssSubset *self, unsigned element);

#endif /* SS_HEADER */
