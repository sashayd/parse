#ifndef STANDARD_HEADER
#define STANDARD_HEADER

/*
 * types boolean and unsignedMaybe
 */

/*-------------------------*/
/* macros                  */
/*-------------------------*/

/* put next to function parameter which was unused in the body, to remove
   unused parameter warning */
#define UNUSED __attribute__ ((unused))

/*-------------------------*/
/* types                   */
/*-------------------------*/

typedef enum {false, true}  boolean;

typedef struct unsignedMaybe {
    boolean is;
    unsigned value;
}                           unsignedMaybe;

/*

  (another implementation of unsignedMaybe,
  stealing largest unsigned for maybe)

  typedef unsigned maybe_unsigned;

*/

/*-------------------------*/
/* functions               */
/*-------------------------*/

extern  boolean         unsignedMaybe_is(unsignedMaybe mu);
extern  unsigned        unsignedMaybe_value(unsignedMaybe mu);
extern  unsignedMaybe   unsignedMaybe_from_unsigned(unsigned u);
extern  unsignedMaybe   unsignedMaybe_from_false();

#endif /* STANDARD_HEADER */
