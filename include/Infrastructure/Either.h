#ifndef CAF_EITHER_H
#define CAF_EITHER_H

#include <cstddef>
#include <utility>
#include <type_traits>

namespace caf {

namespace details {

template <typename T>
constexpr T max(T lhs, T rhs) {
  return lhs < rhs ? rhs : lhs;
}

template <typename Lhs, typename Rhs>
struct EitherTraits {
  static constexpr const size_t DataSize = max(sizeof(Lhs), sizeof(Rhs));
  static constexpr const size_t DataAlignment = max(alignof(Lhs), alignof(Rhs));
  static constexpr const bool IsCopyConstructible =
      std::is_copy_constructible<Lhs>::value && std::is_copy_constructible<Rhs>::value;
  static constexpr const bool IsMoveConstructible =
      std::is_move_constructible<Lhs>::value && std::is_move_constructible<Rhs>::value;
  static constexpr const bool IsCopyAssignable =
      std::is_copy_assignable<Lhs>::value && std::is_copy_assignable<Rhs>::value;
  static constexpr const bool IsMoveAssignable =
      std::is_move_assignable<Lhs>::value && std::is_move_assignable<Rhs>::value;
}; // struct EitherTraits

} // namespace details

/**
 * @brief A tag type instructing constructor of Either to construct an Either object representing a
 * Lhs object.
 *
 */
struct LhsTag { };

/**
 * @brief A tag type instructing constructor of Either to construct an Either object representing a
 * Rhs object.
 *
 */
struct RhsTag { };

/**
 * @brief Sum type of Lhs and Rhs.
 *
 * @tparam Lhs the first component type.
 * @tparam Rhs the second component type.
 */
template <typename Lhs, typename Rhs>
class Either {
public:
  /**
   * @brief Construct a new Either object that represents a Lhs object.
   *
   * @tparam Args the types of arguments used for initializing a Lhs object.
   * @param tag the tag object.
   * @param args arguments to initialize a Lhs object.
   */
  template <typename ...Args>
  explicit Either(LhsTag tag, Args&&... args)
    : _isLhs(true)
  {
    new (static_cast<void *>(_data)) Lhs(std::forward<Args>(args)...);
  }

  /**
   * @brief Construct a new Either object that represents a Rhs object.
   *
   * @tparam Args the types of arguments used for initializing a Rhs object.
   * @param args arguments to initialize a Rhs object.
   */
  template <typename ...Args>
  explicit Either(RhsTag, Args&&... args)
    : _isLhs(false)
  {
    new (static_cast<void *>(_data)) Rhs(std::forward<Args>(args)...);
  }

  /**
   * @brief Copy-construct a new Either object.
   *
   * This constructor takes part in overload resolution only if both Lhs and Rhs are copy
   * constructible.
   */
  template <typename std::enable_if<
      details::EitherTraits<Lhs, Rhs>::IsCopyConstructible, int>::type = 0>
  Either(const Either<Lhs, Rhs>& another)
    : _isLhs(another._isLhs)
  {
    if (_isLhs) {
      new (static_cast<void *>(_data)) Lhs(another.AsLhs());
    } else {
      new (static_cast<void *>(_data)) Rhs(another.AsRhs());
    }
  }

  /**
   * @brief Move-construct a new Either object.
   *
   * This constructor takes part in overload resolution only if both Lhs and Rhs are move
   * constructible.
   */
  template <typename std::enable_if<
      details::EitherTraits<Lhs, Rhs>::IsMoveConstructible, int>::type = 0>
  Either(Either<Lhs, Rhs>&& another)
    : _isLhs(another._isLhs)
  {
    if (_isLhs) {
      new (static_cast<void *>(_data)) Lhs(std::move(another.AsLhs()));
    } else {
      new (static_cast<void *>(_data)) Rhs(std::move(another.AsRhs()));
    }
  }

  /**
   * @brief Copy-assign this Either object.
   *
   * This operator takes part in overload resolution only if both Lhs and Rhs are copy assignable.
   */
  template <typename std::enable_if<
      details::EitherTraits<Lhs, Rhs>::IsCopyAssignable, int>::type = 0>
  Either<Lhs, Rhs>& operator=(const Either<Lhs, Rhs>& another) {
    ~Either();
    _isLhs = another._isLhs;
    if (_isLhs) {
      new (static_cast<void *>(_data)) Lhs(another.AsLhs());
    } else {
      new (static_cast<void *>(_data)) Rhs(another.AsRhs());
    }
    return *this;
  }

  /**
   * @brief Move-assign this Either object.
   *
   * This operator takes part in overload resolution only if both Lhs and Rhs are move asssignable.
   *
   */
  template <typename std::enable_if<
      details::EitherTraits<Lhs, Rhs>::IsMoveAssignable, int>::type = 0>
  Either<Lhs, Rhs>& operator=(Either<Lhs, Rhs>&& another) {
    ~Either();
    _isLhs = another._isLhs;
    if (_isLhs) {
      new (static_cast<void *>(_data)) Lhs(std::move(another.AsLhs()));
    } else {
      new (static_cast<void *>(_data)) Rhs(std::move(another.AsRhs()));
    }
    return *this;
  }

  ~Either() {
    if (_isLhs) {
      GetLhs()->~Lhs();
    } else {
      GetRhs()->~Rhs();
    }
  }

  /**
   * @brief Determine whether this Either object holds a Lhs object.
   *
   * @return true if this Either object holds a Lhs object.
   * @return false if this Either object holds a Rhs object.
   */
  bool isLhs() const { return _isLhs; }

  /**
   * @brief Determine whether this Either object holds a Rhs object.
   *
   * @return true if this Either object holds a Rhs object.
   * @return false if this Either object holds a Lhs object.
   */
  bool isRhs() const { return !_isLhs; }

  /**
   * @brief Cast this Either object to Lhs object.
   *
   * @return Lhs* pointer to the Lhs object casted from this Either object. If this Either object
   * does not hold a Lhs object, returns nullptr.
   */
  Lhs* GetLhs() {
    if (!_isLhs) {
      return nullptr;
    }

    return reinterpret_cast<Lhs *>(static_cast<void *>(_data));
  }

  /**
   * @brief Cast this Either object to Lhs object.
   *
   * @return const Lhs* const pointer to the Lhs object casted from this Either object. If this
   * Either object does not hold a Lhs object, returns nullptr.
   */
  const Lhs* GetLhs() const {
    if (!_isLhs) {
      return nullptr;
    }

    return reinterpret_cast<const Lhs *>(static_cast<const void *>(_data));
  }

  /**
   * @brief Cast this Either object to Rhs object.
   *
   * @return Rhs* pointer to the Rhs object casted from this Either object. If this Either object
   * does not hold a Rhs object, returns nullptr.
   */
  Rhs* GetRhs() {
    if (_isLhs) {
      return nullptr;
    }

    return reinterpret_cast<Rhs *>(static_cast<void *>(_data));
  }

  /**
   * @brief Cast this Either object to Rhs object.
   *
   * @return const Rhs* const pointer to the Rhs object casted from this Either object. If this
   * Either object does not hold a Rhs object, returns nullptr.
   */
  const Rhs* GetRhs() const {
    if (_isLhs) {
      return nullptr;
    }

    return reinterpret_cast<const Rhs *>(static_cast<void *>(_data));
  }

  /**
   * @brief Cast this Either object to Lhs object.
   *
   * @return Lhs& reference to Lhs object casted from this Either object. If this Either object does
   * not hold a Lhs object, the behavior is undefined.
   */
  Lhs& AsLhs() { return *GetLhs(); }

  /**
   * @brief Cast this Either object to Lhs object.
   *
   * @return const Lhs& const reference to Lhs object casted from this Either object. If this Either
   * object does not hold a Lhs object, the behavior is undefined.
   */
  const Lhs& AsLhs() const { return *GetLhs(); }

  /**
   * @brief Cast this Either object to Rhs object.
   *
   * @return Rhs& reference to Rhs object casted from this Either object. If this Either object does
   * not hold a Rhs object, the behavior is undefined.
   */
  Rhs& AsRhs() { return *GetRhs(); }

  /**
   * @brief Cast this Either object to Rhs object.
   *
   * @return const Rhs& const reference to Rhs object casted from this Either object. If this Either
   * object does not hold a Rhs object, the behavior is undefined.
   */
  const Rhs& AsRhs() const { return *GetRhs(); }

  template <typename ...Args>
  static Either<Lhs, Rhs> CreateLhs(Args&&... args) {
    return Either<Lhs, Rhs> { LhsTag { }, std::forward<Args>(args)... };
  }

  template <typename ...Args>
  static Either<Lhs, Rhs> CreateRhs(Args&&... args) {
    return Either<Lhs, Rhs> { RhsTag { }, std::forward<Args>(args)... };
  }

private:
  alignas(details::EitherTraits<Lhs, Rhs>::DataAlignment)
      char _data[details::EitherTraits<Lhs, Rhs>::DataSize];
  bool _isLhs;
}; // class Either

}; // namespace caf

#endif
