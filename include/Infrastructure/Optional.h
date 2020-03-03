#ifndef CAF_OPTIONAL_H
#define CAF_OPTIONAL_H

#include <cassert>
#include <new>
#include <utility>
#include <type_traits>

namespace caf {

/**
 * @brief A monad representing an optional value.
 *
 * This class resembles the `Maybe` type class in Haskell and `Option` enum in Rust.
 *
 * @tparam T the type of the object contained in the optional value.
 */
template <typename T>
class Optional {
public:
  /**
   * @brief Construct a new Optional object that contains no values.
   *
   */
  explicit Optional()
    : _hasValue(false)
  { }

  /**
   * @brief Construct a new Optional object that contains the given object.
   *
   * @param value the object to be contained in the constructed @see Optional<T> object.
   */
  explicit Optional(T value)
    : _hasValue(false) {
    emplace(std::move(value));
  }

  template <typename std::enable_if<std::is_copy_constructible<T>::value, int>::type = 0>
  Optional(const Optional<T>& another)
    : _hasValue(false)
  {
    if (another.hasValue()) {
      emplace(another.value());
    }
  }

  template <typename std::enable_if<std::is_move_constructible<T>::value, int>::type = 0>
  Optional(Optional<T>&& another) noexcept
    : _hasValue(false)
  {
    if (another.hasValue()) {
      emplace(another.take());
    }
  }

  ~Optional() {
    drain();
  }

  template <typename std::enable_if<std::is_copy_assignable<T>::value, int>::type = 0>
  Optional<T>& operator=(const Optional<T>& another) {
    drain();
    if (another.hasValue()) {
      emplace(another.value());
    }
    return *this;
  }

  template <typename std::enable_if<std::is_move_assignable<T>::value, int>::type = 0>
  Optional<T>& operator=(Optional<T>&& another) {
    drain();
    if (another.hasValue()) {
      emplace(another.take());
    }
    return *this;
  }

  /**
   * @brief Determine whether this @see Optional<T> object contains a value.
   *
   * @return true if this @see Optional<T> object contains a value.
   * @return false if this @see Optional<T> object does not contain a value.
   */
  bool hasValue() const { return _hasValue; }

  explicit operator bool() const { return hasValue(); }

  /**
   * @brief Destroy the old contained value and construct a new value using the given arguments in
   * place.
   *
   * @tparam Args types of arguments.
   * @param args arguments that will be passed to the constructor of T to construct a new value of
   * T in place.
   */
  template <typename ...Args>
  void emplace(Args&&... args) {
    drain();
    new(_value) T(std::forward<Args>(args)...);
    _hasValue = true;
  }

  /**
   * @brief Destroy the old contained value and moves the given value into this @see Optional<T>
   * instance.
   *
   * @param value the value to be moved in.
   */
  void set(T value) {
    emplace(std::move(value));
  }

  /**
   * @brief Destroy the old contained value, if there is one. After calling this function, this
   * @see Optional<T> object will be an empty container.
   *
   */
  void drain() {
    if (_hasValue) {
      value().~T();
      _hasValue = false;
    }
  }

  /**
   * @brief Take out the contained value, leave an empty Optional object behind.
   *
   * @return T the contained value.
   */
  T take() {
    assert(_hasValue && "Trying to take an empty Optional object.");
    auto ret = std::move(value());
    _hasValue = false;
    return ret;
  }

  /**
   * @brief Get a reference to the contained value.
   *
   * @return T& a reference to the contained value.
   */
  T& value() { return *reinterpret_cast<T *>(static_cast<char *>(_value)); }

  /**
   * @brief Get a constant reference to the contained value.
   *
   * @return const T& a constant reference to the contained value.
   */
  const T& value() const { return *reinterpret_cast<const T *>(static_cast<const char *>(_value)); }

  T& operator*() { return value(); }

  const T& operator*() const { return value(); }

  T* operator->() { return &value(); }

  const T* operator->() const { return &value(); }

private:
  alignas(alignof(T)) char _value[sizeof(T)];
  bool _hasValue;
  // _value contains raw data of an object of T which might be uninitialized.
}; // class Optional

} // namespace caf

#endif
