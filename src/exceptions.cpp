#include <utility>

#include "MiniLua/exceptions.hpp"

namespace minilua {

// class InterpreterException
InterpreterException::InterpreterException(const std::string& what) : std::runtime_error(what) {}

auto InterpreterException::with(StackItem item) const -> InterpreterException {
    auto e = InterpreterException(this->what());
    e.stack.push_back(std::move(item));
    return e;
}

// class BadArgumentError
BadArgumentError::BadArgumentError(int index, const std::string& what)
    : std::runtime_error(what), index(index) {}

auto BadArgumentError::with(const std::string& function_name, StackItem item) const
    -> InterpreterException {
    // "bad argument #{index} '{function_name}' ({msg})"
    auto message = "bad argument #" + std::to_string(this->index) + " '" + function_name + "' (" +
                   this->what() + ")";
    return InterpreterException(message).with(std::move(item));
}

} // namespace minilua
