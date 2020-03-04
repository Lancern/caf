#ifndef CAF_TEST_CASE_DUMPER_H
#define CAF_TEST_CASE_DUMPER_H

#include "Printer.h"

namespace caf {

class TestCase;
class FunctionCall;
class Value;
class BitsValue;
class PointerValue;
class FunctionValue;
class ArrayValue;
class StructValue;
class AggregateValue;
class PlaceholderValue;

/**
 * @brief Provide methods to dump test cases.
 *
 */
class TestCaseDumper {
public:
  /**
   * @brief Construct a new TestCaseDumper object.
   *
   * @param printer the printer.
   */
  explicit TestCaseDumper(Printer& printer)
    : _printer { printer }
  { }

  /**
   * @brief Set whether to demangle the symbols before printing them.
   *
   * @param demangle whether to demangle the symbols.
   */
  void SetDemangle(bool demangle = true) { _demangle = demangle; }

  /**
   * @brief Dump the given test case to standard output stream.
   *
   * @param tc the test case to dump.
   */
  void Dump(const TestCase& tc);

private:
  class DumpContext;

  Printer& _printer;
  bool _demangle;

  /**
   * @brief Dump the given value.
   *
   * @param value the argument to dump.
   * @param context the dump context.
   */
  void DumpValue(const Value& value, DumpContext& context);

  /**
   * @brief Dump the given symbol name. This function will demangle the given symbol name if
   * required.
   *
   * @param name the symbol name to dump.
   */
  void DumpSymbolName(const char* name);

  /**
   * @brief Dump the given binary data.
   *
   * @param buffer pointer to the head of the data buffer.
   * @param size size of the data buffer, in bytes.
   */
  void DumpHex(const void* buffer, size_t size);

  void DumpFunctionCall(const FunctionCall& value, DumpContext& context);

  void DumpBitsValue(const BitsValue& value, DumpContext& context);

  void DumpPointerValue(const PointerValue& value, DumpContext& context);

  void DumpFunctionValue(const FunctionValue& value, DumpContext& context);

  void DumpArrayValue(const ArrayValue& value, DumpContext& context);

  void DumpStructValue(const StructValue& value, DumpContext& context);

  void DumpAggregateValue(const AggregateValue& value, DumpContext& context);

  void DumpPlaceholderValue(const PlaceholderValue& value, DumpContext& context);
}; // class TestCaseDumper

} // namespace caf

#endif
