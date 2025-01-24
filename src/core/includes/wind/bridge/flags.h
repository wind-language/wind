#include <stdint.h>

#ifndef BRIDGE_FLAGS
#define BRIDGE_FLAGS

#define PURE_STACK  (1 << 0)
#define PURE_ABI    (1 << 1)
#define PURE_NOABI  (1 << 2)
#define PURE_EXPR   (1 << 3)
#define PURE_LOGUE  (1 << 4)
#define PURE_STCHK  (1 << 5)
#define FN_EXTERN   (1 << 6)
#define FN_VARIADIC (1 << 7)
#define FN_PUBLIC   (1 << 8)
#define FN_NOMANGLE (1 << 9)
#define FN_ARGPUSH  (1 << 10)

typedef uint16_t FnFlags;

#endif