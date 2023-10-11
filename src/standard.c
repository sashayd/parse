#include "standard.h"

boolean unsignedMaybe_is(unsignedMaybe mu) {
    return mu.is;
}

unsigned unsignedMaybe_value(unsignedMaybe mu) {
    return mu.value;
}

unsignedMaybe unsignedMaybe_from_unsigned(unsigned u) {
    unsignedMaybe mu;
    mu.is = true;
    mu.value = u;
    return mu;
}

unsignedMaybe unsignedMaybe_from_false() {
    unsignedMaybe mu;
    mu.is = false;
    return mu;
}

/*

(another implementation of unsignedMaybe, stealing largest unsigned for maybe)

#include <limits.h>

boolean unsignedMaybe_is(unsignedMaybe mu) {
    return ((unsigned) mu == UINT_MAX ? false : true);
}

unsigned unsignedMaybe_value(unsignedMaybe mu) {
    return (unsigned) u;
}

unsignedMaybe unsignedMaybe_from_unsigned(unsigned u) {
    return (unsignedMaybe) u;
}

unsignedMaybe unsignedMaybe_from_false() {
    return (unsigned) UINT_MAX;
}

*/
