#ifndef CAF_PRINTER_H
#define CAF_PRINTER_H

#include <iostream>
#include <utility>

namespace caf {

enum class PrinterColor {
  Black         = 30,
  Red           = 31,
  Green         = 32,
  Yellow        = 33,
  Blue          = 34,
  Magenta       = 35,
  Cyan          = 36,
  White         = 37,
  BrightBlack   = 90,
  BrightRed     = 91,
  BrightGreen   = 92,
  BrightYellow  = 93,
  BrightBlue    = 94,
  BrightMagenta = 95,
  BrightCyan    = 96,
  BrightWhite   = 97,
};

/**
 * @brief Provide a state machine to print formatted text on terminal.
 *
 */
class Printer {
public:
  /**
   * @brief Provide a RAII type that keeps the current indentation level until values of this type
   * are destructed.
   *
   */
  class IndentGuard {
  public:
    IndentGuard(const IndentGuard &) = delete;
    IndentGuard(IndentGuard &&) noexcept = default;

    ~IndentGuard() { _printer.PopIndent(); }

    friend class Printer;

  private:
    Printer& _printer;

    explicit IndentGuard(Printer& printer)
      : _printer(printer)
    { }
  };

  /**
   * @brief Provide a RAII type that keeps the current
   *
   */
  class ColorGuard {
  public:
    ColorGuard(const ColorGuard &) = delete;
    ColorGuard(ColorGuard &&) noexcept = default;

    ~ColorGuard() { _printer.ClearColor(); }

    friend class Printer;

  private:
    Printer& _printer;

    explicit ColorGuard(Printer& printer)
      : _printer(printer)
    { }
  }; // class ColorGuard

  /**
   * @brief A placeholder type for new line characters.
   *
   */
  struct EndLine { };

  /**
   * @brief The new line character placeholder.
   *
   */
  static const EndLine endl;

  /**
   * @brief Construct a new Printer object.
   *
   * @param out the output stream.
   */
  explicit Printer(std::ostream& out)
    : _out(out),
      _color(true),
      _indentWidth(2)
  { }

  /**
   * @brief Set whether to colorize the output.
   *
   * @param color should we colorize the output?
   */
  void SetColorOn(bool color = true) { _color = color; }

  /**
   * @brief Set the width of a single indentation level.
   *
   * @param width the width of a single indentation level.
   */
  void SetIndentWidth(int width) { _indentWidth = width; }

  /**
   * @brief Create a new indentation level.
   *
   * @return IndentGuard a RAII type that keeps the created indentation level until it is
   * destructed.
   */
  IndentGuard PushIndent();

  /**
   * @brief Set the foreground color.
   *
   * @param color the foreground color.
   * @return ColorGuard a RAII guard that keeps the given color set until it is destructed.
   */
  ColorGuard SetForegroundColor(PrinterColor color);

  /**
   * @brief Set the background color.
   *
   * @param color the background color.
   * @return ColorGuard a RAII guard that keeps the given color set until it is destructed.
   */
  ColorGuard SetBackgroundColor(PrinterColor color);

  /**
   * @brief Set the foreground and background color.
   *
   * @param fg the foreground color.
   * @param bg the background color.
   * @return ColorGuard a RAII guard that keeps the given color set until it is destructed.
   */
  ColorGuard SetColor(PrinterColor fg, PrinterColor bg);

  /**
   * @brief Output the given value to this printer.
   *
   * @tparam T the type of the value.
   * @param value the value to output.
   * @return Printer& the current Printer object.
   */
  template <typename T>
  Printer& operator<<(T&& value) {
    Print(std::forward<T>(value));
    return *this;
  }

  /**
   * @brief Output a new line.
   *
   * @return Printer& the current Printer object.
   */
  Printer& operator<<(const EndLine &) {
    PrintLine();
    return *this;
  }

  /**
   * @brief Output the given value to this printer.
   *
   * @tparam T the type of the value.
   * @param value the value to output.
   * @return Printer& the current Printer object.
   */
  template <typename T>
  Printer& Print(T&& value) {
    if (_startOfLine) {
      _startOfLine = false;
      auto indentWidth = _indentWidth * _indentLevel;
      while (indentWidth--) {
        _out << ' ';
      }
    }
    _out << value;
    return *this;
  }

  /**
   * @brief Output a new line character.
   *
   * @return Printer& the current Printer object.
   */
  Printer& PrintLine();

  /**
   * @brief Output the given values to this printer, then write a new line.
   *
   * @tparam T types of the values to output.
   * @param values values to output.
   * @return Printer& the current Printer object.
   */
  template <typename ...T>
  Printer& PrintLine(T&&... values) {
    Print(std::forward<T>(values)...);
    operator<<(endl);
    return *this;
  }

  /**
   * @brief Output the given values to this printer.
   *
   * @tparam Head the type of the first value in the value list.
   * @tparam Tail the types of the values except the first value in the value list.
   * @param h the first value.
   * @param t the rest values in the value list.
   * @return Printer& the current Printer object.
   */
  template <typename Head, typename ...Tail>
  Printer& Print(Head&& h, Tail&&... t) {
    Print(h);
    Print(std::forward<Tail>(t)...);
    return *this;
  }

  /**
   * @brief Output the given valus with the given foreground color.
   *
   * @tparam T types of the values to output.
   * @param fg the foreground color.
   * @param values the values to output.
   * @return Printer& the current Printer object.
   */
  template <typename ...T>
  Printer& PrintWithColor(PrinterColor fg, T&&... values) {
    auto guard = SetForegroundColor(fg);
    Print(std::forward<T>(values)...);
    return *this;
  }

  /**
   * @brief Output the given values with the given foreground color. Then write a new line
   * character.
   *
   * @tparam T types of the values to output.
   * @param fg the foreground color.
   * @param values the values to output.
   * @return Printer& the current Printer object.
   */
  template <typename ...T>
  Printer& PrintLineWithColor(PrinterColor fg, T&&... values) {
    PrintWithColor(fg, std::forward<T>(values)...);
    PrintLine();
    return *this;
  }

private:
  std::ostream& _out;
  bool _color;
  int _indentWidth;
  int _indentLevel;
  bool _startOfLine;

  /**
   * @brief Remove an indentation level.
   *
   */
  void PopIndent();

  /**
   * @brief Clear color settings.
   *
   */
  void ClearColor();
}; // class Printer

} // namespace caf

#endif
