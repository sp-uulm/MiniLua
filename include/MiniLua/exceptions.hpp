#ifndef MINILUA_EXCEPTIONS_HPP
#define MINILUA_EXCEPTIONS_HPP

#include "MiniLua/source_change.hpp"
#include <memory>
#include <stdexcept>
#include <vector>

namespace minilua {

struct StackItem {
    Range position;
    std::string info;
};

/**
 * @brief Exception thrown by the interpreter.
 */
class InterpreterException : public std::runtime_error {
    std::vector<StackItem> stack;

public:
    InterpreterException(const std::string& what);

    [[nodiscard]] auto with(StackItem item) const -> InterpreterException;
};

class BadArgumentError : public std::runtime_error {
    int index;

public:
    BadArgumentError(int index, const std::string& what);

    [[nodiscard]] auto with(const std::string& function_name, StackItem item) const
        -> InterpreterException;
};

} // namespace minilua

#endif // MINILUA_EXCEPTIONS_HPP
