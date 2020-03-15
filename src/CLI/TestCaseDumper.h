#ifndef CAF_TEST_CASE_DUMPER_H
#define CAF_TEST_CASE_DUMPER_H

#include "Printer.h"

#include <cstdint>

namespace caf {

class CAFStore;
class TestCase;
class FunctionCall;
class Value;

/**
 * @brief Provide methods to dump test cases.
 *
 */
class TestCaseDumper {
public:
  /**
   * @brief Construct a new TestCaseDumper object.
   *
   * @param store the metadata store.
   * @param printer the printer.
   */
  explicit TestCaseDumper(CAFStore& store, Printer& printer)
    : _store(store),
      _printer { printer }
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

  CAFStore& _store;
  Printer& _printer;
  bool _demangle;

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

  /**
   * @brief Dump the given function call.
   *
   * @param value the function call to dump.
   * @param context the dump context.
   */
  void DumpFunctionCall(const FunctionCall& value, DumpContext& context);

  /**
   * @brief Dump the given value.
   *
   * @param value the argument to dump.
   * @param context the dump context.
   */
  void DumpValue(const Value& value, DumpContext& context);

  /**
   * @brief Dump the given string value.
   *
   * @param s the string to dump.
   */
  void DumpStringValue(const char* s);

  /**
   * @brief Dump the given integer value.
   *
   * @param value the integer value to dump.
   */
  void DumpIntegerValue(int32_t value);
}; // class TestCaseDumper

} // namespace caf

#endif
