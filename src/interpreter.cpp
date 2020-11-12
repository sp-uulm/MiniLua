#include "MiniLua/interpreter.hpp"

#include <string>
#include <vector>

namespace minilua {

ParseResult::operator bool() const { return !this->errors.empty(); }

// TODO incomplete
// can maybe also be included in the Interpreter::Impl
class Parser {
    std::string _source;

public:
    Parser(std::string source) : _source(std::move(source)) {}

    [[nodiscard]] auto source() const -> const std::string& { return _source; }
};

// TODO replace by tree-sitter tree OR our ast
class Tree {};

struct Interpreter::Impl {
    Parser parser;
    Tree tree;
    Environment env;

    Impl(Parser parser, Environment env) : parser(parser), env(std::move(env)) {}
};

Interpreter::Interpreter() : Interpreter("") {}
Interpreter::Interpreter(std::string initial_source_code)
    : impl(std::make_unique<Interpreter::Impl>(
          Parser(std::move(initial_source_code)), Environment())) {
    // TODO initialize parser with source code
}
Interpreter::~Interpreter() = default;

auto Interpreter::environment() const -> Environment& { return impl->env; }
auto Interpreter::source_code() const -> std::string_view { return impl->parser.source(); }
auto Interpreter::parse(std::string source_code) -> ParseResult {
    // TODO parse
    std::cout << "parse\n";
    return ParseResult();
}
void Interpreter::apply_source_change(SourceChange change) {
    // TODO apply source change
    std::cout << "apply_source_change\n";
}
auto Interpreter::evaluate() -> EvalResult {
    // TODO evaluate
    std::cout << "run\n";
    return EvalResult();
}

} // namespace minilua
