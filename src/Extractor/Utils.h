#ifndef CAF_EXTRACTOR_UTILS_H
#define CAF_EXTRACTOR_UTILS_H

namespace llvm {
class Function;
} // namespace llvm

namespace caf {

/**
 * @brief Determine whether the given function is an API function.
 *
 * @param func the function.
 * @return true if the given function is an API function.
 * @return false if the given function is not an API function.
 */
bool IsApiFunction(const llvm::Function* func);

} // namespace caf

#endif
