#include "MiniLua/exceptions.hpp"

namespace minilua {

// class InterpreterException
InterpreterException::InterpreterException(const std::string& what) : std::runtime_error(what) {}

// class BadArgumentError
BadArgumentError::BadArgumentError(int index, const std::string& what)
    : InterpreterException(what), index(index) {}
auto BadArgumentError::format(const std::string& function_name) const -> std::string {
    // "bad argument #{index} '{function_name}' ({msg})"
    return "bad argument #" + std::to_string(this->index) + " '" + function_name + "' (" +
           this->what() + ")";
}

} // namespace minilua
