#ifndef CAF_FUNCTION_DATABASE_H
#define CAF_FUNCTION_DATABASE_H

#include "Infrastructure/Memory.h"
#include "Infrastructure/Optional.h"
#include "Basic/CAFStore.h"
#include "Targets/Common/Diagnostics.h"
#include "Targets/Common/PropertyResolver.h"

#include <cassert>
#include <memory>
#include <unordered_map>
#include <utility>

namespace caf {

/**
 * @brief Provide a mapping from function ID to function objects.
 *
 * @tparam TargetTraits the trait type describing target's type system.
 */
template <typename TargetTraits>
class FunctionDatabase {
public:
  using ValueType = typename TargetTraits::ValueType;

  /**
   * @brief Construct a new FunctionDatabase object.
   *
   * @param resolver the property resolver.
   * @param global the global object.
   */
  explicit FunctionDatabase(PropertyResolver<TargetTraits>& resolver, ValueType global)
    : _resolver(resolver),
      _global(global),
      _root(caf::make_unique<TrieNode>(global)),
      _funcIdToNode()
  { }

  FunctionDatabase(const FunctionDatabase<TargetTraits> &) = delete;
  FunctionDatabase(FunctionDatabase<TargetTraits> &&) noexcept = default;

  FunctionDatabase<TargetTraits>& operator=(const FunctionDatabase<TargetTraits> &) = delete;
  FunctionDatabase<TargetTraits>& operator=(FunctionDatabase<TargetTraits> &&) = default;

  /**
   * @brief Associate the given function ID with the function whose name is `name`.
   *
   * @param funcId the function ID.
   * @param name name of the function.
   */
  void AddFunction(uint32_t funcId, const std::string& name) {
    auto node = GetNode(name);
    if (!node) {
      PRINT_ERR_AND_EXIT_FMT("funcdb: Cannot find function \"%s\"\n", name.c_str());
    }

    _funcIdToNode[funcId] = node;
  }

  /**
   * @brief Add all functions contained in the given CAF metadata database to this function
   * database.
   *
   * @param store the CAF metadata store.
   */
  void Populate(const CAFStore& store) {
    for (const auto& fn : store) {
      AddFunction(fn.id(), fn.name());
    }
  }

  /**
   * @brief Get the function with the given function ID.
   *
   * @param funcId the function ID.
   * @return Optional<ValueType> the function object.
   */
  Optional<ValueType> GetFunction(uint32_t funcId) const {
    auto i = _funcIdToNode.find(funcId);
    if (i == _funcIdToNode.end()) {
      return Optional<ValueType> { };
    }

    auto node = i->second;
    return Optional<ValueType> { node->value() };
  }

private:
  class TrieNode {
  public:
    explicit TrieNode(ValueType value)
      : _value(value), _children()
    { }

    TrieNode(const TrieNode &) = delete;
    TrieNode(TrieNode &&) noexcept = default;

    TrieNode& operator=(const TrieNode &) = delete;
    TrieNode& operator=(TrieNode &&) = default;

    ValueType value() const { return _value; }

    bool HasChild(const std::string& name) const {
      return _children.find(name) != _children.end();
    }

    TrieNode* GetChild(const std::string& name) const {
      auto i = _children.find(name);
      if (i == _children.end()) {
        return nullptr;
      }
      return i->second.get();
    }

    TrieNode* AddChild(const std::string& name, ValueType value) {
      auto ptr = caf::make_unique<TrieNode>(value);
      auto ret = ptr.get();
      _children[name] = std::move(ptr);
      return ret;
    }

  private:
    ValueType _value;
    std::unordered_map<std::string, std::unique_ptr<TrieNode>> _children;
  };

  PropertyResolver<TargetTraits>& _resolver;
  ValueType _global;
  std::unique_ptr<TrieNode> _root;
  std::unordered_map<uint32_t, TrieNode *> _funcIdToNode;

  TrieNode* GetNode(const std::string& name) const {
    auto ptr = _root.get();
    size_t start = 0;
    while (start < name.length()) {
      auto sepPos = name.find('.', start);
      if (sepPos == std::string::npos) {
        sepPos = name.length();
      }
      auto component = name.substr(start, sepPos - start);
      start = sepPos + 1;

      if (!ptr->HasChild(component)) {
        // Resolve the property.
        auto prop = _resolver.Resolve(ptr->value(), component);
        if (!prop) {
          // No such property.
          return nullptr;
        }
        auto propValue = prop.take();
        ptr = ptr->AddChild(component, propValue);
      } else {
        ptr = ptr->GetChild(component);
      }
    }
    return ptr;
  }
}; // class FunctionDatabase

} // namespace caf

#endif
