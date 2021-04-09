#ifndef MINILUA_EXCEPTIONS_HPP
#define MINILUA_EXCEPTIONS_HPP

#include "MiniLua/source_change.hpp"
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace minilua {

struct StackItem {
    Range position;
    std::string info;
};

auto operator<<(std::ostream& o, const StackItem& self) -> std::ostream&;

/**
 * @brief Exception thrown by the interpreter.
 *
 * This exception can contain a stack trace.
 */
class InterpreterException : public std::runtime_error {
    std::vector<StackItem> stack;

public:
    InterpreterException(const std::string& what);

    /**
     * @brief Create a new exception with the given stack item added.
     */
    [[nodiscard]] auto with(StackItem item) const -> InterpreterException;

    /**
     * @brief Print the stacktrace to the output stream.
     */
    void print_stacktrace(std::ostream& os) const;
};

/**
 * @brief Exception indicating a bad argument of a function call.
 *
 * This is usually thrown when validating an argument.
 *
 * The interpreter has special code to format this exception and print the name
 * of the called function.
 */
class BadArgumentError : public InterpreterException {
    int index;
    std::string message;

public:
    /**
     * @brief Create a new BadArgumentError.
     *
     * @param index The index of the bad argument (starting with 1)
     * @param message Error information for the bad argument
     */
    BadArgumentError(int index, const std::string& message);

    [[nodiscard]] auto get_index() const -> int;
    [[nodiscard]] auto get_message() const -> const std::string&;

    /**
     * @brief Create a new exception with the given function name and stack item.
     */
    [[nodiscard]] auto with(const std::string& function_name, StackItem item) const
        -> InterpreterException;
};

/**
 * @brief Execute the given function and correctly re-throw exceptions using the
 * given function name and stack item.
 */
template <typename Fn>
auto with_call_stack(Fn f, const std::string& function_name, const StackItem& item) {
    try {
        return f();
    } catch (const BadArgumentError& e) {
        throw e.with(function_name, item);
    } catch (const InterpreterException& e) {
        throw e.with(item);
    } catch (const std::exception& e) {
        throw InterpreterException(e.what()).with(item);
    }
}

} // namespace minilua

#endif // MINILUA_EXCEPTIONS_HPP
