#ifndef CAF_EXTRACTOR_UTILS_H
#define CAF_EXTRACTOR_UTILS_H

#include <string>

namespace llvm {
class Function;
class Type;
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
 * @brief Determine whether the given function is a V8 API function.
 *
 * This function returns true if the given LLVM function match the following signature:
 *
 * `T (%"class.v8::FunctionCallbackInfo"*)`
 *
 * @param func
 * @return true
 * @return false
 */
bool IsV8ApiFunction(const llvm::Function& func);

/**
 * @brief Determine whether the given function is a constructor of some struct type.
 *
 * @param func the function.
 * @return true if the given function is a constructor of some struct type.
 * @return false if the given function is not a constructor.
 */
bool IsConstructor(const llvm::Function& func);

/**
 * @brief Get the type the given constructor is constructing.
 *
 * @param ctor the constructor.
 * @return llvm::Type* the type the given constructor is constructing.
 */
llvm::Type* GetConstructingType(llvm::Function* ctor);

} // namespace caf

#endif
