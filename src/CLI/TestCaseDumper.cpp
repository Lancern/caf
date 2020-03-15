#include "TestCaseDumper.h"
#include "Infrastructure/Casting.h"
#include "Infrastructure/Identity.h"
#include "Infrastructure/Intrinsic.h"
#include "Basic/CAFStore.h"
#include "Basic/Function.h"
#include "Fuzzer/FunctionCall.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/Value.h"

#include <cxxabi.h>

#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <memory>

namespace caf {

namespace {

char GetHexDigit(uint8_t decimal) {
  assert(decimal < 16 && "decimal should be less than 16.");
  if (decimal < 10) {
    return '0' + decimal;
  } else {
    return 'a' + decimal - 10;
  }
}

} // namespace <anonymous>

constexpr static const PrinterColor KeywordColor = PrinterColor::BrightGreen;
constexpr static const PrinterColor ValueTypeColor = PrinterColor::BrightCyan;
constexpr static const PrinterColor SpecialValueColor = PrinterColor::Yellow;

class TestCaseDumper::DumpContext {
public:
  explicit DumpContext() = default;

  DumpContext(const DumpContext &) = delete;
  DumpContext(DumpContext &&) noexcept = default;

  DumpContext& operator=(const DumpContext &) = delete;
  DumpContext& operator=(DumpContext &&) = default;

  bool HasValue(const Value* value) const {
    return _valueToIndex.find(value) != _valueToIndex.end();
  }

  size_t GetValueIndex(const Value* value) const {
    return _valueToIndex.at(value);
  }

  void SetNextValue(const Value* value) {
    auto id = _valueIndexAlloc.next();
    _valueToIndex[value] = id;
  }

  void SkipNextValue() {
    _valueIndexAlloc.next();
  }

  size_t PeekNextValueIndex() const {
    return _valueIndexAlloc.peek();
  }

private:
  std::unordered_map<const Value *, size_t> _valueToIndex;
  caf::IncrementIdAllocator<size_t> _valueIndexAlloc;
}; // class TestCaseDumper::DumpContext

void TestCaseDumper::Dump(const TestCase& tc) {
  DumpContext context;

  for (size_t callId = 0; callId < tc.GetFunctionCallsCount(); ++callId) {
    const auto& call = tc.GetFunctionCall(callId);

    _printer.PrintWithColor(KeywordColor, "CALL ");
    _printer << "#" << callId << ": ";
    DumpFunctionCall(call, context);
    if (callId != tc.GetFunctionCallsCount() - 1) {
      _printer << Printer::endl;
    }
  }
}

void TestCaseDumper::DumpFunctionCall(const FunctionCall& value, DumpContext& context) {
  const auto& func = _store.GetFunction(value.funcId());
  _printer << "A" << value.funcId() << " ";
  DumpSymbolName(func.name().c_str());

  // Print `this` value.
  if (value.HasThis()) {
    _printer << Printer::endl;
    auto indentGuard = _printer.PushIndent();

    // Print: THIS:
    _printer.PrintWithColor(KeywordColor, "THIS");
    _printer << ": ";
    DumpValue(*value.GetThis(), context);
  }

  // Print arguments
  auto indentGuard = _printer.PushIndent();
  for (size_t i = 0; i < value.GetArgsCount(); ++i) {
    _printer << Printer::endl;
    auto arg = value.GetArg(i);

    // Print: ARG #<index>:
    _printer.PrintWithColor(KeywordColor, "ARG");
    _printer << " #" << i << ": ";
    DumpValue(*arg, context);
  }

  _printer << Printer::endl;
  _printer.PrintWithColor(KeywordColor, "RET");
  _printer << " @ $" << context.PeekNextValueIndex();

  // The next value index is reserved for the return value of the current function.
  context.SkipNextValue();
}

void TestCaseDumper::DumpValue(const Value& value, DumpContext& context) {
  if (context.HasValue(&value)) {
    PlaceholderValue ref { context.GetValueIndex(&value) };
    DumpValue(ref, context);
    return;
  }

  switch (value.kind()) {
    case ValueKind::Undefined:
      _printer.PrintWithColor(ValueTypeColor, "Undefined");
      break;
    case ValueKind::Null:
      _printer.PrintWithColor(ValueTypeColor, "Null");
      break;
    case ValueKind::Function:
      _printer.PrintWithColor(ValueTypeColor, "Function");
      break;
    case ValueKind::Boolean:
      _printer.PrintWithColor(ValueTypeColor, "Boolean");
      _printer << " ";
      if (value.GetBooleanValue()) {
        _printer.PrintWithColor(SpecialValueColor, "true");
      } else {
        _printer.PrintWithColor(SpecialValueColor, "false");
      }
      break;
    case ValueKind::String:
      _printer.PrintWithColor(ValueTypeColor, "String");
      _printer << " ";
      DumpStringValue(value.GetStringValue().c_str());
      break;
    case ValueKind::Integer:
      _printer.PrintWithColor(ValueTypeColor, "Integer");
      _printer << " ";
      DumpIntegerValue(value.GetIntegerValue());
      break;
    case ValueKind::Float:
      _printer.PrintWithColor(ValueTypeColor, "Float");
      _printer << " " << value.GetFloatValue();
      break;
    case ValueKind::Array: {
      auto slot = context.PeekNextValueIndex();
      context.SetNextValue(&value);
      const auto& arrayValue = caf::dyn_cast<ArrayValue>(value);
      _printer.PrintWithColor(ValueTypeColor, "Array");
      _printer << " $" << slot;
      _printer << " [" << arrayValue.size() << "]";

      auto indentGuard = _printer.PushIndent();
      for (size_t i = 0; i < arrayValue.size(); ++i) {
        _printer << Printer::endl;
        _printer.PrintWithColor(KeywordColor, "[");
        _printer << i;
        _printer.PrintWithColor(KeywordColor, "]");
        _printer << " ";
        DumpValue(*arrayValue.GetElement(i), context);
      }

      break;
    }
    case ValueKind::Placeholder:
      _printer.PrintWithColor(KeywordColor, "Placeholder");
      _printer << " ";
      _printer.PrintWithColor(KeywordColor, "REF");
      _printer << " $" << value.GetPlaceholderIndex();
      break;
    default:
      CAF_UNREACHABLE;
  }
}

void TestCaseDumper::DumpSymbolName(const char* name) {
  int demangleStatus;
  if (_demangle) {
    auto demangled = abi::__cxa_demangle(name, 0, 0, &demangleStatus);
    if (demangled) {
      name = demangled;
    }
  }

  _printer << name;
  if (_demangle && demangleStatus == 0) {
    // Demangle had been successful. We need to free the memory region allocated by the demangler.
    std::free(const_cast<char *>(name));
  }
}

void TestCaseDumper::DumpHex(const void* data, size_t size) {
  auto ptr = reinterpret_cast<const uint8_t *>(data);
  _printer << "{";
  while (size--) {
    auto b = *ptr++;
    _printer << ' ' << GetHexDigit(b / 16) << GetHexDigit(b % 16);
  }
  _printer << " }";
}

void TestCaseDumper::DumpStringValue(const char* s) {
  _printer << "\"";
  while (*s) {
    auto ch = *s++;
    if (std::isprint(ch)) {
      if (ch == '"') {
        _printer << "\\\"";
      } else {
        _printer << ch;
      }
    } else {
      if (ch == '\n') {
        _printer << "\\n";
      } else if (ch == '\t') {
        _printer << "\\t";
      } else if (ch == '\r') {
        _printer << "\\r";
      } else {
        _printer << "\\x"
                 << GetHexDigit(static_cast<uint8_t>(ch) / 16)
                 << GetHexDigit(static_cast<uint8_t>(ch) % 16);
      }
    }
  }
  _printer << "\"";
}

void TestCaseDumper::DumpIntegerValue(int32_t value) {
  _printer << value << ' ';

  uint8_t xdgt[8];
  uint8_t* head = xdgt;

  auto uvalue = static_cast<uint32_t>(value);
  for (auto i = 0; i < 8; ++i) {
    *head++ = static_cast<uint8_t>(uvalue % 16);
    uvalue /= 16;
  }

  _printer << "0x";
  while (head != xdgt) {
    _printer << GetHexDigit(*--head);
  }
}

} // namespace caf
