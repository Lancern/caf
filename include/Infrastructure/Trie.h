#ifndef CAF_TRIE_H
#define CAF_TRIE_H

#include "Infrastructure/Optional.h"
#include "Infrastructure/Memory.h"

#include <functional>
#include <memory>
#include <utility>
#include <unordered_map>
#include <type_traits>

namespace caf {

namespace details {

template <typename Key,
          typename T,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T>>>
class TrieNode {
public:
  /**
   * @brief Get the value contained on this node.
   *
   * @return Optional<T>& the value contained on this node.
   */
  Optional<T>& value() { return _value; }

  /**
   * @brief Get the value contained on this node.
   *
   * @return const Optional<T>& the value contained on this node.
   */
  const Optional<T>& value() const { return _value; }

  /**
   * @brief Set the value contained on this node.
   *
   * @param value the value contaiend on this node.
   */
  void SetValue(T value) {
    _value.set(std::move(value));
  }

  /**
   * @brief Get the child node corresponding to the given key. If the required child node does not
   * exist, create a new child node associated with the given key and returns the new child node.
   *
   * @param key the key of the child node.
   * @return NodeType* pointer to the child node.
   */
  NodeType* GetOrAddChild(const Key& key) {
    auto i = _children.find(key);
    if (i == _children.end()) {
      auto entry = std::make_pair(key, std::make_unique<NodeType>());
      auto child = entry.second.get();
      _children.insert(std::move(entry));
      return child;
    }

    return i->second.get();
  }

  /**
   * @brief Get the child node corresponding to the given key. If the required child node does not
   * exist, returns nullptr.
   *
   * @param key the key of the child node.
   * @return NodeType* pointer to the child node, or nullptr if the required child node does not
   * exist.
   */
  NodeType* GetChild(const Key& key) const {
    auto i = _children.find(key);
    if (i == _children.end()) {
      return nullptr;
    }
    return i->second.get();
  }

private:
  using NodeType = TrieNode<Key, T, Hash, KeyEqual, Allocator>;

  Optional<T> _value;
  std::unordered_map<Key, std::unique_ptr<NodeType>,
                     Hash, KeyEqual, Allocator> _children;
}; // namespace TrieNode

} // namespace details

/**
 * @brief A trie.
 *
 * @tparam Key type of the key.
 * @tparam T type of the value.
 * @tparam Hash type of the key type's hasher.
 * @tparam std::equal_to<Key> type of the functor that compares equality of two keys.
 * @tparam std::allocator<std::pair<const Key, T>> type of memory allocator that allocates trie
 * entries.
 */
template <typename Key,
          typename T,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T>>>
class Trie {
public:
  /**
   * @brief Determine whether this trie contains the given key sequence.
   *
   * @tparam Iterator type of the iterator producing key sequence.
   * @param keyBegin begin iterator of the key sequence.
   * @param keyEnd end iterator of the key sequence.
   * @return true if this trie contains the given key sequence.
   * @return false if this trie does not contain the given key sequence.
   */
  template <typename Iterator>
  bool Contains(Iterator keyBegin, Iterator keyEnd) const {
    auto nonConstThis = const_cast<Trie<Key, T, Hash, KeyEqual, Allocator> *>(this);
    return nonConstThis->GetNode(keyBegin, keyEnd, false);
  }

  /**
   * @brief Insert the given value into the trie, using the given key sequence.
   *
   * @tparam Iterator type of the itrator producing the key sequence.
   * @param keyBegin begin iterator of the key sequence.
   * @param keyEnd end iterator of the key sequence.
   * @param value value to be inserted.
   */
  template <typename Iterator>
  void Insert(Iterator keyBegin, Iterator keyEnd, T value) {
    auto node = GetNode(keyBegin, keyEnd, true);
    node->SetValue(std::move(value));
  }

  /**
   * @brief Get the value corresponds to the given key sequence.
   *
   * @tparam Iterator tyep of iterator producing the key sequence.
   * @param keyBegin begin iterator of the key sequence.
   * @param keyEnd end iterator of the key sequence.
   * @return T* pointer to the value corresponding to the key sequence.
   */
  template <typename Iterator>
  T* Get(Iterator keyBegin, Iterator keyEnd) const {
    auto node = GetNode(keyBegin, keyEnd, false);
    if (!node) {
      return nullptr;
    }
    const auto& value = node->value();
    if (value) {
      return &value.value();
    }
    return nullptr;
  }

private:
  using NodeType = details::TrieNode<Key, T, Hash, KeyEqual, Allocator>;

  std::unique_ptr<NodeType> _root;

  /**
   * @brief Get the trie node corresponding to the given key sequence.
   *
   * If `createIfNotExist` is false, this method can be called on constant receiver objects.
   *
   * @tparam Iterator tyep of the iterator producing key sequence.
   * @param keyBegin begin iterator of the key sequence.
   * @param keyEnd end iterator of the key sequence.
   * @param createIfNotExist a value indicating whether to create new node if some key in the key
   * sequence does not exist.
   * @return NodeType* pointer to the trie node, or nullptr if some key in the key sequence does not
   * exist and `createIfNotExist` is false.
   */
  template <typename Iterator>
  NodeType* GetNode(Iterator keyBegin, Iterator keyEnd, bool createIfNotExist) {
    if (!_root) {
      if (!createIfNotExist) {
        return nullptr;
      }
      _root = caf::make_unique<NodeType>();
    }

    // current keeps track of which node we are visiting along the key sequence.
    auto* current = _root.get();
    for (; keyBegin != keyEnd; ++keyBegin) {
      auto key = *keyBegin;
      current = createIfNotExist
          ? current->GetOrAddChild(key)
          : current->GetChild(key);
      if (!current) {
        return nullptr;
      }
    }

    return current;
  }
}; // namespace Trie

} // namespace caf

#endif
