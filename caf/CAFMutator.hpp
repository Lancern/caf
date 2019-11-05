#include "CAFMeta.hpp"

#include <cstdint>
#include <utility>
#include <string>
#include <vector>


namespace {

namespace caf {

class Value;
class FunctionCall;
class TestCase;


/**
 * @brief Provide mechanisms to initializ a value.
 *
 */
enum class ValueInitialization : uint32_t {
  /**
   * @brief The value should be initialized by raw bits.
   *
   */
  Bitwise = 0,

  /**
   * @brief The value should be initialized by initializing its fields directly.
   *
   */
  FieldWise = 1,

  /**
   * @brief The value should be initialized by invoking an activator.
   *
   */
  Activator = 2,
};

/**
 * @brief Provide information on value activation strategies.
 *
 */
class Value {
public:
  /**
   * @brief Construct a new Value object.
   *
   * @param initialization the initialization strategy of the value.
   */
  explicit Value(ValueInitialization initialization) noexcept
    : _initialization(initialization)
  { }

  /**
   * @brief Get the initialization strategy of the value.
   *
   * @return ValueInitialization the initialization strategy of the value.
   */
  ValueInitialization initialization() const noexcept {
    return _initialization;
  }

  /**
   * @brief Set the raw data used to initialize the value bitwisely.
   *
   * @param buffer pointer to the buffer containing
   * @param size the size of the data, in bytes.
   */
  void setData(const void* buffer, size_t size) noexcept {
    _data.resize(size);
    memcpy(_data.data(), buffer, size);
  }

  /**
   * @brief Get the raw data used to initialize the value if the value should be
   * initialized bitwisely.
   *
   * @return const std::basic_string<uint8_t>& raw data of the value.
   */
  const std::basic_string<uint8_t>& data() const noexcept {
    return _data;
  }

  /**
   * @brief Set the Activator's ID.
   *
   * @param activatorId the activator's ID.
   */
  void setActivatorId(uint64_t activatorId) noexcept {
    _activatorId = activatorId;
  }

  /**
   * @brief Get the ID of the activator to use.
   *
   * @return uint64_t the ID of the activator to use.
   */
  uint64_t activatorId() const noexcept {
    return _activatorId;
  }

  /**
   * @brief Add an argument to the value initialization.
   *
   */
  template <typename ...Args>
  void addArg(Args&& ...args) {
    _args.emplace_back(std::forward<Args>(args)...);
  }

  /**
   * @brief Get activation strategies of the value to the selected activator.
   *
   * @return const std::vector<Value>& activation strategies of the value to the
   * selected activator.
   */
  const std::vector<Value>& args() const noexcept {
    return _args;
  }

private:
  ValueInitialization _initialization;
  std::basic_string<uint8_t> _data;
  uint64_t _activatorId;
  std::vector<Value> _args;
}; // class Value


/**
 * @brief Provide information on function call.
 *
 */
class FunctionCall {
public:
  /**
   * @brief Construct a new FunctionCall object.
   *
   * @param funcId The ID of the function to be called.
   */
  explicit FunctionCall(uint64_t funcId) noexcept
    : _funcId(funcId),
      _args { }
  { }

  /**
   * @brief Get the ID of the function to be called.
   *
   * @return uint64_t the ID of the function to be called.
   */
  uint64_t funcId() const noexcept {
    return _funcId;
  }

  /**
   * @brief Add an argument to the value.
   *
   */
  template <typename ...Args>
  void addArg(Args&& ...args) {
    _args.emplace_back(std::forward<Args>(args)...);
  }

  /**
   * @brief Get the arguments to the function being called.
   *
   * @return const std::vector<Argument>& arguments to the function being
   * called.
   */
  const std::vector<Value>& args() const noexcept {
    return _args;
  }

private:
  uint64_t _funcId;
  std::vector<Value> _args;
}; // class FunctionCall


/**
 * @brief Represent a test case, a.k.a. a sequence of function calls described
 * by FunctionCall instances.
 *
 */
class TestCase {
public:
  /**
   * @brief Construct a new TestCase object.
   *
   */
  explicit TestCase() noexcept
    : _sequence { }
  { }

  /**
   * @brief Add a function call to the test case.
   *
   */
  template <typename ...Args>
  void addFunctionCall(Args&& ...args) {
    _sequence.emplace_back(std::forward<Args>(args)...);
  }

  /**
   * @brief Get the sequence of function calls carried out by the test case.
   *
   * @return const std::vector<FunctionCall>& the sequence of function calls
   * carried out by the test case.
   */
  const std::vector<FunctionCall>& sequence() const noexcept {
    return _sequence;
  }

private:
  std::vector<FunctionCall> _sequence;
}; // class TestCase


/**
 * @brief Provide functions to serialize and deserialize instances of TestCase
 * from or to binary representation.
 *
 */
class TestCaseSerializer {
public:
  /**
   * @brief Write the given test case into the given stream.
   *
   * @tparam Output the type of the output stream.
   * @param o the output stream.
   * @param testCase the test case to be written.
   */
  template <typename Output>
  void write(Output& o, const TestCase& testCase) const noexcept {
    // Write the number of function calls to the output stream.
    writeTrivial(o, testCase.sequence().size());

    // Write each function call to the output stream.
    for (const auto& funcCall : testCase.sequence()) {
      write(o, funcCall);
    }
  }

  /**
   * @brief Read a TestCase instance from the given input stream.
   *
   * @tparam Input the type of the input stream.
   * @param in the input stream.
   * @return TestCase TestCase instance read from the given input stream.
   */
  template <typename Input>
  TestCase readTestCase(Input& in) const noexcept {
    // Read the number of function calls from the input stream.
    auto numFuncCalls = readTrivial<
        typename std::vector<FunctionCall>::size_type>(in);

    // Read those function calls.
    TestCase testCase { };
    for (decltype(numFuncCalls) i = 0; i < numFuncCalls; ++i) {
      testCase.addFunctionCall(readFunctionCall(in));
    }

    return testCase;
  }

private:
  /**
   * @brief Write a value of a trivial type into the given output stream.
   *
   * @tparam Output The type of the output stream.
   * @tparam POD The type of the value. Trait std::is_trivial<ValueType> must be
   * satisfied.
   * @param o the output stream.
   * @param value the value to be written.
   */
  template <typename Output, typename ValueType>
  void writeTrivial(Output& o, ValueType value) const noexcept {
    static_assert(std::is_trivial<ValueType>::value,
        "Type argument ValueType is not a trivial type.");
    o.write(reinterpret_cast<const uint8_t *>(&value), sizeof(ValueType));
  }

  /**
   * @brief Read a value of a trivial type from the given input stream.
   *
   * @tparam Input the type of the input stream.
   * @tparam ValueType the type of the value. Trait std::is_trivial<ValueType>
   * must be satisfied.
   * @param in the input stream.
   * @return ValueType the value read from the given input stream.
   */
  template <typename Input, typename ValueType>
  ValueType readTrivial(Input& in) const noexcept {
    static_assert(std::is_trivial<ValueType>::value,
        "Type argument ValueType is not a trivial type.");

    uint8_t buffer[sizeof(ValueType)];
    in.read(buffer, sizeof(ValueType));

    return *reinterpret_cast<ValueType *>(&buffer[0]);
  }

  /**
   * @brief Write the given Value object into the given stream.
   *
   * @tparam Output type of the output stream.
   * @param o the output stream.
   * @param value the value to be written.
   */
  template <typename Output>
  void write(Output& o, const Value& value) const noexcept {
    // Write strategy of initialization.
    writeTrivial(o, arg.initialization());
    switch (arg.initialization()) {
      case ValueInitialization::Bitwise:
        // Write the length of the raw data.
        writeTrivial(o, arg.data().length());
        // Write the data.
        o.write(reinterpret_cast<const uint8_t *>(arg.data().data()));
        break;

      case ValueInitialization::Activator:
        writeTrivial(o, arg.activatorId());
        // Fallthrough deliberately.

      case ValueInitialization::FieldWise:
        // Write number of arguments.
        writeTrivial(o, arg.args().size());
        // Write each arg.
        for (const auto& subArg : arg.args()) {
          write(o, subArg);
        }

        break;
    }
  }

  template <typename Input>
  Value readValue(Input& in) const noexcept {
    // Read strategy of initialization.
    auto initialization = readTrivial<ValueInitialization>(in);
    Value obj { initialization };

    switch (initialization) {
      case ValueInitialization::Bitwise: {
        // Read the length of the raw data.
        auto len = readTrivial<typename std::basic_string<uint8_t>::size_type>(
            in);
        // Read the raw data.
        std::basic_string<uint8_t> buffer { };
        buffer.resize(len);
        in.read(buffer.data(), len);
        obj.setData(buffer.data(), len);
        break;
      }

      case ValueInitialization::Activator:

    }
  }

  /**
   * @brief Write the given FunctionCall object into the given stream.
   *
   * @tparam Output the type of the stream.
   * @param o the output stream.
   * @param funcCall the function call object to be written.
   */
  template <typename Output>
  void write(Output& o, const FunctionCall& funcCall) const noexcept {
    // Write function ID.
    writeTrivial(o, funcCall.funcId());

    // Write number of arguments.
    writeTrivial(o, funcCall.args().size());

    // Write arguments.
    for (const auto& arg : funcCall.args()) {
      write(o, arg);
    }
  }

  /**
   * @brief Read a FunctionCall instance from the given input stream.
   *
   * @tparam Input the type of the input stream.
   * @param in the input stream.
   * @return FunctionCall FunctionCall instance read from the given input
   * stream.
   */
  template <typename Input>
  FunctionCall readFunctionCall(Input& in) const noexcept {
    // Read function ID.
    auto funcId = readTrivial<uint64_t>(in);

    // Read number of arguments.
    auto numArgs = readTrivial<typename std::vector<Value>::size_type>(in);
    FunctionCall obj { funcId };

    // Read arguments.
    for (decltype(numArgs) i = 0; i < numArgs; ++i) {
      obj.addArg(readValue(in));
    }
  }
}; // class TestCaseSerializer


} // namespace caf

} // namespace <anonymous>
