#include "./err.h"

#include <stdlib.h>

static unsigned _err_num_of_hooks = 0;
static errHook _err_hooks[ERR_MAX_NUM_OF_HOOKS];

void err_add_hook(errHook hook) {
    if (_err_num_of_hooks == ERR_MAX_NUM_OF_HOOKS) {
        err_terminate();
        return;
    }
    _err_hooks[_err_num_of_hooks] = hook;
    ++_err_num_of_hooks;
}

void err_terminate(void) {
    for (unsigned i  = 0; i < _err_num_of_hooks; ++i) {
        _err_hooks[_err_num_of_hooks-1-i]();
    }
    exit(1);
}
