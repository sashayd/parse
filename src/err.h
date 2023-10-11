#ifndef ERR_HEADER
#define ERR_HEADER

/*
 * "err" stands for "error".
 * Intended to be a simple generic choice when all failures are terminal.
 */

/*-------------------------*/
/* Macros                  */
/*-------------------------*/

#define ERR_MAX_NUM_OF_HOOKS 10

/*-------------------------*/
/* types                   */
/*-------------------------*/

typedef void    (*errHook)(void);

/*-------------------------*/
/* functions               */
/*-------------------------*/

extern  void    err_add_hook(errHook hook);
extern  void    err_terminate(void);

#endif /* ERR_HEADER */
