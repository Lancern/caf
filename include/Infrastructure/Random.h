#ifndef CAF_RANDOM_H
#define CAF_RANDOM_H

#include <random>
#include <limits>
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
   * @brief Generate a floating point value in [0, 1] and test whether it is less than p.
   *
   * @param p the probability of returning true.
   * @return true if the generated value is less than p.
   * @return false if the generated value is equal to or greater than p.
   */
  bool WithProbability(double p) {
    if (p < 0) {
      p = 0;
    }
    if (p > 1) {
      p = 1;
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

private:
  RNG _rng;
}; // class Random

} // namespace caf

#endif
