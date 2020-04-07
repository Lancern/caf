#ifndef CAF_TARGETS_DIAGNOSTICS_H
#define CAF_TARGETS_DIAGNOSTICS_H

#include <cstdio>
#include <cstdlib>

#define PRINT_ERR_AND_EXIT(msg) \
  do { std::fprintf(stderr, msg); std::exit(1); } while (false)

#define PRINT_ERR_AND_EXIT_FMT(msg, ...) \
  do { std::fprintf(stderr, msg, __VA_ARGS__); std::exit(1); } while (false)

#endif
