#ifndef MINILUA_EXCEPTIONS_HPP
#define MINILUA_EXCEPTIONS_HPP

#include <stdexcept>

namespace minilua {

/**
 * @brief Exception thrown by the interpreter.
 */
class InterpreterException : public std::runtime_error {
public:
    InterpreterException(const std::string& what);
};

class BadArgumentError : public InterpreterException {
    int index;

public:
    BadArgumentError(int index, const std::string& what);

    [[nodiscard]] auto format(const std::string& function_name) const -> std::string;
};

} // namespace minilua

#endif // MINILUA_EXCEPTIONS_HPP
