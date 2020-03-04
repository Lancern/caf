#include "TestCaseDumper.h"
#include "Infrastructure/Casting.h"
#include "Infrastructure/Identity.h"
#include "Infrastructure/Intrinsic.h"
#include "Basic/Function.h"
#include "Basic/BitsType.h"
#include "Basic/StructType.h"
#include "Basic/AggregateType.h"
#include "Fuzzer/FunctionCall.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/Value.h"
#include "Fuzzer/BitsValue.h"
#include "Fuzzer/PointerValue.h"
#include "Fuzzer/FunctionValue.h"
#include "Fuzzer/ArrayValue.h"
#include "Fuzzer/StructValue.h"
#include "Fuzzer/AggregateValue.h"
#include "Fuzzer/PlaceholderValue.h"

#include <cxxabi.h>

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

  for (auto callId = 0; callId < tc.GetFunctionCallCount(); ++callId) {
    const auto& call = tc.GetFunctionCall(callId);

    _printer.PrintWithColor(KeywordColor, "CALL ");
    _printer << "#" << callId << ": ";
    DumpFunctionCall(call, context);
    if (callId != tc.GetFunctionCallCount() - 1) {
      _printer << Printer::endl;
    }
  }
}

void TestCaseDumper::DumpFunctionCall(const FunctionCall& value, DumpContext& context) {
  _printer << "A" << value.func()->id() << " ";
  DumpSymbolName(value.func()->name().c_str());

  // Print arguments
  for (size_t i = 0; i < value.GetArgCount(); ++i) {
    _printer << Printer::endl;
    auto arg = value.GetArg(i);
    auto indentGuard = _printer.PushIndent();

    // Print: ARG #<index>: $<slot>
    _printer.PrintWithColor(KeywordColor, "ARG ");
    _printer << "#" << i << ": ";
    DumpValue(*arg, context);
  }

  // The next value index is reserved for the return value of the current function.
  context.SkipNextValue();
}

void TestCaseDumper::DumpValue(const Value& value, DumpContext& context) {
  _printer << "T" << value.type()->id() << " $" << context.PeekNextValueIndex() << ": ";
  if (context.HasValue(&value)) {
    _printer.PrintWithColor(KeywordColor, "XREF ");
    _printer << "$" << context.GetValueIndex(&value);
    context.SkipNextValue();
  } else {
    if (value.kind() != ValueKind::PlaceholderValue) {
      context.SetNextValue(&value);
    } else {
      context.SkipNextValue();
    }

    switch (value.kind()) {
      case ValueKind::BitsValue: {
        DumpBitsValue(caf::dyn_cast<BitsValue>(value), context);
        break;
      }
      case ValueKind::PointerValue: {
        DumpPointerValue(caf::dyn_cast<PointerValue>(value), context);
        break;
      }
      case ValueKind::FunctionValue: {
        DumpFunctionValue(caf::dyn_cast<FunctionValue>(value), context);
        break;
      }
      case ValueKind::ArrayValue: {
        DumpArrayValue(caf::dyn_cast<ArrayValue>(value), context);
        break;
      }
      case ValueKind::StructValue: {
        DumpStructValue(caf::dyn_cast<StructValue>(value), context);
        break;
      }
      case ValueKind::AggregateValue: {
        DumpAggregateValue(caf::dyn_cast<AggregateValue>(value), context);
        break;
      }
      case ValueKind::PlaceholderValue: {
        DumpPlaceholderValue(caf::dyn_cast<PlaceholderValue>(value), context);
        break;
      }
      default:
        CAF_UNREACHABLE;
    }
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

void TestCaseDumper::DumpBitsValue(const BitsValue& value, DumpContext& context) {
  auto type = caf::dyn_cast<BitsType>(value.type());
  _printer.PrintWithColor(ValueTypeColor, "Bits ");
  if (type->name().empty()) {
    _printer << "(no name)";
  } else {
    _printer << type->name();
  }
  _printer << " size = " << value.size() << " ";
  DumpHex(value.data(), value.size());
}

void TestCaseDumper::DumpPointerValue(const PointerValue& value, DumpContext& context) {
  _printer.PrintWithColor(ValueTypeColor, "Pointer");
  _printer << Printer::endl;
  auto indentGuard = _printer.PushIndent();
  _printer.PrintWithColor(KeywordColor, "POINTEE ");
  DumpValue(*value.pointee(), context);
}

void TestCaseDumper::DumpFunctionValue(const FunctionValue& value, DumpContext& context) {
  _printer.PrintWithColor(ValueTypeColor, "Function");
  _printer << " function ID = " << value.functionId();
}

void TestCaseDumper::DumpArrayValue(const ArrayValue& value, DumpContext& context) {
  _printer.PrintWithColor(ValueTypeColor, "Array");
  _printer << " size = " << value.size();
  auto indentGuard = _printer.PushIndent();
  for (size_t i = 0; i < value.size(); ++i) {
    auto element = value.GetElement(i);
    _printer << Printer::endl;
    _printer.PrintWithColor(KeywordColor, "ELEM ");
    _printer << "#" << i << ": ";
    DumpValue(*element, context);
  }
}

void TestCaseDumper::DumpStructValue(const StructValue& value, DumpContext& context) {
  auto type = caf::dyn_cast<StructType>(value.type());

  _printer.PrintWithColor(ValueTypeColor, "Struct ");
  if (type->name().empty()) {
    _printer << "(no name)";
  } else {
    DumpSymbolName(type->name().c_str());
  }
  _printer << Printer::endl;

  auto indentGuard1 = _printer.PushIndent();
  _printer.PrintWithColor(KeywordColor, "CTOR ");
  _printer << "#" << value.ctor()->id();

  auto indentGuard2 = _printer.PushIndent();
  for (size_t i = 0; i < value.GetArgsCount(); ++i) {
    _printer << Printer::endl;
    _printer.PrintWithColor(KeywordColor, "ARG ");
    _printer << "#" << i << ": ";
    DumpValue(*value.GetArg(i), context);
  }
}

void TestCaseDumper::DumpAggregateValue(const AggregateValue& value, DumpContext& context) {
  auto type = caf::dyn_cast<AggregateType>(value.type());

  _printer.PrintWithColor(ValueTypeColor, "Aggregate ");
  if (type->name().empty()) {
    _printer << "(no name)";
  } else {
    DumpSymbolName(type->name().c_str());
  }

  auto indentGuard = _printer.PushIndent();
  for (size_t i = 0; i < value.GetFieldsCount(); ++i) {
    _printer << Printer::endl;
    _printer.PrintWithColor(KeywordColor, "FIELD ");
    _printer << "#" << i << ": ";
    DumpValue(*value.GetField(i), context);
  }
}

void TestCaseDumper::DumpPlaceholderValue(const PlaceholderValue& value, DumpContext& context) {
  _printer.PrintWithColor(ValueTypeColor, "Placeholder ");
  _printer.PrintWithColor(KeywordColor, "REF CALL ");
  _printer << "#" << value.valueIndex();
}

} // namespace caf
