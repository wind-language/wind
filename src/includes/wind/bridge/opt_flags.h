#include <stdint.h>

#ifndef OPT_FLAGS
#define OPT_FLAGS

#define PURE_STACK (1 << 0)
#define PURE_ABI   (1 << 1)
#define PURE_NOABI (1 << 2)
#define PURE_EXPR  (1 << 3)
#define PURE_LOGUE (1 << 4)

typedef uint16_t FnFlags;

#endif