#ifndef MA_HEADER
#define MA_HEADER

#include <stddef.h>

/*
 * "ma" stands for "memory allocation".
 * Intended to be a simple generic choice for debugging memory leaks etc.,
 * when allocation optimization is not sought-for, and allocation
 * failures are terminal.
 * Search for pointers in the allocated-pointers list is naive (linear) and
 * stacky (starting with last inserted),
 * so is faster when allocations are freed in a stacky-ish manner.
 * 
 * Use MALLOC, CALLOC, REALLOC, FREE macros. If MA_TRACK is not defined,
 * those just revert to usual functions. If, additionally to MA_TRACK,
 * MA_DEBUG is defined, more information is registered, regarding the
 * file and line on which allocations occured.
 */

/*-------------------------*/
/* macros                  */
/*-------------------------*/

#ifndef MA_TRACK

#include <stdlib.h>

#define MALLOC(X) malloc(X)
#define CALLOC(X, Y) calloc(X, Y)
#define REALLOC(X, Y) realloc(X, Y)
#define FREE(X) free(X)

#else /* MA_TRACK */

#define MA_INITIAL_BLOCK_SIZE 1024
#define MA_ADDITIONAL_BLOCK_SIZE 1024

#ifndef MA_DEBUG

#define MALLOC(X) ma_malloc(X)
#define CALLOC(X, Y) ma_calloc(X, Y)
#define REALLOC(X, Y) ma_realloc(X, Y)
#define FREE(X) ma_free(X)

#else /* MA_DEBUG */

#define MALLOC(X) ma_malloc(X, __FILE__, __LINE__)
#define CALLOC(X, Y) ma_calloc(X, Y, __FILE__, __LINE__)
#define REALLOC(X, Y) ma_realloc(X, Y, __FILE__, __LINE__)
#define FREE(X) ma_free(X, __FILE__, __LINE__)

#endif /* MA_DEBUG */

#endif /* MA_TRACK */

/*-------------------------*/
/* types                   */
/*-------------------------*/

/*-------------------------*/
/* functions               */
/*-------------------------*/

extern  void        ma_initialize(void);
extern  void        ma_finalize(void);

#ifdef MA_TRACK

extern  unsigned    ma_number_of_allocations(void);

#ifndef MA_DEBUG

extern  void*       ma_malloc(size_t size);
extern  void*       ma_calloc(size_t num, size_t size);
extern  void*       ma_realloc(void *ptr, size_t size);
extern  void        ma_free(void* ptr);

#else /* MA_DEBUG */

extern  void*       ma_malloc(size_t size, char* file, unsigned line);
extern  void*       ma_calloc(size_t num, size_t size, char* file,
                              unsigned line);
extern  void*       ma_realloc(void *ptr, size_t size, char* file,
                               unsigned line);
extern  void        ma_free(void* ptr, char* file, unsigned line);

#endif /* MA_DEBUG */

#endif /* MA_TRACK */

#endif /* MA_HEADER */
