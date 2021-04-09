#include <algorithm>
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

static void print_range(const std::optional<Range>& range, std::ostream& os) {
    if (range.has_value()) {
        if (range->file) {
            os << *range->file;
        } else {
            os << "<unknown>";
        }
        os << ":" << range->start.line + 1;
    } else {
        os << "[NATIVE]";
    }
}

void InterpreterException::print_stacktrace(std::ostream& os) const {
    // Separate ranges and infos
    // We need to print the range and associated info offset so the information
    // is accurate.
    //
    // We have stack items of the form (range, "in function f")
    // but the range is the location where function f is called and we want to
    // print the range inside of f followed by "in function f".
    //
    // To achieve this the offset we insert a nullopt at the beginning of ranges.
    // This also signals that the exception was thrown in native code (which is
    // always true).
    std::vector<std::optional<Range>> ranges;
    ranges.reserve(this->stack.size() + 1);
    ranges.emplace_back(std::nullopt);
    std::transform(
        this->stack.begin(), this->stack.end(), std::back_inserter(ranges),
        [](const auto& item) { return item.position; });

    // We also insert "main chunk" in infos to keep the length the same. This is
    // always true because the first call to any function must occur in the main
    // chunk of a program (even if we internally don't call it "chunks").
    std::vector<std::string> infos;
    infos.reserve(this->stack.size() + 1);
    std::transform(
        this->stack.begin(), this->stack.end(), std::back_inserter(infos),
        [](const auto& item) { return item.info; });
    infos.emplace_back("main chunk");

    for (size_t i = 0; i < ranges.size(); ++i) {
        os << "\t";
        print_range(ranges[i], os);
        os << ": in " << infos[i] << "\n";
    }
}

// class BadArgumentError
// NOTE: we generate a proper error message in the constructor so this exception
// is also useful to catch from C++, we need to create the string in the
// constructor because of the return type of std::exception::what()
BadArgumentError::BadArgumentError(int index, const std::string& message)
    : InterpreterException("bad argument #" + std::to_string(index) + "(" + message + ")"),
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

auto operator<<(std::ostream& o, const StackItem& self) -> std::ostream& {
    return o << "StackItem{ " << self.position << ", " << self.info << "}";
}

} // namespace minilua
