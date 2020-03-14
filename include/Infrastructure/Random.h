#ifndef CAF_RANDOM_H
#define CAF_RANDOM_H

#include <cstdint>
#include <random>
#include <limits>
#include <string>
#include <iterator>
#include <type_traits>

namespace caf {

/**
 * @brief Generate random number sequence.
 *
 * @tparam std::default_random_engine the type of the underlying random number generator.
 */
template <typename RNG = std::default_random_engine>
class Random {
public:
  /**
   * @brief Construct a new Random object.
   *
   * @param rng the underlying random number generator.
   */
  explicit Random(RNG rng = RNG { })
    : _rng(std::move(rng))
  { }

  /**
   * @brief Seed the current random number generator.
   *
   * @tparam T type of the seed.
   * @param seed the new seed.
   */
  template <typename T>
  void seed(T seed) {
    _rng.seed(seed);
  }

  /**
   * @brief Get a random integer number in range [min, max].
   *
   * This function takes part in overload resolution only if T is an integral type.
   *
   * @tparam T the type of the output number.
   * @tparam RNG the type of the random number generator.
   * @param min the minimal value in output range.
   * @param max the maximal value in output range.
   * @return T the generated random number.
   */
  template <typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  T Next(T min = 0, T max = std::numeric_limits<T>::max()) {
    std::uniform_int_distribution<T> dist { min, max };
    return dist(_rng);
  }

  /**
   * @brief Get a random real number in range [min, max].
   *
   * This function takes part in overload resolution only if T is an floating point type.
   *
   * @tparam T the type of the output number.
   * @tparam RNG the type of the random number generator.
   * @param min the minimal value in output range.
   * @param max the maximal value in output range.
   * @return T the generated random number.
   */
  template <typename T,
            typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
  T Next(T min = 0, T max = 1) {
    std::uniform_real_distribution<T> dist { min, max };
    return dist(_rng);
  }

  /**
   * @brief Generate random bytes to fill the given buffer.
   *
   * @param buffer pointer to the first byte of the buffer.
   * @param size size of the buffer, in bytes.
   */
  void NextBuffer(uint8_t* buffer, size_t size) {
    while (size >= sizeof(int)) {
      *reinterpret_cast<int *>(buffer) = Next<int>();
      buffer += sizeof(int);
      size -= sizeof(int);
    }
    while (size > 0) {
      --size;
      *buffer++ = Next<uint8_t>();
    }
  }

  /**
   * @brief Generate a new string whose characters are randomly selected from the given character
   * string.
   *
   * @param length length of the string to be generated.
   * @param charset the character string.
   * @return std::string the generated string.
   */
  std::string NextString(size_t length, const std::string& charset) {
    std::string s;
    s.reserve(length);
    for (size_t i = 0; i < length; ++i) {
      s.push_back(Select(charset));
    }
    return s;
  }

  /**
   * @brief Generate a random ASCII string.
   *
   * @param length the length of the string.
   * @return std::string the generated string.
   */
  std::string NextString(size_t length) {
    static const std::string Characters =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "~!@#$%^&*()-=_+"
        "`[]\\{}|;':\",./<>?"
        "\n\t\r";
    return NextString(length, Characters);
  }

  /**
   * @brief Generate a new string whose length is in the given range and characters are randomly
   * selected from the given character string.
   *
   * @param minLength the minimum length of the string.
   * @param maxLength the maximum length of the string.
   * @param charset the character string.
   * @return std::string the generated string.
   */
  std::string NextString(size_t minLength, size_t maxLength, const std::string& charset) {
    return NextString(Next(minLength, maxLength), charset);
  }

  /**
   * @brief Generate a new string whose length is in the given range.
   *
   * @param minLength the minimum length of the string.
   * @param maxLength the maximum length of the string.
   * @return std::string the generated string.
   */
  std::string NextString(size_t minLength, size_t maxLength) {
    return NextString(Next(minLength, maxLength));
  }

  /**
   * @brief Generate a floating point value in [0, 1] and test whether it is less than p.
   *
   * @param p the probability of returning true.
   * @return true if the generated value is less than p.
   * @return false if the generated value is equal to or greater than p.
   */
  bool WithProbability(double p) {
    if (p <= 0) {
      return false;
    }
    if (p >= 1) {
      return true;
    }

    return Next<double>() < p;
  }

  /**
   * @brief Generate a random integer number that is greater than or equal to 0 and less than the
   * size of the given container.
   *
   * The behavior is undefined if the given container is empty.
   *
   * @tparam Container type of the container. The container type should satisfy C++ named
   * requirement `Container`.
   * @param c the container object.
   * @return Container::size_type type of the size of the container.
   */
  template <typename Container>
  typename Container::size_type Index(const Container &c) {
    return Next<typename Container::size_type>(0, c.size() - 1);
  }

  /**
   * @brief Generate a random integer number that is greater than or equal to 0 and less than the
   * size of the given array.
   *
   * The behavior is undefined if the given array is empty.
   *
   * @tparam T the type of the elements in the array.
   * @tparam N the size of the array.
   * @return size_t a randomly generated index of the array.
   */
  template <typename T, size_t N>
  size_t Index(const T (&)[N]) {
    return Next<size_t>(0, N - 1);
  }

  /**
   * @brief Randomly select an element from the given container.
   *
   * @tparam Container type of the container. The container type should satisfy C++ named
   * requirement `SequenceContainer`.
   * @param c the container instance.
   * @return Container::const_reference const reference to the selected element.
   */
  template <typename Container>
  typename Container::const_reference Select(const Container& c) {
    return c[Index(c)];
  }

  /**
   * @brief Randomly select an element from the given container.
   *
   * @tparam Container type of the container. The container type should satisfy C++ named
   * requirement `SequenceContainer`.
   * @param c the container instance.
   * @return Container::const_reference const reference to the selected element.
   */
  template <typename Container>
  typename Container::reference Select(Container& c) {
    return c[Index(c)];
  }

  /**
   * @brief Randomly select an element from the given range.
   *
   * @tparam Iter type of the iterator referencing the rance.
   * @param begin the begin iterator.
   * @param end the end iterator.
   * @return std::iterator_traits<Iter>::reference reference to the selected element.
   */
  template <typename Iter>
  typename std::iterator_traits<Iter>::reference Select(Iter begin, Iter end) {
    auto size = std::distance(begin, end);
    return *std::next(begin, Next<size_t>(0, size - 1));
  }

  /**
   * @brief Randomly select an element from the given array.
   *
   * @tparam T type of the elements in the array.
   * @tparam N number of elements in the array.
   * @param array the array.
   * @return T& the selected element.
   */
  template <typename T, size_t N>
  T& Select(T (&array)[N]) {
    return array[Index(array)];
  }

  /**
   * @brief Randomly select an element from the given array.
   *
   * @tparam T type of the elements in the array.
   * @tparam N number of elements in the array.
   * @param array the array.
   * @return T& the selected element.
   */
  template <typename T, size_t N>
  const T& Select(const T (&array)[N]) {
    return array[Index(array)];
  }

private:
  RNG _rng;
}; // class Random

} // namespace caf

#endif
