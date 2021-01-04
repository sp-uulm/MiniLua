#include "MiniLua/interpreter.hpp"
#include "tree_sitter/tree_sitter.hpp"

#include <string>
#include <utility>
#include <vector>

namespace minilua {

ParseResult::operator bool() const { return this->errors.empty(); }

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
    // TODO evaluate
    std::cout << "run\n";
    return EvalResult();
}

} // namespace minilua
