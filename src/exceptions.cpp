#include <utility>

#include "MiniLua/exceptions.hpp"

namespace minilua {

// class InterpreterException
InterpreterException::InterpreterException(const std::string& what) : std::runtime_error(what) {}

auto InterpreterException::with(StackItem item) const -> InterpreterException {
    InterpreterException e = *this;
    e.stack.push_back(std::move(item));
    return e;
}

/**
 * lua: luaprograms/errors.lua:10: some error
        stack traceback:
        [C]: in function 'error'
        luaprograms/errors.lua:10: in function 'func3'
        luaprograms/errors.lua:6: in function 'func2'
        luaprograms/errors.lua:2: in function 'func1'
        luaprograms/errors.lua:13: in main chunk
        [C]: in ?
 */
void InterpreterException::print_stacktrace(std::ostream& os) const {
    for (const auto& item : this->stack) {
        if (item.position.file) {
            os << *item.position.file;
        } else {
            os << "<unknown>";
        }

        os << ":" << item.position.start.line + 1;

        os << ": in " << item.info << "\n";
    }
}

// class BadArgumentError
// NOTE: we generate a proper error message in the constructor so this exception
// is also useful to catch from C++, we need to create the string in the
// constructor because of the return type of std::exception::what()
BadArgumentError::BadArgumentError(int index, const std::string& message)
    : std::runtime_error("bad argument #" + std::to_string(index) + "(" + message + ")"),
      index(index), message(message) {}

auto BadArgumentError::get_index() const -> int { return this->index; }
auto BadArgumentError::get_message() const -> const std::string& { return this->message; }

auto BadArgumentError::with(const std::string& function_name, StackItem item) const
    -> InterpreterException {
    // "bad argument #{index} '{function_name}' ({msg})"
    auto message = "bad argument #" + std::to_string(this->index) + " '" + function_name + "' (" +
                   this->message + ")";
    return InterpreterException(message).with(std::move(item));
}

} // namespace minilua
