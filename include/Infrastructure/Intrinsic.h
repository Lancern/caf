#ifndef CAF_INTRINSIC_H
#define CAF_INTRINSIC_H

#include <cstdio>
#include <cstdlib>

#define CAF_UNREACHABLE  \
    { fprintf(stderr, "Unreachable code: %s:%d\n", __FILE__, __LINE__); abort(); \
      __builtin_unreachable(); }

#endif
