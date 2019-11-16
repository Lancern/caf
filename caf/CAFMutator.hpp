#include "CAFMeta.hpp"

#include <cstdint>
#include <utility>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <exception>
#include <random>
#include <limits>
#include <type_traits>


#define CAF_UNREACHABLE()               \
  while (1) {                           \
    std::cerr << "Unreachable code: "   \
        << __FUNC__                     \
        << "@" << __FILE__              \
        << ":" << __LINE__              \
        << std::endl;                   \
    std::terminate();                   \
  }


namespace {

namespace caf {

class CAFObjectPool;
class CAFCorpus;

class Value;
class BitsValue;
class PointerValue;
class ArrayValue;
class StructValue;
class FunctionCall;
class TestCase;


/**
 * @brief Pool of objects that can be passed as function arguments of some specific type.
 *
 * Instances of this class cannot be copied.
 *
 */
class CAFObjectPool {
public:
  /**
   * @brief Construct a new CAFObjectPool object.
   *
   */
  explicit CAFObjectPool()
    : _values { }
  { }

  CAFObjectPool(const CAFObjectPool &) = delete;
  CAFObjectPool(CAFObjectPool &&) = default;

  CAFObjectPool& operator=(const CAFObjectPool &) = delete;
  CAFObjectPool& operator=(CAFObjectPool &&) = default;

  /**
   * @brief Get all values contained in this object pool.
   *
   * @return const std::vector<std::unique_ptr<Value>>& list of all values contained in this object
   * pool.
   */
  const std::vector<std::unique_ptr<Value>>& values() const noexcept {
    return _values;
  }

  /**
   * @brief Get the number of values contained in this object pool.
   *
   * @return size_t
   */
  size_t size() const noexcept {
    return static_cast<size_t>(_values.size());
  }

  /**
   * @brief Create a new value in the object pool.
   *
   * @tparam T the type of the value to be created.
   * @tparam Args the types of the arguments for constructing a new value.
   * @param args arguments for constructing a new value.
   * @return T* pointer to the created value.
   */
  template <typename T, typename ...Args>
  T* create(Args&&... args) {
    _values.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    return _values.back().get();
  }

private:
  std::vector<std::unique_ptr<Value>> _values;
};


/**
 * @brief Provide a smart pointer type that references to a test case owned by a `CAFCorpus`
 * instance.
 *
 */
class CAFCorpusTestCaseRef {
public:
  using index_type = typename std::vector<TestCase>::size_type;

  /**
   * @brief Construct a new CAFCorpusTestCaseRef object.
   *
   * @param corpus the corpus owning the referenced `TestCase` object.
   * @param index the index of the `TestCase` object inside the corpus.
   */
  explicit CAFCorpusTestCaseRef(CAFCorpus* corpus, index_type index) noexcept
    : _corpus(corpus),
      _index(index)
  { }

  CAFCorpus* corpus() const noexcept {
    return _corpus;
  }

  TestCase& operator*() const noexcept;
  TestCase* operator->() const noexcept;

private:
  CAFCorpus* _corpus;
  index_type _index;
};

/**
 * @brief Corpus of the fuzzer. This is the top-level object maintained in the fuzzer server.
 *
 * Instance of corpus contains a `CAFStore` instance, which provide metadata about the target
 * program's type information and API information; contains multiple `CAFObjectPool` objects,
 * each associated with a string representing the name of the type of the values contained in the
 * `CAFObjectPool` object.
 *
 */
class CAFCorpus {
public:
  /**
   * @brief Construct a new CAFCorpus object.
   *
   * @param store the `CAFStore` instance containing metadata about the target program's type
   * information.
   */
  explicit CAFCorpus(std::unique_ptr<CAFStore> store) noexcept
    : _store(std::move(store)),
      _pools { }
  { }

  /**
   * @brief Get the `CAFStore` instance containing type information and API information in the
   * target program.
   *
   * @return CAFStore* the `CAFStore` instance containing type information and API information in
   * the target program.
   */
  CAFStore* store() const noexcept {
    return _store.get();
  }

  /**
   * @brief Get a list of test cases contained in the corpus.
   *
   * @return const std::vector<TestCase>& a list of test cases contained in the corpus.
   */
  const std::vector<TestCase>& testCases() const noexcept {
    return _testCases;
  }

  /**
   * @brief Get the `TestCase` object at the given index.
   *
   * @param index the index of the desired `TestCase` object.
   * @return CAFCorpusTestCaseRef smart pointer to the test case object.
   */
  CAFCorpusTestCaseRef getTestCase(size_t index) noexcept {
    return CAFCorpusTestCaseRef { this, index };
  }

  /**
   * @brief Create a new test case.
   *
   * @tparam Args types of arguments used to construct a new `TestCase` object.
   * @param args arguments used to construct a new `TestCase` object.
   * @return CAFCorpusTestCaseRef a `CAFCorpusTestCaseRef` instance representing a smart pointer
   * to the created `TestCase` object.
   */
  template <typename ...Args>
  CAFCorpusTestCaseRef createTestCase(Args&&... args) noexcept {
    _testCases.emplace_back(std::forward<Args>(args)...);
    return CAFCorpusTestCaseRef { this, _testCases.size() - 1 };
  }

  friend class CAFCorpusTestCaseRef;

private:
  std::unique_ptr<CAFStore> _store;
  std::unordered_map<std::string, std::unique_ptr<CAFObjectPool>> _pools;
  std::vector<TestCase> _testCases;
};

TestCase& CAFCorpusTestCaseRef::operator*() const noexcept {
  return _corpus->_testCases[_index];
}

TestCase* CAFCorpusTestCaseRef::operator->() const noexcept {
  return &_corpus->_testCases[_index];
}


/**
 * @brief Represent the kind of a value.
 *
 */
enum class ValueKind : int {
  /**
   * @brief The value is an instance of `BitsType`.
   *
   */
  BitsValue,

  /**
   * @brief The value is an instance of `PointerType`.
   *
   */
  PointerValue,

  /**
   * @brief The value is an instance of `ArrayType`.
   *
   */
  ArrayValue,

  /**
   * @brief The value is an instance of `StructType`.
   *
   */
  StructValue
};

/**
 * @brief Abstract base class representing a value that can be passed to a function.
 *
 */
class Value {
public:
/**
 * @brief Destroy the Value object.
 *
 */
  virtual ~Value() noexcept = default;

  /**
   * @brief Get the object pool containing this value.
   *
   * @return CAFObjectPool* object pool containing this value.
   */
  CAFObjectPool* pool() const noexcept {
    return _pool;
  }

  /**
   * @brief Kind of the value.
   *
   * @return ValueKind kind of the value.
   */
  ValueKind kind() const noexcept {
    return _kind;
  }

  /**
   * @brief Get the type of the value.
   *
   * @return const Type* type of the value.
   */
  const Type* type() const noexcept {
    return _type;
  }

protected:
  /**
   * @brief Construct a new Value object.
   *
   * @param pool object pool containing this value.
   * @param kind kind of the value.
   * @param type the type of the value.
   */
  explicit Value(CAFObjectPool* pool, ValueKind kind, const Type* type) noexcept
    : _pool(pool),
      _kind(kind),
      _type { type }
  { }

private:
  CAFObjectPool* _pool;
  ValueKind _kind;
  const Type* _type;
};


/**
 * @brief Represent a value of `BitsType`.
 *
 */
class BitsValue : public Value {
public:
  /**
   * @brief Construct a new BitsValue object. The data of the value will be initialized to zeros.
   *
   * @param pool object pool containing this value.
   * @param type the type of the value.
   */
  explicit BitsValue(CAFObjectPool* pool, const BitsType* type) noexcept
    : Value { pool, ValueKind::BitsValue, type },
      _size(type->size()),
      _data { } {
    _data.resize(_size);
  }

  /**
   * @brief Get the size of the value, in bytes.
   *
   * @return size_t size of the value, in bytes.
   */
  size_t size() const noexcept {
    return _size;
  }

  /**
   * @brief Get a pointer to the buffer containing raw binary data of the value.
   *
   * @return uint8_t* a pointer to the buffer containing raw binary data of the value.
   */
  uint8_t* data() noexcept {
    return _data.data();
  }

  /**
   * @brief Get a pointer to the buffer containing raw binary data of the value. The returned
   * pointer is immutable.
   *
   * @return const uint8_t* immutable pointer to the buffer containing raw binary data of the value.
   */
  const uint8_t* data() const noexcept {
    return _data.data();
  }

private:
  size_t _size;
  std::vector<uint8_t> _data;
};


/**
 * @brief Represent a value of `PointerType`.
 *
 */
class PointerValue : public Value {
public:
  /**
   * @brief Construct a new PointerValue object.
   *
   * @param pool the object pool containing this value.
   * @param pointee value at the pointee's site.
   * @param type type of the pointer.
   */
  explicit PointerValue(CAFObjectPool* pool, Value* pointee, const PointerType* type)
    : Value { pool, ValueKind::PointerValue, type },
      _pointee(pointee)
  { }

  /**
   * @brief Get the value at the pointee's site.
   *
   * @return Value* value at the pointee's site.
   */
  Value* pointee() const noexcept {
    return _pointee;
  }

private:
  Value* _pointee;
};


/**
 * @brief Represent a value of `ArrayType`.
 *
 */
class ArrayValue : public Value {
public:
  /**
   * @brief Construct a new ArrayValue object.
   *
   * @param pool the object pool containing this value.
   * @param type type of the array value.
   * @param elements elements contained in the array.
   */
  explicit ArrayValue(CAFObjectPool* pool, const ArrayType* type, std::vector<Value *> elements)
      noexcept
    : Value { pool, ValueKind::ArrayValue, type },
      _size(type->size()),
      _elements(std::move(elements))
  { }

  /**
   * @brief Get the number of elements in this array.
   *
   * @return size_t the number of elements in this array.
   */
  size_t size() const noexcept {
    return _size;
  }

  /**
   * @brief Get the elements contained in this array.
   *
   * @return const std::vector<Value *>& elements contained in this array.
   */
  const std::vector<Value *>& elements() const noexcept {
    return _elements;
  }

private:
  size_t _size;
  std::vector<Value *> _elements;
};


/**
 * @brief Represent a value of `StructType`.
 *
 */
class StructValue : public Value {
public:
  /**
   * @brief Construct a new StructValue object.
   *
   * @param pool the object pool containing this value.
   * @param type the type of the value.
   * @param activator the activator to use to activate the object.
   * @param args arguments passed to the object.
   */
  explicit StructValue(CAFObjectPool* pool, const StructType* type,
      const Activator* activator, std::vector<Value *> args)
      noexcept
    : Value { pool, ValueKind::StructValue, type },
      _activator(activator),
      _args(std::move(args))
  { }

  /**
   * @brief Get the activator used to activate objects.
   *
   * @return const Activator* activator used to activate objects.
   */
  const Activator* activator() const noexcept {
    return _activator;
  }

  /**
   * @brief Get the arguments passed to the activator.
   *
   * @return const std::vector<Value *>& list of arguments passed to the activator.
   */
  const std::vector<Value *>& args() const noexcept {
    return _args;
  }

private:
  const Activator *_activator;
  std::vector<Value *> _args;
};


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
   * @brief Add an argument to the function call.
   *
   * @param arg argument to the function call.
   */
  void addArg(Value* arg) noexcept {
    _args.push_back(arg);
  }

  /**
   * @brief Get the arguments to the function being called.
   *
   * @return const std::vector<Argument>& arguments to the function being
   * called.
   */
  const std::vector<Value *>& args() const noexcept {
    return _args;
  }

private:
  uint64_t _funcId;
  std::vector<Value *> _args;
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
 * @brief Provide functions to serialize instances of `TestCase` to binary representation that can
 * be sent to the target program for fuzzing.
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
   * @brief Write the given Value object into the given stream.
   *
   * @tparam Output type of the output stream.
   * @param o the output stream.
   * @param value the value to be written.
   */
  template <typename Output>
  void write(Output& o, const Value& value) const noexcept {
    // Write kind of the value.
    writeTrivial(o, value.kind());
    switch (value.kind()) {
      case ValueKind::BitsValue: {
        // Write the number of bytes followed by the raw binary data of the value to the output
        // stream.
        auto bitsValue = dynamic_cast<const BitsValue &>(value);
        writeTrivial(o, bitsValue.size());
        o.write(bitsValue.data(), bitsValue.size());
        break;
      }
      case ValueKind::PointerValue: {
        // Write the underlying value pointed to by the pointer.
        auto pointerValue = dynamic_cast<const PointerValue &>(value);
        write(o, *pointerValue.pointee());
        break;
      }
      case ValueKind::ArrayValue: {
        // Write the number of elements and each element into the output stream.
        auto arrayValue = dynamic_cast<const ArrayValue &>(value);
        writeTrivial(o, arrayValue.size());
        for (auto el : arrayValue.elements()) {
          write(o, *el);
        }
        break;
      }
      case ValueKind::StructValue: {
        // Write the ID of the activator and arguments to the activator to the output stream.
        auto structValue = dynamic_cast<const StructValue &>(value);
        writeTrivial(o, structValue.activator()->id());
        for (auto arg : structValue.args()) {
          write(o, *arg);
        }
        break;
      }
      default: CAF_UNREACHABLE()
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
}; // class TestCaseSerializer


/**
 * @brief Provide mutation strategy for test cases.
 *
 */
template <typename RNG = std::mt19937>
class TestCaseMutator {
public:
  /**
   * @brief Construct a new TestCaseMutator object.
   *
   * @param store the type info metadata store.
   * @param corpus test corpus.
   */
  explicit TestCaseMutator(CAFCorpus* corpus) noexcept
    : _corpus(corpus),
      _rng { }
  { }

  /**
   * @brief Get the corpus.
   *
   * @return CAFCorpus* pointer to the corpus.
   */
  CAFCorpus* corpus() const noexcept {
    return _corpus;
  }

  /**
   * @brief Mutate the given test case and produce a new one.
   *
   * @param corpus the corpus of test cases.
   * @return TestCase the mutated test case.
   */
  CAFCorpusTestCaseRef mutate(CAFCorpusTestCaseRef testCase) noexcept {
    enum MutationStrategy {
      Sequence,
      Argument,
      _MutationStrategyMax = Argument
    };
    auto strategy = static_cast<MutationStrategy>(
        random<int>(0, _MutationStrategyMax));
    switch (strategy) {
      case Sequence:
        return mutateSequence(previous);
      case Argument:
        return mutateArgument(previous);
      default:
        CAF_UNREACHABLE()
    }
  }

private:
  CAFCorpus* _corpus;
  RNG _rng;

  CAFCorpusTestCaseRef squash(CAFCorpusTestCaseRef previous) noexcept {
    // TODO: Implement TestCaseMutator::squash.
  }

  CAFCorpusTestCaseRef splice(CAFCorpusTestCaseRef previous) noexcept {
    auto source = _corpus->getTestCase(random_index(_corpus->testCases()));
    auto previousLen = previous->sequence().size();
    auto sourceLen = source->sequence().size();

    using index = decltype(previousLen);
    auto spliceIndex = random<index>(0, std::min(previousLen, sourceLen));

    auto mutated = _corpus->createTestCase();
    for (index i = 0; i < spliceIndex; ++i) {
      mutated->addFunctionCall(previous->sequence()[i]);
    }
    for (index i = spliceIndex; i < sourceLen; ++i) {
      mutated->addFunctionCall(source->sequence()[i]);
    }

    return mutated;
  }

  CAFCorpusTestCaseRef insertCall(CAFCorpusTestCaseRef previous) noexcept {
    // TODO: Implement TestCaseMutator::insertCall.
  }

  CAFCorpusTestCaseRef removeCall(CAFCorpusTestCaseRef previous) noexcept {
    auto mutated = _corpus->createTestCase();
    auto previousSequenceLen = previous->sequence().size();
    if (previousSequenceLen == 1) {
      // TODO: Maybe change here to reject the current mutation request.
      return previous;
    }

    using index = decltype(previousSequenceLen);

    // Randomly choose an index at which the function call will be dropped.
    auto drop = random<index>(0, previousSequenceLen - 1);

    for (index i = 0; i < drop; ++i) {
      mutated->addFunctionCall(previous->sequence()[i]);
    }
    for (index i = drop + 1; i < previousSequenceLen; ++i) {
      mutated->addFunctionCall(previous->sequence()[i]);
    }

    return mutated;
  }

  TestCase mutateSequence(const TestCase& previous) noexcept {
    enum SequenceMutationStrategy {
      Squash,
      Splice,
      InsertCall,
      RemoveCall,
      _SequenceMutationStrategyMax = RemoveCall
    };

    auto strategy = static_cast<SequenceMutationStrategy>(
        random<int>(0, _SequenceMutationStrategyMax));
    switch (strategy) {
      case Squash:
        return squash(previous);
      case Splice:
        return splice(previous);
      case InsertCall:
        return insertCall(previous);
      case RemoveCall:
        return removeCall(previous);
      default:
        CAF_UNREACHABLE()
    }
  }

  CAFCorpusTestCaseRef mutateArgument(CAFCorpusTestCaseRef previous) noexcept {
    // TODO: Implement TestCaseMutator::mutateArgument.
  }

  /**
   * @brief Get a random number in range [min, max].
   *
   * @tparam T the type of the output number.
   * @param min the minimal value in output range.
   * @param max the maximal value in output range.
   * @return int the generated random number.
   */
  template <typename T>
  T random(T min = 0, T max = std::numeric_limits<T>::max()) noexcept {
    static_assert(std::is_integral<T>::value, "T is not an integral type.");
    std::uniform_int_distribution<T> dist { min, max };
    return dist(_rng);
  }

  /**
   * @brief Generate a random integer number between 0 and the size of the given container.
   *
   * @tparam Container type of the container.
   * @param c the container object.
   * @return Container::size_type type of the size of the container.
   */
  template <typename Container>
  typename Container::size_type random_index(const Container &c) noexcept {
    return random<typename Container::size_type>(0, c.size());
  }
}; // class TestCaseMutator


} // namespace caf

} // namespace <anonymous>


#undef CAF_UNREACHABLE
