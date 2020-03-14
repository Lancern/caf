#ifndef CAF_INTRINSIC_H
#define CAF_INTRINSIC_H

#include <cstdio>
#include <cstdlib>

#define CAF_UNREACHABLE  \
    do { fprintf(stderr, "Unreachable code: %s:%d\n", __FILE__, __LINE__); abort(); \
      __builtin_unreachable(); } while (false)

#define CAF_NO_OP \
    do { } while (0)

#endif
