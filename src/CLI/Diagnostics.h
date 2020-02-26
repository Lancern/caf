#ifndef CAF_DIAGNOSTICS_H
#define CAF_DIAGNOSTICS_H

#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#define PRINT_ERR_AND_EXIT(msg) \
    do { std::fprintf(stderr, msg "\n"); std::exit(1); } while (0)

#define PRINT_ERR_AND_EXIT_FMT(format, ...) \
    do { std::fprintf(stderr, format "\n", __VA_ARGS__); std::exit(1); } while (0)

#define PRINT_OS_ERR_AND_EXIT(err_code, msg) \
    PRINT_ERR_AND_EXIT_FMT(msg ": %s (%d)\n", std::strerror(err_code), err_code)

#define PRINT_OS_ERR_AND_EXIT_FMT(err_code, format, ...) \
    PRINT_ERR_AND_EXIT_FMT(format ": %s (%d)\n", __VA_ARGS__, std::strerror(err_code), err_code)

#define PRINT_LAST_OS_ERR_AND_EXIT(msg) \
    PRINT_OS_ERR_AND_EXIT(errno, msg)

#define PRINT_LAST_OS_ERR_AND_EXIT_FMT(format, ...) \
    PRINT_OS_ERR_AND_EXIT_FMT(errno, format, __VA_ARGS__)

#endif
