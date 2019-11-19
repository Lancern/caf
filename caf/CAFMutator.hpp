#include "CAFMeta.hpp"

#include <cstdint>
#include <climits>
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
        << __func__                     \
        << "@" << __FILE__              \
        << ":" << __LINE__              \
        << std::endl;                   \
    std::terminate();                   \
  }


namespace {

namespace rng {

/**
 * @brief Get a random number in range [min, max].
 *
 * @tparam T the type of the output number.
 * @tparam RNG the type of the random number generator. This type should satisfy C++ named
 * requirement `RandomNumberEngine`.
 * @param r reference to the random number generator.
 * @param min the minimal value in output range.
 * @param max the maximal value in output range.
 * @return int the generated random number.
 */
template <typename T, typename RNG>
T random(RNG& r, T min = 0, T max = std::numeric_limits<T>::max()) noexcept {
  static_assert(std::is_integral<T>::value, "T is not an integral type.");
  std::uniform_int_distribution<T> dist { min, max };
  return dist(r);
}

/**
 * @brief Generate a random integer number between 0 and the size of the given container.
 *
 * @tparam RNG the type of the random number generator. This type should satisfy C++ named
 * requirement `RandomNumberEngine`.
 * @tparam Container type of the container. The container type should satisfy C++ named
 * requirement `Container`.
 * @param r reference to the random number generator.
 * @param c the container object.
 * @return Container::size_type type of the size of the container.
 */
template <typename RNG, typename Container>
typename Container::size_type random_index(RNG& r, const Container &c) noexcept {
  return random<typename Container::size_type>(r, 0, c.size());
}

/**
 * @brief Randomly select an element from the given container.
 *
 * @tparam RNG the type of the random number generator. This type should satisfy C++ named
 * requirement `RandomNumberEngine`.
 * @tparam Container type of the container. The container type should satisfy C++ named
 * requirement `SequenceContainer`.
 * @param r reference to the random number generator.
 * @param c the container instance.
 * @return Container::const_reference const reference to the selected element.
 */
template <typename RNG, typename Container>
typename Container::const_reference select(RNG& r, const Container& c) noexcept {
  return c[random_index(r, c)];
}

} // namespace rng

namespace caf {

class CAFObjectPool;
class CAFCorpusTestCaseRef;
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
   * @brief Determine whether the object pool is empty.
   *
   * @return true if the object pool is empty.
   * @return false if the object pool is not empty.
   */
  bool empty() const noexcept {
    return size() == 0;
  }

  /**
   * @brief Create a new value in the object pool.
   *
   * @tparam T the type of the value to be created. T should derive from `Value`.
   * @tparam Args the types of the arguments for constructing a new value.
   * @param args arguments for constructing a new value.
   * @return T* pointer to the created value.
   */
  template <typename T, typename ...Args>
  T* create(Args&&... args) {
    static_assert(std::is_base_of<Value, T>::value, "T does not derive from Value.");
    _values.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    return dynamic_cast<T *>(_values.back().get());
  }

  template <typename T>
  friend class CAFObjectPoolValueRef;

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
  /**
   * @brief Type of the index.
   *
   */
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

  /**
   * @brief Get the corpus instance owning the test case.
   *
   * @return CAFCorpus* the corpus instance owning the test case.
   */
  CAFCorpus* corpus() const noexcept {
    return _corpus;
  }

  /**
   * @brief Get the index of the referenced `TestCase` instance inside the corpus.
   *
   * @return index_type the index of the referenced `TestCase` instance.
   */
  index_type index() const noexcept {
    return _index;
  }

  TestCase& operator*() const noexcept {
    return *get();
  }

  TestCase* operator->() const noexcept {
    return get();
  }

  inline TestCase* get() const noexcept;

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

  /**
   * @brief Get the object pool associated with the given type ID.
   *
   * @param typeId the type ID of the object pool.
   * @return CAFObjectPool* pointer to the desired object pool. If no such object pool is found,
   * `nullptr` will be returned.
   */
  CAFObjectPool* getObjectPool(uint64_t typeId) const noexcept {
    auto i = _pools.find(typeId);
    if (i == _pools.end()) {
      return nullptr;
    }

    return i->second.get();
  }

  /**
   * @brief Get the object pool associated with the given type name. If the desired object pool does
   * not exist, then create it.
   *
   * @param typeId the type ID of the object pool.
   * @return CAFObjectPool* pointer to the object pool.
   */
  CAFObjectPool* getOrCreateObjectPool(uint64_t typeId) noexcept {
    auto pool = getObjectPool(typeId);
    if (pool) {
      return pool;
    }

    auto createdPool = std::make_unique<CAFObjectPool>();
    return _pools.emplace(typeId, std::move(createdPool)).first->second.get();
  }

  friend class CAFCorpusTestCaseRef;

private:
  std::unique_ptr<CAFStore> _store;
  std::unordered_map<uint64_t, std::unique_ptr<CAFObjectPool>> _pools;
  std::vector<TestCase> _testCases;
};

TestCase* CAFCorpusTestCaseRef::get() const noexcept {
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
   * @brief Set the element value at the given index.
   *
   * @param index the index of the element within the array.
   * @param value the value to be set.
   */
  void setElement(typename std::vector<Value *>::size_type index, Value* value) noexcept {
    _elements[index] = value;
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
   * @brief Set the argument value at the given index.
   *
   * @param index the index of the argument.
   * @param value the value of the argument to set.
   */
  void setArg(typename std::vector<Value *>::size_type index, Value* value) noexcept {
    _args[index] = value;
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
   * @param func the function to be called.
   */
  explicit FunctionCall(const Function* func) noexcept
    : _func(func),
      _args { }
  { }

  /**
   * @brief Get the function called.
   *
   * @return const Function* pointer to the function definition to be called.
   */
  const Function* func() const noexcept {
    return _func;
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
   * @brief Set the argument at the given index to the specified value.
   *
   * @param index the index of the argument.
   * @param value the value of the argument.
   */
  void setArg(typename std::vector<Value *>::size_type index, Value* value) noexcept {
    _args[index] = value;
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
  const Function* _func;
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
    writeTrivial(o, funcCall.func()->id());

    // Write number of arguments.
    writeTrivial(o, funcCall.args().size());

    // Write arguments.
    for (const auto& arg : funcCall.args()) {
      write(o, *arg);
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
  explicit TestCaseMutator(CAFCorpus* corpus, RNG rng = RNG()) noexcept
    : _corpus(corpus),
      _rng { std::move(rng) }
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
    enum MutationStrategy : int {
      Sequence,
      Argument,
      _MutationStrategyMax = Argument
    };
    auto strategy = static_cast<MutationStrategy>(
        rng::random<int>(_rng, 0, static_cast<int>(_MutationStrategyMax)));
    switch (strategy) {
      case Sequence:
        return mutateSequence(testCase);
      case Argument:
        return mutateArgument(testCase);
      default:
        CAF_UNREACHABLE()
    }
  }

private:
  CAFCorpus* _corpus;
  RNG _rng;

  using sequence_index_type = typename std::vector<FunctionCall>::size_type;

  CAFCorpusTestCaseRef squash(CAFCorpusTestCaseRef previous) noexcept {
    // TODO: Implement TestCaseMutator::squash.
    CAF_UNREACHABLE()
  }

  CAFCorpusTestCaseRef splice(CAFCorpusTestCaseRef previous) noexcept {
    auto source = _corpus->getTestCase(rng::random_index(_rng, _corpus->testCases()));
    auto previousLen = previous->sequence().size();
    auto sourceLen = source->sequence().size();

    auto spliceIndex = rng::random<sequence_index_type>(_rng, 0, std::min(previousLen, sourceLen));

    auto mutated = _corpus->createTestCase();
    for (sequence_index_type i = 0; i < spliceIndex; ++i) {
      mutated->addFunctionCall(previous->sequence()[i]);
    }
    for (sequence_index_type i = spliceIndex; i < sourceLen; ++i) {
      mutated->addFunctionCall(source->sequence()[i]);
    }

    return mutated;
  }

  Value* generateNewBitsType(const BitsType* type) noexcept {
    auto objectPool = _corpus->getObjectPool(type->id());
    if (!objectPool) {
      // TODO: Refactor here to reject current value generation request.
      return nullptr;
    }

    return rng::select(_rng, objectPool->values()).get();
  }

  Value* generateNewPointerType(const PointerType* type) noexcept {
    auto objectPool = _corpus->getOrCreateObjectPool(type->id());
    auto pointeeValue = generateValue(type->pointeeType().get());
    return objectPool->create<PointerValue>(objectPool, pointeeValue, type);
  }

  Value* generateNewArrayType(const ArrayType* type) noexcept {
    std::vector<Value *> elements { };
    elements.reserve(type->size());
    for (size_t i = 0; i < type->size(); ++i) {
      elements.push_back(generateValue(type->elementType().get()));
    }

    auto objectPool = _corpus->getOrCreateObjectPool(type->id());
    return objectPool->create<ArrayValue>(objectPool, type, std::move(elements));
  }

  Value* generateNewStructType(const StructType* type) noexcept {
    // Choose an activator.
    auto activator = rng::select(_rng, type->activators()).get();
    const auto& activatorArgs = activator->signature().args();

    // Generate arguments passed to the activator. Note that the first argument to the activator
    // is a pointer to the constructing object and thus should not be generated here.
    std::vector<Value *> args { };
    args.reserve(activatorArgs.size() - 1);
    for (size_t i = 1; i < activatorArgs.size(); ++i) {
      args.push_back(generateValue(activatorArgs[i].get()));
    }

    auto objectPool = _corpus->getOrCreateObjectPool(type->id());
    return objectPool->create<StructValue>(objectPool, type, activator, std::move(args));
  }

  /**
   * @brief Generate a new value of the given type.
   *
   * @param type type of the value to be generated.
   * @return Value* pointer to the generated new value.
   */
  Value* generateNewValue(const Type* type) noexcept {
    switch (type->kind()) {
      case TypeKind::Bits: {
        return generateNewBitsType(dynamic_cast<const BitsType *>(type));
      }
      case TypeKind::Pointer: {
        return generateNewPointerType(dynamic_cast<const PointerType *>(type));
      }
      case TypeKind::Array: {
        return generateNewArrayType(dynamic_cast<const ArrayType *>(type));
      }
      case TypeKind::Struct: {
        return generateNewStructType(dynamic_cast<const StructType *>(type));
      }
      default: CAF_UNREACHABLE()
    }
  }

  /**
   * @brief Generate an argument of the given type.
   *
   * This function randomly applies the following strategies:
   * * Select an exiting value from the corresponding object pool in the corpus, if any;
   * * Create a new value and insert it into the corresponding object pool; any related values used
   * to construct the new value will be generated recursively until `BitsValue` is required. The
   * required `BitsValue` will be populated from the corresponding object pool directly.
   *
   * @param type the type of the argument.
   * @return Value* pointer to the generated `Value` instance wrapping around the argument.
   */
  Value* generateValue(const Type* type) noexcept {
    auto objectPool = _corpus->getOrCreateObjectPool(type->id());
    if (objectPool->empty()) {
      // Does not make sense to select existing value from the object pool since it is empty.
      return generateNewValue(type);
    }

    enum GenerateValueStrategies : int {
      UseExisting,
      CreateNew,
      _GenerateValueStrategiesMax = CreateNew
    };

    auto strategy = static_cast<GenerateValueStrategies>(
        rng::random<int>(_rng, 0, static_cast<int>(_GenerateValueStrategiesMax)));
    switch (strategy) {
      case UseExisting: {
        // Randomly choose an existing value from the object pool.
        return rng::select(_rng, objectPool->values()).get();
      }
      case CreateNew: {
        return generateNewValue(type);
      }
      default: CAF_UNREACHABLE()
    }
  }

  /**
   * @brief Generate a function call. The callee is selected randomly within the `CAFStore` instance
   * in the corpus and the arguments are generated recursively.
   *
   * @return FunctionCall the generated function call.
   */
  FunctionCall generateCall() noexcept {
    auto api = rng::select(_rng, _corpus->store()->apis()).get();

    FunctionCall call { api };
    for (auto arg : api->signature().args()) {
      call.addArg(generateValue(arg.get()));
    }

    return call;
  }

  CAFCorpusTestCaseRef insertCall(CAFCorpusTestCaseRef previous) noexcept {
    auto insertIndex = rng::random_index(_rng, previous->sequence());
    auto mutated = _corpus->createTestCase();
    for (sequence_index_type i = 0; i < insertIndex; ++i) {
      mutated->addFunctionCall(previous->sequence()[i]);
    }
    mutated->addFunctionCall(generateCall());
    for (sequence_index_type i = insertIndex; i < previous->sequence().size(); ++i) {
      mutated->addFunctionCall(previous->sequence()[i]);
    }

    return mutated;
  }

  CAFCorpusTestCaseRef removeCall(CAFCorpusTestCaseRef previous) noexcept {
    auto mutated = _corpus->createTestCase();
    auto previousSequenceLen = previous->sequence().size();
    if (previousSequenceLen == 1) {
      // TODO: Maybe change here to reject the current mutation request.
      return previous;
    }

    // Randomly choose an index at which the function call will be dropped.
    auto drop = rng::random<sequence_index_type>(_rng, 0, previousSequenceLen - 1);

    for (sequence_index_type i = 0; i < drop; ++i) {
      mutated->addFunctionCall(previous->sequence()[i]);
    }
    for (sequence_index_type i = drop + 1; i < previousSequenceLen; ++i) {
      mutated->addFunctionCall(previous->sequence()[i]);
    }

    return mutated;
  }

  CAFCorpusTestCaseRef mutateSequence(CAFCorpusTestCaseRef previous) noexcept {
    enum SequenceMutationStrategy : int {
      Squash,
      Splice,
      InsertCall,
      RemoveCall,
      _SequenceMutationStrategyMax = RemoveCall
    };

    auto strategy = static_cast<SequenceMutationStrategy>(
        rng::random<int>(_rng, 0, static_cast<int>(_SequenceMutationStrategyMax)));
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

  void flipBits(uint8_t* buffer, size_t size, size_t width) noexcept {
    auto offset = rng::random<size_t>(_rng, 0, size * CHAR_BIT - width);
    buffer[offset / CHAR_BIT] ^= ((1 << width) - 1) << (offset % CHAR_BIT);
  }

  void flipBytes(uint8_t* buffer, size_t size, size_t width) noexcept {
    auto offset = rng::random<size_t>(_rng, 0, size - width);
    switch (width) {
      case 1:
        buffer[offset] ^= 0xff;
        break;
      case 2:
        *reinterpret_cast<uint16_t *>(buffer + offset) ^= 0xffff;
        break;
      case 4:
        *reinterpret_cast<uint32_t *>(buffer + offset) ^= 0xffffffff;
        break;
      default:
        CAF_UNREACHABLE()
    }
  }

  void arith(uint8_t* buffer, size_t size, size_t width) noexcept {
    auto offset = rng::random<size_t>(_rng, 0, size - width);
    // TODO: Add code here to allow user modify the maximal absolute value of arithmetic delta.
    auto delta = rng::random<int32_t>(_rng, -35, 35);
    switch (width) {
      case 1:
        *reinterpret_cast<int8_t *>(buffer + offset) += static_cast<int8_t>(delta);
        break;
      case 2:
        *reinterpret_cast<int16_t *>(buffer + offset) += static_cast<int16_t>(delta);
        break;
      case 4:
        *reinterpret_cast<int32_t *>(buffer + offset) += static_cast<int32_t>(delta);
        break;
      default:
        CAF_UNREACHABLE()
    }
  }

  Value* mutateBitsValue(const BitsValue* value) noexcept {
    auto objectPool = value->pool();
    auto mutated = objectPool->create<BitsValue>(*value);

    enum BitsValueMutationStrategy : int {
      BitFlip1,
      BitFlip2,
      BitFlip4,
      ByteFlip1,
      ByteFlip2,
      ByteFlip4,
      ByteArith,
      WordArith,
      DWordArith,
      _MutationStrategyMax = DWordArith
    };

    std::vector<BitsValueMutationStrategy> valid {
      BitFlip1,
      BitFlip2,
      BitFlip4,
      ByteFlip1,
      ByteArith
    };
    if (value->size() >= 2) {
      valid.push_back(ByteFlip2);
      valid.push_back(WordArith);
    }
    if (value->size() >= 4) {
      valid.push_back(ByteFlip4);
      valid.push_back(DWordArith);
    }

    auto strategy = rng::select(_rng, valid);
    switch (strategy) {
      case BitFlip1: {
        flipBits(mutated->data(), mutated->size(), 1);
        break;
      }
      case BitFlip2: {
        flipBits(mutated->data(), mutated->size(), 2);
        break;
      }
      case BitFlip4: {
        flipBits(mutated->data(), mutated->size(), 4);
        break;
      }
      case ByteFlip1: {
        flipBytes(mutated->data(), mutated->size(), 1);
        break;
      }
      case ByteFlip2: {
        flipBytes(mutated->data(), mutated->size(), 2);
        break;
      }
      case ByteFlip4: {
        flipBytes(mutated->data(), mutated->size(), 4);
        break;
      }
      case ByteArith: {
        arith(mutated->data(), mutated->size(), 1);
        break;
      }
      case WordArith: {
        arith(mutated->data(), mutated->size(), 2);
        break;
      }
      case DWordArith: {
        arith(mutated->data(), mutated->size(), 4);
        break;
      }
      default: CAF_UNREACHABLE()
    }

    return mutated;
  }

  Value* mutatePointerValue(const PointerValue* value) noexcept {
    auto objectPool = value->pool();
    auto mutatedPointee = mutateValue(value->pointee());
    return objectPool->create<PointerValue>(objectPool, mutatedPointee,
        dynamic_cast<const PointerType *>(value->type()));
  }

  Value* mutateArrayValue(const ArrayValue* value) noexcept {
    auto elementIndex = rng::random_index(_rng, value->elements());
    auto mutatedElement = mutateValue(value->elements()[elementIndex]);

    auto objectPool = value->pool();
    auto mutated = objectPool->create<ArrayValue>(*value);
    mutated->setElement(elementIndex, mutatedElement);

    return mutated;
  }

  Value* mutateStructValue(const StructValue* value) noexcept {
    enum StructValueMutationStrategy : int {
      MutateActivator,
      MutateArgument,
      _StrategyMax = MutateArgument
    };

    auto strategy = MutateActivator;
    if (!value->args().empty()) {
      strategy = static_cast<StructValueMutationStrategy>(rng::random<int>(_rng, 0, _StrategyMax));
    }

    switch (strategy) {
      case MutateActivator: {
        return generateNewStructType(dynamic_cast<const StructType *>(value->type()));
      }
      case MutateArgument: {
        auto argIndex = rng::random_index(_rng, value->args());
        auto mutatedArg = mutateValue(value->args()[argIndex]);
        auto objectPool = value->pool();
        auto mutated = objectPool->create<StructValue>(*value);
        mutated->setArg(argIndex, mutatedArg);
        return mutated;
      }
      default: CAF_UNREACHABLE()
    }
  }

  /**
   * @brief Mutate the given value.
   *
   * @param value the value to be mutated.
   * @return Value* pointer to the mutated value.
   */
  Value* mutateValue(const Value* value) noexcept {
    switch (value->kind()) {
      case ValueKind::BitsValue: {
        return mutateBitsValue(dynamic_cast<const BitsValue *>(value));
      }
      case ValueKind::PointerValue: {
        return mutatePointerValue(dynamic_cast<const PointerValue *>(value));
      }
      case ValueKind::ArrayValue: {
        return mutateArrayValue(dynamic_cast<const ArrayValue *>(value));
      }
      case ValueKind::StructValue: {
        return mutateStructValue(dynamic_cast<const StructValue *>(value));
      }
      default: CAF_UNREACHABLE()
    }
  }

  CAFCorpusTestCaseRef mutateArgument(CAFCorpusTestCaseRef previous) noexcept {
    auto functionCallIndex = rng::random_index(_rng, previous->sequence());
    auto mutated = _corpus->createTestCase();
    for (sequence_index_type i = 0; i < functionCallIndex; ++i) {
      mutated->addFunctionCall(previous->sequence()[i]);
    }

    auto targetCall = previous->sequence()[functionCallIndex];
  if (targetCall.args().empty()) {
      // TODO: Handle this case when the selected function call does not have any arguments.
      return previous;
    } else {
      auto argIndex = rng::random_index(_rng, targetCall.args());
      auto value = targetCall.args()[argIndex];
      value = mutateValue(value);
      targetCall.setArg(argIndex, value);
    }

    for (sequence_index_type i = functionCallIndex + 1; i < previous->sequence().size(); ++i) {
      mutated->addFunctionCall(previous->sequence()[i]);
    }

    return mutated;
  }
}; // class TestCaseMutator


} // namespace caf

} // namespace <anonymous>


#undef CAF_UNREACHABLE
