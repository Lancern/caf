#ifndef CAF_IDENTITY_H
#define CAF_IDENTITY_H

#include "json/json.hpp"

namespace caf {

namespace details {

/**
 * @brief Provide default logic to allocate self-increment IDs. This class is
 * not thread-safe. Multiple instances of IdAllocator are independent, and they
 * may produce the same IDs.
 *
 */
template <typename T>
class DefaultIdAllocator {
  static_assert(std::is_integral<T>::value, "T should be an integral type.");

public:
  /**
   * @brief Generate next ID in ID sequence.
   *
   * @return T the next ID.
   */
  T operator()() noexcept {
    return _id++;
  }

private:
  T _id;
}; // class IdAllocator

} // namespace details

/**
 * @brief Abstract base class for types whose instances own unique IDs.
 *
 * @tparam SeriesTag a tag type indicating the ID series. @see Identity values on different
 * SeriesTag types may produce the same ID; while @see Identity values on the same SeriesTag type
 * will always produce different IDs.
 * @tparam Id the type of instance IDs to be used.
 * @tparam IdAllocator the type of the ID allocator.
 */
template <typename SeriesTag,
          typename Id,
          typename IdAllocator = details::DefaultIdAllocator<Id>>
class Identity {
  static IdAllocator _allocator;

public:
  /**
   * @brief Construct a new Identity object. The ID of this object will be generated automatically
   * through the specified IdAllocator.
   *
   */
  explicit Identity()
    : _id(_allocator())
  { }

  /**
   * @brief Construct a new Identity object.
   *
   * @param id the ID of this object.
   */
  explicit Identity(Id id)
    : _id(id)
  { }

  /**
   * @brief Get the ID of this object.
   *
   * @return Id ID of this object.
   */
  Id id() const { return _id; }

  /**
   * @brief Set the ID of this object.
   *
   * @param id the new ID of this object.
   */
  void SetId(Id id) { _id = id; }

  /**
   * @brief Serialize the object ID of the given object and populate it into the given JSON
   * container.
   *
   * @param object the object to serialize.
   * @param json the JSON container.
   */
  static void PopulateJson(const Identity<Id, IdAllocator>& object, nlohmann::json& json) {
    json["id"] = object._id;
  }

#ifndef CAF_LLVM
  /**
   * @brief Deserialize object ID from the given JSON snippet and populate it onto the given object.
   *
   * @param object the object to populate object ID onto.
   * @param json the JSON snippet.
   */
  static void PopulateFromJson(Identity<Id, IdAllocator>& object, const nlohmann::json& json) {
    object._id = json.get<Id>();
  }
#endif

private:
  Id _id;
};

template <typename SeriesTag, typename Id, typename IdAllocator>
IdAllocator Identity<SeriesTag, Id, IdAllocator>::_allocator { };

} // namespace caf

#endif
