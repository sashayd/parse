#include "gs.h"

#include <string.h>

#include "standard.h"
#include "ma.h"

size_t gsStack_length(const gsStack* self) {
    return self->length;
}

void* gsStack_0(const gsStack* self) {
    return self->first;
}

void* gsStack_end(const gsStack* self) {
    return self->end;
}

void* gsStack_last(const gsStack* self) {
    return (unsigned char*) self->end - self->element_size;
}

void* gsStack_element(const gsStack* self, size_t index) {
    return (unsigned char*) self->first + self->element_size * index;
}

boolean gsStack_is_nonempty(const gsStack* self) {
    return (self->length != 0 ? true : false);
}

size_t gsStack_element_index(const gsStack* self, const void* element,
                             gsGenericComparisonFn f) {
    const size_t length = gsStack_length(self);
    const unsigned char* pointer = self->end;
    for (size_t ii = 1; ii <= length; ++ii) {
	pointer -= self->element_size;
        if (f(pointer, element) == true) {
            return length-ii;
        }
    }
    return length;
}

void gsStack_destroy_(gsStack* self) {
    if (self == NULL) {
        return;
    }
    FREE(self->first);
    return;
}

void gsStack_destroy(gsStack* self) {
    if (self == NULL) {
        return;
    }
    gsStack_destroy_(self);
    FREE(self);
    return;
}

void gsStack_create_(gsStack* self, size_t element_size) {
    self->element_size = element_size;
    self->length = 0;
    self->allocated_length = 0;
    self->first = NULL;
    self->end = NULL;
    return;
}

gsStack* gsStack_create(size_t element_size) {
    gsStack* const self = MALLOC(sizeof(*self));
    gsStack_create_(self, element_size);
    return self;
}

static void _gsStack_recalculate_end(gsStack* self) {
    if (self->allocated_length != 0) {
        self->end = gsStack_element(self, self->length);
    }
    return;
}

void gsStack_create_with_length_(gsStack* self, size_t element_size,
                                 size_t length) {
    gsStack_create_(self, element_size);
    if (length != 0) {
        self->length = length;
        self->allocated_length = self->length;
        self->first = MALLOC(self->allocated_length * self->element_size);
        _gsStack_recalculate_end(self);
    }
    return;
}

gsStack* gsStack_create_with_length(size_t element_size, size_t length) {
    gsStack* const self = gsStack_create(element_size);
    gsStack_create_with_length_(self, element_size, length);
    return self;
}

void gsStack_create_zeros_(gsStack *self, size_t element_size, size_t length) {
    gsStack_create_(self, element_size);
    if (length != 0) {
        self->length = length;
        self->allocated_length = self->length;
        self->first = CALLOC(self->allocated_length, self->element_size);
        _gsStack_recalculate_end(self);
    }
    return;
}

gsStack* gsStack_create_zeros(size_t element_size, size_t length) {
    gsStack* const self = gsStack_create(element_size);
    gsStack_create_zeros_(self, element_size, length);
    return self;
}

void gsStack_copy_(gsStack* self, const gsStack* l) {
    self->element_size = l->element_size;
    self->length = l->length;
    self->allocated_length = l->length;
    if (self->length == 0) {
        self->first = NULL;
        self->end = NULL;
    } else {
        self->first = MALLOC(self->allocated_length * self->element_size);
        memcpy(self->first, l->first, self->length * self->element_size);
        _gsStack_recalculate_end(self);
    }
    return;
}

gsStack* gsStack_copy(const gsStack* l) {
    gsStack* const self = MALLOC(sizeof(*self));
    gsStack_copy_(self, l);
    return self;
}

void gsStack_pre_append_(gsStack* self) {
    if (self->allocated_length == 0) {
        self->allocated_length = 1 + GS_MINIMAL_ALLOCATION / self->element_size;
        self->first = MALLOC(self->allocated_length * self->element_size);
        ++self->length;
        self->end = (unsigned char*) self->first + self->element_size;
    } else if (self->length == self->allocated_length) {
        self->allocated_length = 2 * self->allocated_length;
        self->first =
            REALLOC(self->first, self->allocated_length * self->element_size);
        ++self->length;
        _gsStack_recalculate_end(self);
    } else {
        ++self->length;
        self->end = (unsigned char*) self->end + self->element_size;
    }
    return;
}

void gsStack_pre_append_several_(gsStack* self, size_t how_many) {
    if (how_many == 0) {
        return;
    }
    self->length += how_many;
    if (self->length > self->allocated_length) {
        if (self->allocated_length == 0) {
            self->allocated_length =
                1 + GS_MINIMAL_ALLOCATION / self->element_size;
            if (self->allocated_length < self->length) {
                self->allocated_length = self->length;
            }
            self->first = MALLOC(self->allocated_length * self->element_size);
        } else {
            self->allocated_length = 2 * self->allocated_length;
            if (self->allocated_length < self->length) {
                self->allocated_length = self->length;
            }
            self->first = REALLOC(self->first,
                                  self->allocated_length * self->element_size);
        }
    }
    _gsStack_recalculate_end(self);
    return;
}

void gsStack_post_pop_(gsStack* self) {
    --self->length;
    if (self->length > GS_MINIMAL_ALLOCATION / self->element_size
        && self->length < self->allocated_length / GS_DEALLOCATION_FACTOR) {
        self->allocated_length = self->length;
        self->first = REALLOC(self->first,
                              self->allocated_length * self->element_size);
        _gsStack_recalculate_end(self);
    } else {
        self->end = (unsigned char*) self->end - self->element_size;
    }
    return;
}

void gsStack_post_pop_several_(gsStack* self, size_t how_many) {
    if (self->allocated_length == 0) {
        return;
    }
    if (how_many >= self->length) {
        how_many = self->length;
    }
    self->length -= how_many;
    if (self->length > GS_MINIMAL_ALLOCATION / self->element_size
        && self->length < self->allocated_length / GS_DEALLOCATION_FACTOR) {
        self->allocated_length = self->length;
        self->first = REALLOC(self->first,
                              self->allocated_length * self->element_size);
    }
    _gsStack_recalculate_end(self);
    return;
}

void gsStack_make_empty_(gsStack* self) {
    if (self->length != 0) {
        self->length = 0;
        self->end = self->first;
    }
    return;
}

void gsStack_make_room_(gsStack* self, size_t length) {
    if (self->allocated_length >= self->length + length) {
        return;
    }
    if (self->allocated_length == 0) {
        self->allocated_length = 1 + GS_MINIMAL_ALLOCATION / self->element_size;
        if (self->allocated_length < self->length + length) {
            self->allocated_length = self->length + length;
        }
        self->first = MALLOC(self->allocated_length * self->element_size);
    } else {
        self->allocated_length = 2 * self->allocated_length;
        if (self->allocated_length < self->length + length) {
            self->allocated_length = self->length + length;
        }
        self->first = REALLOC(self->first,
                              self->allocated_length * self->element_size);
    }
    _gsStack_recalculate_end(self);
    return;
}

void gsStack_concat_(gsStack *self, const gsStack *l1, const gsStack *l2) {
    self->element_size = l1->element_size;
    self->length = l1->length + l2->length;
    self->allocated_length = self->length;
    if (self->allocated_length == 0) {
        self->first = NULL;
        self->end = NULL;
    } else {
        self->first = MALLOC(self->allocated_length * self->element_size);
        _gsStack_recalculate_end(self);
        memcpy(self->first, l1->first, l1->length * self->element_size);
        memcpy(gsStack_element(self, l1->length), l2->first,
               l2->length * self->element_size);
    }
    return;
}

gsStack* gsStack_concat(const gsStack *l1, const gsStack *l2) {
    gsStack* const self = gsStack_create(l1->element_size);
    gsStack_concat_(self, l1, l2);
    return self;
}

void gsStack_of_gsStacks_destroy_(gsStack* self) {
    if (self == NULL || self->allocated_length == 0) {
        return;
    }
    for (gsStack* l = self->first; l < (gsStack*) self->end; ++l) {
	gsStack_destroy_(l);
    }
    gsStack_destroy_(self);
    return;
}

void gsStack_of_gsStacks_destroy(gsStack* self) {
    if (self == NULL || self->allocated_length == 0) {
        return;
    }
    for (gsStack* l = self->first; l < (gsStack*) self->end; ++l) {
	gsStack_destroy_(l);
    }
    gsStack_destroy(self);
    return;
}

void gsStack_of_gsStacks_create_with_length_(gsStack* self, size_t element_size,
					     size_t length) {
    gsStack_create_with_length_(self, sizeof(gsStack), length);
    if (self->allocated_length == 0) {
	return;
    }
    for (gsStack* l = self->first; l < (gsStack*) self->end; ++l) {
	gsStack_create_(l, element_size);
    }
    return;
}

gsStack* gsStack_of_gsStacks_create_with_length(size_t element_size,
                                                size_t length) {
    gsStack* const self = gsStack_create(sizeof(gsStack));
    gsStack_of_gsStacks_create_with_length_(self, element_size, length);
    return self;
}

void gsStack_of_gsStacks_pre_append_(gsStack* self, size_t element_size) {
    gsStack_pre_append_(self);
    gsStack_create_(gsStack_last(self), element_size);
    return;
}
