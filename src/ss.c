#include "./ss.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h> /* for CHAR_BIT */

#include "standard.h"
#include "ma.h"
#include "gs.h"

unsigned ssSubset_length(const ssSubset* self) {
    return gsStack_length(&self->elements_list);
}

unsigned ssSubset_element(const ssSubset* self, unsigned index) {
    return * (unsigned*) gsStack_element(&self->elements_list, index);
}

boolean ssSubset_is_nonempty(const ssSubset* self) {
    return (ssSubset_length(self) != 0 ? true : false);
}

boolean ssSubset_is_in(const ssSubset* self, unsigned element) {
    return (self->elements_bitmask[element / CHAR_BIT]
	    & (1<<(element % CHAR_BIT)) ? true : false);
}

void ssSubset_destroy_(ssSubset *self) {
    if (self == NULL) {
	return;
    }
    gsStack_destroy_(&self->elements_list);
    FREE(self->elements_bitmask);
    return;
}

void ssSubset_destroy(ssSubset* self) {
    if (self == NULL) {
	return;
    }
    ssSubset_destroy_(self);
    FREE(self);
    return;
}

void ssSubset_create_(ssSubset* self, unsigned containing_set_length) {
    self->containing_set_length = containing_set_length;
    gsStack_create_(&self->elements_list, sizeof(unsigned));
    const unsigned bitmask_size_to_allocate =
	containing_set_length / CHAR_BIT + 1;
    self->elements_bitmask =
	CALLOC(bitmask_size_to_allocate, sizeof(*self->elements_bitmask));
    return;
}

ssSubset* ssSubset_create(unsigned containing_set_length) {
    ssSubset* self = MALLOC(sizeof(*self));
    ssSubset_create_(self, containing_set_length);
    return self;
}

boolean ssSubset_are_equal(const ssSubset *s1, const ssSubset *s2) {
    if (s1->containing_set_length != s2->containing_set_length) {
	return false;
    }
    const unsigned bitmask_size = s1->containing_set_length / CHAR_BIT + 1;
    return (memcmp(s1->elements_bitmask, s2->elements_bitmask, bitmask_size)
	    == 0 ? true : false);
}

void ssSubset_make_empty_(ssSubset* self) {
    gsStack_make_empty_(&self->elements_list);
    for (unsigned i = 0; i < self->containing_set_length / CHAR_BIT + 1; ++i) {
	self->elements_bitmask[i] = 0;
    }
    return;
}

boolean ssSubset_add_(ssSubset* self, unsigned element) {
    if (ssSubset_is_in(self, element) == true) {
	return true;
    }
    GS_APPEND(&(self->elements_list), element, unsigned);
    self->elements_bitmask[element / CHAR_BIT] =
	self->elements_bitmask[element / CHAR_BIT] | (1<<(element % CHAR_BIT));
    return false;
}
