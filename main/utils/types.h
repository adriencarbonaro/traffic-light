
#ifndef TYPES_H_
#define TYPES_H_

#include "stdint.h"
#include "stdbool.h"

#define TYPEDEF(x) typedef int##x##_t int##x
#define TYPEDEF_U(x) typedef uint##x##_t uint##x

TYPEDEF(32);

TYPEDEF_U(8);
TYPEDEF_U(16);
TYPEDEF_U(32);

#endif /* TYPES_H_ */
