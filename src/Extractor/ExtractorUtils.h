#ifndef CAF_EXTRACTOR_UTILS_H
#define CAF_EXTRACTOR_UTILS_H

#include <string>

namespace llvm {
class Function;
} // namespace llvm

namespace caf {

/**
 * @brief Determine whether the given function is a CAF target API function.
 *
 * @param func the function.
 * @return true if the given function is a CAF target API function.
 * @return false if the given function is not a CAF target API function.
 */
bool IsApiFunction(const llvm::Function& func);

/**
 * @brief Determine whether the given function is a constructor of some struct type.
 *
 * @param func the function.
 * @return true if the given function is a constructor of some struct type.
 * @return false if the given function is not a constructor.
 */
bool IsConstructor(const llvm::Function& func);

/**
 * @brief Get the name of the struct type that can be constructed from the given constructor
 * function.
 *
 * If the given function is not a constructor, the behavior is undefined.
 *
 * @param ctor the constructor function.
 * @return std::string the name of the constructed type by the give constructor.
 */
std::string GetConstructingTypeName(const llvm::Function& ctor);

} // namespace caf

#endif
