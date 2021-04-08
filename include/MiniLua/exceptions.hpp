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

inline auto operator<<(std::ostream& o, const StackItem& self) -> std::ostream& {
    return o << "StackItem{ " << self.position << ", " << self.info << "}";
}

/**
 * @brief Exception thrown by the interpreter.
 */
class InterpreterException : public std::runtime_error {
    std::vector<StackItem> stack;

public:
    InterpreterException(const std::string& what);

    [[nodiscard]] auto with(StackItem item) const -> InterpreterException;

    void print_stacktrace(std::ostream& os) const;
};

class BadArgumentError : public std::runtime_error {
    int index;
    std::string message;

public:
    BadArgumentError(int index, const std::string& message);

    [[nodiscard]] auto get_index() const -> int;
    [[nodiscard]] auto get_message() const -> const std::string&;

    [[nodiscard]] auto with(const std::string& function_name, StackItem item) const
        -> InterpreterException;
};

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
