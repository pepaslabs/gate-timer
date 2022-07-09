// The Arduino preprocessor rearranges code, which sometimes moves function
// declarations above typedefs.  If those functions rely on those types,
// this will cause a compilation error.
// The work-around is to move typedefs out into a header file, to ensure
// they are defined before the relocated function declarations.

#include <stdint.h>

typedef int pin_t;  // An Arduino pin number.
typedef uint8_t digit_t;  // A 7-segment digit.
typedef uint32_t millis_t;  // Milliseconds as returned by millis().
