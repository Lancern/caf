#ifndef CAF_EXTRACTOR_CONTEXT_H
#define CAF_EXTRACTOR_CONTEXT_H

#include "Basic/Function.h"

#include <memory>
#include <vector>

namespace llvm {
class Function;
} // namespace llvm

namespace caf {

class CAFStore;

/**
 * @brief Provide context for the extractor.
 *
 */
class ExtractorContext {
public:
  /**
   * @brief Construct a new ExtractorContext object.
   *
   */
  explicit ExtractorContext() = default;

  ExtractorContext(const ExtractorContext &) = delete;
  ExtractorContext(ExtractorContext &&) noexcept = default;

  ExtractorContext& operator=(const ExtractorContext &) = delete;
  ExtractorContext& operator=(ExtractorContext &&) = default;

  /**
   * @brief Add an API function to this extractor context.
   *
   * @param function
   */
  void AddFunction(llvm::Function* function);

  /**
   * @brief Get all API functions.
   *
   * @return const std::vector<llvm::Function *>& a list of all API functions.
   */
  const std::vector<llvm::Function *>& funcs() const { return _funcs; }

  /**
   * @brief Get the number of API functions.
   *
   * @return size_t the number of API functions.
   */
  size_t GetFunctionsCount() const { return _funcs.size(); }

  /**
   * @brief Get the API function with the given ID.
   *
   * @param id ID of the API function.
   * @return llvm::Function* the API function.
   */
  llvm::Function* GetFunction(FunctionIdType id) const { return _funcs.at(id); }

  /**
   * @brief Create a CAFStore object containing metadata included in this extractor context.
   *
   * @return std::unique_ptr<CAFStore> a CAFStore object containing metadata included in this
   * extractor context.
   */
  std::unique_ptr<CAFStore> CreateStore() const;

private:
  std::vector<llvm::Function *> _funcs;
}; // class ExtractorContext

} // namespace caf

#endif
