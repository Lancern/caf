#include "Infrastructure/Casting.h"
#include "ExtractorUtils.h"

#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"

#include <cxxabi.h>

namespace caf {

namespace {

std::string demangle(const std::string& name) {
  auto demangled = abi::__cxa_demangle(name.c_str(), nullptr, nullptr, nullptr);
  if (!demangled) {
    // Demangling failed. Return the original name.
    return name;
  }

  std::string ret(demangled);
  free(demangled);

  return ret;
}

/**
 * @brief Removes template arguments and function arguments specified in the given symbol name.
 *
 * Formally, this function returns any contents that is surrounded by angle brackets or round
 * brackets in some suffix of the given string.
 *
 * @param name the original symbol name.
 * @return std::string the symbol name with template arguments and function arguments removed.
 */
std::string RemoveArgs(const std::string& name) {
  if (name.empty()) {
    return std::string { };
  }

  if (name.back() != ')' && name.back() != '>') {
    return name;
  }

  auto counter = 0;
  for (auto i = name.rbegin(); i != name.rend(); ++i) {
    if (*i == ')' || *i == '>') {
      ++counter;
    } else if (*i == '(' || *i == '<') {
      --counter;
    }

    if (!counter) {
      return name.substr(0, std::distance(i, name.rend()) - 1);
    }
  }

  // Rarely should here be reached. Return the original name of here is actually
  // reached.
  return name;
}

/**
 * @brief Represent a qualified name.
 *
 * A qualified name is a symbol name whose components are separated by "::".
 *
 */
class QualifiedName {
public:
  /**
   * @brief Construct a new QualifiedName object.
   *
   * @param name the original qualified name.
   */
  explicit QualifiedName(const std::string& name)
    : _components { } {
    load(name);
  }

  using iterator = typename std::vector<std::string>::const_iterator;
  using size_type = typename std::vector<std::string>::size_type;

  iterator begin() const {
    return _components.begin();
  }

  iterator end() const {
    return _components.end();
  }

  /**
   * @brief Get the number of components contained in the fully qualified name.
   *
   * @return size_type the number of components.
   */
  size_type size() const {
    return _components.size();
  }

  /**
   * @brief Get the component at the given index. The index can be negative in which case the index
   * will counts from the back of the container.
   *
   * @param index the index.
   * @return const std::string& the component at the given index.
   */
  const std::string& operator[](int index) const {
    if (index >= 0) {
      return _components[index];
    } else {
      return _components[_components.size() + index];
    }
  }

private:
  std::vector<std::string> _components;

  /**
   * @brief Load the given qualified symbol name and split all components, with regard to the
   * necessary bracket structure.
   *
   * @param name the qualified symbol name.
   */
  void load(const std::string& name) {
    for (auto left = 0; left < static_cast<int>(name.length()); ) {
      auto bracketDepth = 0;
      auto right = left;
      while (right < static_cast<int>(name.length())) {
        if (right + 1 < static_cast<int>(name.length()) &&
            bracketDepth == 0 &&
            name[right] == ':' && name[right + 1] == ':') {
          // Split point hit.
          break;
        }

        auto curr = name[right++];
        if (curr == '(' || curr == '<' || curr == '[' || curr == '{') {
          ++bracketDepth;
        } else if (curr == ')' || curr == '>' || curr == ']' || curr == ')') {
          --bracketDepth;
        }
      }

      _components.emplace_back(name.substr(left, right - left));
      left = right + 2; // Skip the "::" separator.
    }
  }
};

} // namespace <anonymous>

bool IsApiFunction(const llvm::Function& func) {
  if (func.isDeclaration()) {
    return false;
  }

  return func.hasFnAttribute(llvm::Attribute::CafApi);
}

bool IsV8ApiFunction(const llvm::Function& func) {
  if (func.isDeclaration()) {
    return false;
  }

  auto funcType = func.getFunctionType();
  if (funcType->getNumParams() != 1) {
    return false;
  }

  auto paramType = funcType->getParamType(0);
  if (!paramType->isPointerTy()) {
    return false;
  }

  auto pointeeType = paramType->getPointerElementType();
  if (!pointeeType->isStructTy()) {
    return false;
  }

  auto structType = caf::dyn_cast<llvm::StructType>(pointeeType);
  if (!structType->hasName()) {
    return false;
  }

  return pointeeType->getStructName() == "class.v8::FunctionCallbackInfo";
}

bool IsConstructor(const llvm::Function& func) {
  auto name = RemoveArgs(demangle(func.getName()));
  return name.size() >= 2 && name[-1] == name[-2];
}

std::string GetConstructingTypeName(const llvm::Function& ctor) {
  return RemoveArgs(demangle(ctor.getName()));
}

} // namespace caf
