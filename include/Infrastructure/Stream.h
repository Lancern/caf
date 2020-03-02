#ifndef CAF_STREAM_H
#define CAF_STREAM_H

#include <cstddef>

namespace caf {

/**
 * @brief An adapter type that provides a CAF-coherent `read` and `write` method for all streams in
 * STL.
 *
 * @tparam T an STL stream type.
 */
template <typename T>
class StdStreamAdapter {
public:
  /**
   * @brief Construct a new StdStreamAdapter object.
   *
   * @param inner the STL stream.
   */
  explicit StdStreamAdapter(T& inner)
    : _inner(inner)
  { }

  StdStreamAdapter(const StdStreamAdapter<T> &) = delete;
  StdStreamAdapter(StdStreamAdapter<T> &&) noexcept = default;

  using CharType = typename T::char_type;

  void read(void* buffer, size_t size) {
    _inner.read(reinterpret_cast<CharType *>(buffer), size);
  }

  void write(const void* buffer, size_t size) {
    _inner.write(reinterpret_cast<const CharType *>(buffer), size);
  }

private:
  T& _inner;
}; // class StdStreamAdapter

} // namespace caf

#endif
