#include "MiniLua/interpreter.hpp"
#include "details/interpreter.hpp"
#include "tree_sitter/tree_sitter.hpp"

#include <string>
#include <utility>
#include <vector>

namespace minilua {

// struct ParseResult
ParseResult::operator bool() const { return this->errors.empty(); }

// struct EvalResult
EvalResult::EvalResult() = default;
EvalResult::EvalResult(const CallResult& call_result)
    : value(std::get<0>(call_result.values().tuple<1>())),
      source_change(call_result.source_change()) {}
auto operator<<(std::ostream& o, const EvalResult& self) -> std::ostream& {
    o << "EvalResult{ .value = " << self.value << ", .source_change = ";

    if (self.source_change.has_value()) {
        o << *self.source_change;
    } else {
        o << "nullopt";
    }

    return o << "}";
}

// struct InterpreterConfig
InterpreterConfig::InterpreterConfig()
    : target(&std::cerr), trace_nodes(false), trace_calls(false) {}
InterpreterConfig::InterpreterConfig(bool def)
    : target(&std::cerr), trace_nodes(def), trace_calls(def) {}
void InterpreterConfig::all(bool def) {
    this->trace_nodes = def;
    this->trace_calls = def;
}

// class InterpreterException
InterpreterException::InterpreterException(const std::string& what) : std::runtime_error(what) {}

struct Interpreter::Impl {
    ts::Parser parser;       // NOLINT(misc-non-private-member-variables-in-classes)
    std::string source_code; // NOLINT(misc-non-private-member-variables-in-classes)
    ts::Tree tree;           // NOLINT(misc-non-private-member-variables-in-classes)
    Environment env;         // NOLINT(misc-non-private-member-variables-in-classes)

    Impl(std::string initial_source_code, Environment env)
        : source_code(std::move(initial_source_code)), tree(parser.parse_string(this->source_code)),
          env(std::move(env)) {}
};

Interpreter::Interpreter() : Interpreter("") {}
Interpreter::Interpreter(std::string initial_source_code)
    : impl(std::make_unique<Interpreter::Impl>(std::move(initial_source_code), Environment())) {}
Interpreter::~Interpreter() = default;

auto Interpreter::config() -> InterpreterConfig& { return this->_config; }
[[nodiscard]] auto Interpreter::config() const -> const InterpreterConfig& { return this->_config; }
void Interpreter::set_config(InterpreterConfig config) { this->_config = config; }

auto Interpreter::environment() const -> Environment& { return impl->env; }
auto Interpreter::source_code() const -> std::string_view { return impl->source_code; }
auto Interpreter::parse(std::string source_code) -> ParseResult {
    this->impl->source_code = std::move(source_code);
    this->impl->tree = this->impl->parser.parse_string(this->impl->source_code);

    ParseResult result;
    if (this->impl->tree.root_node().has_error()) {
        // TODO properly collect errors from the tree
        result.errors.emplace_back("Tree contains parse error");
    }
    return result;
}
void Interpreter::apply_source_changes(std::vector<SourceChange> source_changes) {
    // TODO apply source change
    std::cout << "apply_source_changes\n";
}
auto Interpreter::evaluate() -> EvalResult {
    details::Interpreter interpreter{this->config()};
    return interpreter.run(this->impl->tree, this->impl->env);
}

} // namespace minilua
