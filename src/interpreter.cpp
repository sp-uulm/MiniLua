#include "MiniLua/interpreter.hpp"

#include <string>
#include <vector>

namespace minilua {

class Parser {
    std::string _source;

public:
    Parser(std::string source) : _source(std::move(source)) {}

    [[nodiscard]] auto source() const -> const std::string& {
        return _source;
    }
};

class Tree {};

struct Interpreter::Impl {
    Parser parser;
    Tree tree;
    Environment env;

    Impl(Parser parser, Environment env) : parser(parser), env(std::move(env)) {}

    void parse(std::string source) {
        // TODO
        std::cout << "parse\n";
    }

    void apply_source_changes(std::vector<SourceChange> changes) {
        // TODO
        std::cout << "apply_source_changes\n";
    }

    auto run() -> EvalResult {
        std::cout << "run\n";
        // TODO
        return EvalResult();
    }
};

Interpreter::Interpreter() : Interpreter("") {}
Interpreter::Interpreter(std::string initial_source_code)
    : impl(std::make_unique<Interpreter::Impl>(
          Parser(std::move(initial_source_code)), Environment())) {
    // TODO initialize parser with source code
}
Interpreter::~Interpreter() = default;

auto Interpreter::environment() const -> Environment& {
    return impl->env;
}
auto Interpreter::source_code() const -> std::string_view {
    return impl->parser.source();
}
void Interpreter::parse(std::string source_code) {
    impl->parse(std::move(source_code));
}
void Interpreter::apply_source_changes(std::vector<SourceChange> changes) {
    impl->apply_source_changes(std::move(changes));
}
auto Interpreter::run() -> EvalResult {
    return impl->run();
}

} // namespace minilua
