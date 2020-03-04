#include "Printer.h"

#include <cassert>

namespace caf {

namespace {

int GetForegroundCode(PrinterColor color) {
  return static_cast<int>(color);
}

int GetBackgroundCode(PrinterColor color) {
  return static_cast<int>(color) + 10;
}

template <typename Output>
void ClearColorCode(Output& out) {
  out << "\x1b[m";
}

template <typename Output>
void SetColorCode(Output& out, int code) {
  out << "\x1b[" << code << 'm';
}

} // namespace <anonymous>

const Printer::EndLine Printer::endl;

Printer::IndentGuard Printer::PushIndent() {
  ++_indentLevel;
  return Printer::IndentGuard { *this };
}

Printer::ColorGuard Printer::SetForegroundColor(PrinterColor color) {
  if (_color) {
    SetColorCode(_out, GetForegroundCode(color));
  }
  return Printer::ColorGuard { *this };
}

Printer::ColorGuard Printer::SetBackgroundColor(PrinterColor color) {
  if (_color) {
    SetColorCode(_out, GetBackgroundCode(color));
  }
  return Printer::ColorGuard { *this };
}

Printer::ColorGuard Printer::SetColor(PrinterColor fg, PrinterColor bg) {
  if (_color) {
    SetColorCode(_out, GetForegroundCode(fg));
    SetColorCode(_out, GetBackgroundCode(bg));
  }
  return Printer::ColorGuard { *this };
}

Printer& Printer::PrintLine() {
  Print('\n');
  _startOfLine = true;
  return *this;
}

void Printer::PopIndent() {
  --_indentLevel;
  assert(_indentLevel >= 0 && "Inbalance indentation level.");
}

void Printer::ClearColor() {
  ClearColorCode(_out);
}

} // namespace caf
