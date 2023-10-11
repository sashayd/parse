#ifndef MA_TRACK

void ma_initialize(void) {
    return;
}

void ma_finalize(void) {
    return;
}

#else /* MA_TRACK */

#include "ma.h"

#include <stdio.h>
#include <stdlib.h>

#include "standard.h"
#include "./err.h"

static unsigned _ma_ptr_list_length;
static unsigned _ma_ptr_list_allocated;
static void** _ma_ptr_list_0;
static void** _ma_ptr_list_end;

#ifdef MA_DEBUG
static boolean _ma_initialized = false;
static char** _ma_debug_file_ptr_list;
static unsigned* _ma_debug_line_ptr_list;
#endif /* MA_DEBUG */

unsigned ma_number_of_allocations(void) {
#ifdef MA_DEBUG
    if (_ma_initialized == false) {
        printf("\n------------------------------------\n");
        printf("Error: ma_initialize was not called!\n");
        printf("------------------------------------\n");
        err_terminate();
        return 0;
    }
#endif /* MA_DEBUG */
    return _ma_ptr_list_length;
}

void ma_initialize(void) {
    _ma_ptr_list_allocated = MA_INITIAL_BLOCK_SIZE;
    _ma_ptr_list_0 =
        malloc(sizeof(*_ma_ptr_list_0) * _ma_ptr_list_allocated);
#ifdef MA_DEBUG
    _ma_initialized = true;
    _ma_debug_file_ptr_list =
        malloc(sizeof(*_ma_debug_file_ptr_list) * _ma_ptr_list_allocated);
    _ma_debug_line_ptr_list =
        malloc(sizeof(*_ma_debug_line_ptr_list) * _ma_ptr_list_allocated);
#endif /* MA_DEBUG */
    if (_ma_ptr_list_0 == NULL) {
        printf("\n--------------------------------------\n");
        printf("Error: ma_initialize failed to malloc!\n");
        printf("--------------------------------------\n");
        err_terminate();
        return;
    }
    _ma_ptr_list_length = 0;
    _ma_ptr_list_end = _ma_ptr_list_0;
    err_add_hook(ma_finalize);
    return;
}

void ma_finalize(void) {
#ifdef MA_DEBUG
    if (_ma_initialized == false) {
        printf("\n------------------------------------\n");
        printf("Error: ma_initialize was not called!\n");
        printf("------------------------------------\n");
        err_terminate();
        return;
    }
#endif /* MA_DEBUG */
    if (_ma_ptr_list_length == 0) {
        return;
    }
    printf("\n------------------------------------------------------\n");
    printf(
        "Warning: Upon finalization have %u non-freed pointers!\n",
        _ma_ptr_list_length
        );
    printf("------------------------------------------------------\n");
#ifdef MA_DEBUG
    char** file_ptr = _ma_debug_file_ptr_list + (_ma_ptr_list_length - 1);
    const unsigned* line_ptr =
        _ma_debug_line_ptr_list + (_ma_ptr_list_length - 1);
    for (unsigned i = 0; i < _ma_ptr_list_length; ++i) {
        printf(
            "| alloc occured in file %s line %u\n",
            *file_ptr--,
            *line_ptr--
            );
    }
    free(_ma_debug_file_ptr_list);
    free(_ma_debug_line_ptr_list);
#endif /* MA_DEBUG */
    for (void** p = _ma_ptr_list_end; p > _ma_ptr_list_0;) {
        free(*--p);
    }
    free(_ma_ptr_list_0);
    return;
}

static unsignedMaybe _ma_find_in_ptr_list(void* ptr) {
    if (_ma_ptr_list_length == 0) {
        return unsignedMaybe_from_false();
    }
    unsigned i = _ma_ptr_list_length - 1;
    for (void** p = _ma_ptr_list_end; p > _ma_ptr_list_0;) {
        if (ptr == *--p) {
            return unsignedMaybe_from_unsigned(i);
        }
        --i;
    }
    return unsignedMaybe_from_false();
}

#ifndef MA_DEBUG
static void _ma_add_to_ptr_list(void* ptr) {
#else /* MA_DEBUG */
static void _ma_add_to_ptr_list(void* ptr, char* file, unsigned line) {
#endif /* MA_DEBUG */
    if (_ma_ptr_list_length == _ma_ptr_list_allocated) {
	_ma_ptr_list_allocated += MA_ADDITIONAL_BLOCK_SIZE;
	void** const new_ptr =
	    realloc(_ma_ptr_list_0, _ma_ptr_list_allocated * sizeof(*new_ptr));
#ifdef MA_DEBUG
	_ma_debug_file_ptr_list =
	    realloc(_ma_debug_file_ptr_list,
		    _ma_ptr_list_allocated * sizeof(*_ma_debug_file_ptr_list));
	_ma_debug_line_ptr_list =
	    realloc(_ma_debug_line_ptr_list,
		    _ma_ptr_list_allocated * sizeof(*_ma_debug_line_ptr_list));
#endif /* MA_DEBUG */
	if (new_ptr == NULL) {
	    free(ptr);
	    printf("\n--------------------------------------------\n");
	    printf("Error: _ma_add_to_ptr_list failed to malloc!\n");
	    printf("--------------------------------------------\n");
	    err_terminate();
	    return;
	} else {
	    _ma_ptr_list_0 = new_ptr;
	    _ma_ptr_list_end = _ma_ptr_list_0 + _ma_ptr_list_length;
	}
    }
    ++_ma_ptr_list_length;
    ++_ma_ptr_list_end;
    *(_ma_ptr_list_end-1) = ptr;
#ifdef MA_DEBUG
    _ma_debug_file_ptr_list[_ma_ptr_list_length - 1] = file;
    _ma_debug_line_ptr_list[_ma_ptr_list_length - 1] = line;
#endif /* MA_DEBUG */
    return;
}

#ifndef MA_DEBUG
void* ma_malloc(size_t size) {
#else /* MA_DEBUG */
void* ma_malloc(size_t size, char* file, unsigned line) {
#endif /* MA_DEBUG */
    void* const ptr = malloc(size);
    if (ptr == NULL) {
	printf("\n--------------------------\n");
	printf("Error: malloc call failed!\n");
	printf("--------------------------\n");
	err_terminate();
	return NULL;
    }
#ifndef MA_DEBUG
    _ma_add_to_ptr_list(ptr);
#else /* MA_DEBUG */
    _ma_add_to_ptr_list(ptr, file, line);
#endif /* MA_DEBUG */
    return ptr;
}

#ifndef MA_DEBUG
void* ma_calloc(size_t num, size_t size) {
#else /* MA_DEBUG */
void* ma_calloc(size_t num, size_t size, char* file, unsigned line) {
#endif /* MA_DEBUG */
    void* const ptr = calloc(num, size);
    if (ptr == NULL) {
	printf("\n--------------------------\n");
	printf("Error: calloc call failed!\n");
	printf("--------------------------\n");
	err_terminate();
	return NULL;
    }
#ifndef MA_DEBUG
    _ma_add_to_ptr_list(ptr);
#else /* MA_DEBUG */
    _ma_add_to_ptr_list(ptr, file, line);
#endif /* MA_DEBUG */
    return ptr;
}

#ifndef MA_DEBUG
void* ma_realloc(void *ptr, size_t size) {
#else /* MA_DEBUG */
void* ma_realloc(void *ptr, size_t size, char* file, unsigned line) {
#endif /* MA_DEBUG */
    const unsignedMaybe index_maybe = _ma_find_in_ptr_list(ptr);
    if (unsignedMaybe_is(index_maybe) == false) {
	printf("\n--------------------------------------------\n");
	printf("Error: reallocating a non-allocated pointer!\n");
	printf("--------------------------------------------\n");
	err_terminate();
	return NULL;
    }
    unsigned index = unsignedMaybe_value(index_maybe);
    void* const new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
	printf("\n---------------------------\n");
	printf("Error: realloc call failed!\n");
	printf("---------------------------\n");
	err_terminate();
	return NULL;
    }
    _ma_ptr_list_0[index] = new_ptr;
#ifdef MA_DEBUG
    _ma_debug_file_ptr_list[index] = file;
    _ma_debug_line_ptr_list[index] = line;
#endif /* MA_DEBUG */
    return new_ptr;
}

#ifndef MA_DEBUG
static void _ma_remove_from_ptr_list(void *ptr) {
#else /* MA_DEBUG */
static void _ma_remove_from_ptr_list(void *ptr, char* file, unsigned line) {
#endif /* MA_DEBUG */
    const unsignedMaybe index_maybe = _ma_find_in_ptr_list(ptr);
    if (unsignedMaybe_is(index_maybe) == false) {
	printf("\n---------------------------------------\n");
	printf("Error: freeing a non-allocated pointer!\n");
	printf("---------------------------------------\n");
#ifdef MA_DEBUG
	printf("| in file %s line %u", file, line);
#endif /* MA_DEBUG */
	err_terminate();
	return;
    }
    const unsigned index = unsignedMaybe_value(index_maybe);
    if (index != _ma_ptr_list_length - 1) {
	_ma_ptr_list_0[index] = *(_ma_ptr_list_end-1);
#ifdef MA_DEBUG
	_ma_debug_file_ptr_list[index] =
	    _ma_debug_file_ptr_list[_ma_ptr_list_length - 1];
	_ma_debug_line_ptr_list[index] =
	    _ma_debug_line_ptr_list[_ma_ptr_list_length - 1];
#endif /* MA_DEBUG */
    }
    --_ma_ptr_list_length;
    --_ma_ptr_list_end;
    return;
}

#ifndef MA_DEBUG
void ma_free(void* ptr) {
#else /* MA_DEBUG */
void ma_free(void* ptr, char* file, unsigned line) {
#endif /* MA_DEBUG */
    if (ptr == NULL) {
	return;
    }
#ifndef MA_DEBUG
    _ma_remove_from_ptr_list(ptr);
#else /* MA_DEBUG */
    _ma_remove_from_ptr_list(ptr, file, line);
#endif /* MA_DEBUG */
    free(ptr);
    return;
}

#endif /* MA_TRACK */
