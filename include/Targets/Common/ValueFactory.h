#ifndef CAF_VALUE_FACTORY_H
#define CAF_VALUE_FACTORY_H

#include <cstdint>
#include <string>

namespace caf {

/**
 * @brief Abstract base class for value factories.
 *
 * @tparam TargetTraits trait object describing type systems used by the target.
 */
template <typename TargetTraits>
class ValueFactory {
public:
  using UndefinedType = typename TargetTraits::UndefinedType;
  using NullType = typename TargetTraits::NullType;
  using BooleanType = typename TargetTraits::BooleanType;
  using StringType = typename TargetTraits::StringType;
  using IntegerType = typename TargetTraits::IntegerType;
  using FloatType = typename TargetTraits::FloatType;
  using ArrayBuilderType = typename TargetTraits::ArrayBuilderType;
  using FunctionType = typename TargetTraits::FunctionType;

  /**
   * @brief Destroy the ValueFactory object.
   *
   */
  virtual ~ValueFactory() = default;

  /**
   * @brief Create a target-specific undefined object.
   *
   * @return UndefinedType the created undefined object.
   */
  virtual UndefinedType CreateUndefined() = 0;

  /**
   * @brief Create a target-specific null object.
   *
   * @return NullType the created null object.
   */
  virtual NullType CreateNull() = 0;

  /**
   * @brief Create a target-specific boolean object.
   *
   * @param value the boolean value.
   * @return BooleanType the created boolean object.
   */
  virtual BooleanType CreateBoolean(bool value) = 0;

  /**
   * @brief Create a target-specific string object.
   *
   * @param buffer pointer to the first byte of the buffer containing the string data.
   * @param size size of the string buffer.
   * @return StringType the created string object.
   */
  virtual StringType CreateString(const uint8_t* buffer, size_t size) = 0;

  /**
   * @brief Create a target-specific integer object.
   *
   * @param value the integer value.
   * @return IntegerType the created integer object.
   */
  virtual IntegerType CreateInteger(int32_t value) = 0;

  /**
   * @brief Create a target-specific float object.
   *
   * @param value the floating point value.
   * @return FloatType the created float object.
   */
  virtual FloatType CreateFloat(double value) = 0;

  /**
   * @brief Start building an array.
   *
   * @param size size of the array to be built.
   * @return ArrayBuilderType the created array builder.
   */
  virtual ArrayBuilderType StartBuildArray(size_t size) = 0;

  /**
   * @brief Create a target-specific undefined object.
   *
   * @param functionId ID of the function.
   * @return UndefinedType the created undefined object.
   */
  virtual FunctionType CreateFunction(uint32_t functionId) = 0;

protected:
  /**
   * @brief Construct a new ValueFactory object.
   *
   */
  explicit ValueFactory() = default;

  ValueFactory(const ValueFactory<TargetTraits> &) = delete;
  ValueFactory(ValueFactory<TargetTraits> &&) noexcept = default;

  ValueFactory<TargetTraits>& operator=(const ValueFactory<TargetTraits> &) = delete;
  ValueFactory<TargetTraits>& operator=(ValueFactory<TargetTraits> &&) = default;
}; // class ValueFactory

} // namespace caf

#endif
