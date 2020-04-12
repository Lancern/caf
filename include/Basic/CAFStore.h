#ifndef CAF_CAF_STORE_H
#define CAF_CAF_STORE_H

#include "Infrastructure/Optional.h"
#include "Infrastructure/Random.h"
#include "Basic/Function.h"

#include "json/json.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace caf {

/**
 * @brief CAF metadata store.
 *
 */
class CAFStore {
public:
  /**
   * @brief Provide statistics about the store.
   *
   */
  struct Statistics {
    size_t ApiFunctionsCount;
  }; // struct Statistics

  /**
   * @brief An entry in the database.
   *
   */
  class Entry {
  public:
    /**
     * @brief Construct a new Entry object.
     *
     * The constructed entry does not contain any functions.
     *
     */
    explicit Entry() = default;

    Entry(const Entry &) = delete;
    Entry(Entry &&) noexcept = default;

    Entry& operator=(const Entry &) = delete;
    Entry& operator=(Entry &&) = default;

    /**
     * @brief Determine whether this entry contains a function.
     *
     * @return true if this entry contains a function.
     * @return false if this entry does not contain a function.
     */
    bool HasFunction() const { return static_cast<bool>(_func); }

    /**
     * @brief Get the function contained on this entry.
     *
     * @return const Function& the function contained on this entry.
     */
    const Function& GetFunction() const { return _func.value(); }

    /**
     * @brief Set the function contained in this entry.
     *
     * @param func the function.
     */
    void SetFunction(Function func) { _func.set(std::move(func)); }

    /**
     * @brief Get the number of descendents.
     *
     * @return size_t the number of descendents.
     */
    size_t GetDescendentsCount() const {
      return _descendents.size();
    }

    /**
     * @brief Randomly select a descendent that contains a function.
     *
     * The selected entry is guaranteed to contain a function.
     *
     * @param rnd the random number generator to use.
     * @return Entry* the selected descendent entry.
     */
    Entry* SelectDescendent(Random<>& rnd) const {
      return rnd.Select(_descendents);
    }

    friend class CAFStore;

  private:
    Optional<Function> _func;
    std::unordered_map<std::string, std::unique_ptr<Entry>> _children;
    std::vector<Entry *> _descendents;

    bool HasChild(const std::string& name) const;

    void AddChild(const std::string& name, std::unique_ptr<Entry> entry);

    Entry* GetChild(const std::string& name) const;

    void AddDescendent(Entry* entry);
  }; // class Entry

  /**
   * @brief Iterator that iterates all functions contained in a CAFStore.
   *
   */
  class FunctionIterator {
  public:
    explicit FunctionIterator(
        typename std::unordered_map<FunctionIdType, Entry *>::const_iterator inner)
      : _inner(inner)
    { }

    const Function& operator*() const {
      return _inner->second->GetFunction();
    }

    const Function* operator->() const {
      return &_inner->second->GetFunction();
    }

    FunctionIterator& operator++() {
      ++_inner;
      return *this;
    }

    FunctionIterator operator++(int) {
      auto dup = *this;
      ++_inner;
      return dup;
    }

    bool operator==(const FunctionIterator& another) const {
      return _inner == another._inner;
    }

    bool operator!=(const FunctionIterator& another) const {
      return _inner != another._inner;
    }

  private:
    typename std::unordered_map<FunctionIdType, Entry *>::const_iterator _inner;
  }; // class FunctionIterator

  using iterator = FunctionIterator;
  using const_iterator = FunctionIterator;

  /**
   * @brief Construct a new CAFStore object.
   *
   */
  explicit CAFStore();

  CAFStore(const CAFStore &) = delete;
  CAFStore(CAFStore &&) noexcept;

  CAFStore& operator=(const CAFStore &) = delete;
  CAFStore& operator=(CAFStore &&);

  ~CAFStore();

  /**
   * @brief Load data from the given JSON container.
   *
   * @param json the JSON container.
   */
  void Load(const nlohmann::json& json);

  /**
   * @brief Get the number of API functions.
   *
   * @return size_t the number of API functions.
   */
  size_t GetFunctionsCount() const;

  /**
   * @brief Add the given API function to the store.
   *
   * The ID of the given function must equal to the number of API functions registered in this
   * CAFStore at the time of being invoked, or this function will trigger an assertion failure.
   *
   * @param func the function to register.
   */
  void AddFunction(Function func);

  /**
   * @brief Get the API function with the given ID.
   *
   * This function will throw an exception if the given ID cannot be found.
   *
   * @param id ID of the function.
   * @return const Function& the function.
   */
  const Function& GetFunction(FunctionIdType id) const {
    return _funcIdToEntry.at(id)->GetFunction();
  }

  /**
   * @brief Get the number of entries.
   *
   * @return size_t the number of entries.
   */
  size_t GetEntriesCount() const { return _entries.size(); }

  /**
   * @brief Randomly select an entry from this CAFStore object.
   *
   * @param rnd the random number generator to use.
   * @return Entry* the selected entry.
   */
  Entry* SelectEntry(Random<>& rnd) const {
    return rnd.Select(_entries);
  }

  /**
   * @brief Get the entry at the given index.
   *
   * @param index the index.
   * @return Entry* the entry at the given index.
   */
  Entry* GetEntry(size_t index) const {
    return _entries.at(index);
  }

  /**
   * @brief Get the statistics about the current store.
   *
   * @return Statistics the statistics about the current store.
   */
  Statistics GetStatistics() const;

  /**
   * @brief Serialize this object to JSON form.
   *
   * @return nlohmann::json the JSON form.
   */
  nlohmann::json ToJson() const;

  const_iterator begin() const {
    return FunctionIterator { _funcIdToEntry.begin() };
  }

  const_iterator end() const {
    return FunctionIterator { _funcIdToEntry.end() };
  }

private:
  std::unique_ptr<Entry> _root;
  std::vector<Entry *> _entries;
  std::unordered_map<FunctionIdType, Entry *> _funcIdToEntry;

  std::unique_ptr<Entry> CreateEntry();
}; // class CAFStore

} // namespace caf

#endif
