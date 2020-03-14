#ifndef CAF_STREAM_H
#define CAF_STREAM_H

#include <cstddef>
#include <cstdint>
#include <vector>
#include <istream>

namespace caf {

/**
 * @brief Abstract base class for all input streams.
 *
 */
class InputStream {
public:
  explicit InputStream() = default;

  InputStream(const InputStream &) = delete;
  InputStream(InputStream &&) noexcept = default;

  InputStream& operator=(const InputStream &) = delete;
  InputStream& operator=(InputStream &&) = default;

  virtual ~InputStream();

  /**
   * @brief Read raw bytes from the input stream.
   *
   * @param buffer pointer to the buffer to hold the data read.
   * @param size the number of bytes to read.
   */
  virtual void Read(void* buffer, size_t size) = 0;

  /**
   * @brief Read a single byte from the input stream.
   *
   * @return uint8_t the byte read.
   */
  uint8_t ReadByte() {
    uint8_t b;
    Read(&b, 1);
    return b;
  }
};

/**
 * @brief Abstract base class for all output streams.
 *
 */
class OutputStream {
public:
  explicit OutputStream() = default;

  OutputStream(const OutputStream &) = delete;
  OutputStream(OutputStream &&) noexcept = default;

  OutputStream& operator=(const OutputStream &) = delete;
  OutputStream& operator=(OutputStream &&) = default;

  virtual ~OutputStream();

  /**
   * @brief Write raw bytes to the ouput stream.
   *
   * @param buffer pointer to the buffer containing data.
   * @param size number of bytes to write.
   */
  virtual void Write(const void* buffer, size_t size) = 0;
};

/**
 * @brief An input stream adapter for STL input streams.
 *
 */
class StlInputStream : public InputStream {
public:
  /**
   * @brief Construct a new StlInputStream object.
   *
   * @param inner the inner stream.
   */
  explicit StlInputStream(std::istream& inner)
    : _inner(inner)
  { }

  StlInputStream(const StlInputStream &) = delete;
  StlInputStream(StlInputStream &&) noexcept = default;

  void Read(void *buffer, size_t size) override {
    _inner.read(reinterpret_cast<char *>(buffer), size);
  }

private:
  std::istream& _inner;
}; // class StlInputStream

/**
 * @brief An output stream interface around a byte buffer. All data written to the stream will be
 * written to the underlying buffer.
 *
 */
class MemoryOutputStream : public OutputStream {
public:
  /**
   * @brief Construct a new MemoryOutputStream object.
   *
   * @param mem the underlying memory buffer.
   */
  explicit MemoryOutputStream(std::vector<uint8_t>& mem)
    : _mem(mem)
  { }

  /**
   * @brief Write the content of the given buffer into the underlying memory buffer.
   *
   * @param data pointer to the buffer containing data.
   * @param size size of the buffer pointed to by `data`.
   */
  void Write(const void* data, size_t size) override {
    auto ptr = reinterpret_cast<const uint8_t *>(data);
    _mem.insert(_mem.end(), ptr, ptr + size);
  }

private:
  std::vector<uint8_t>& _mem;
};

} // namespace caf

#endif
