#include "MiniLua/interpreter.hpp"
#include "details/interpreter.hpp"
#include "tree_sitter/tree_sitter.hpp"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace minilua {

// struct ParseResult
ParseResult::operator bool() const { return this->errors.empty(); }

// struct EvalResult
EvalResult::EvalResult() = default;
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
InterpreterConfig::InterpreterConfig() : target(&std::cerr) { this->all(false); }
InterpreterConfig::InterpreterConfig(bool def) : InterpreterConfig() { this->all(def); }
void InterpreterConfig::all(bool def) {
    this->trace_nodes = def;
    this->trace_calls = def;
    this->trace_enter_block = def;
    this->trace_exprlists = def;
    this->trace_break = def;
    this->trace_varargs = def;
}

// class InterpreterException
InterpreterException::InterpreterException(const std::string& what) : std::runtime_error(what) {}

struct Interpreter::Impl {
    ts::Parser parser;
    std::string source_code;
    ts::Tree tree;
    std::unique_ptr<MemoryAllocator> allocator;
    Environment env;

    Impl(std::string initial_source_code)
        : source_code(std::move(initial_source_code)), tree(parser.parse_string(this->source_code)),
          allocator(std::make_unique<MemoryAllocator>()), env(allocator.get()) {}

    ~Impl() { allocator->free_all(); }
};

Interpreter::Interpreter() : Interpreter("") {}
Interpreter::Interpreter(std::string initial_source_code)
    : impl(std::make_unique<Interpreter::Impl>(std::move(initial_source_code))) {}
Interpreter::~Interpreter() = default;

auto Interpreter::config() -> InterpreterConfig& { return this->_config; }
[[nodiscard]] auto Interpreter::config() const -> const InterpreterConfig& { return this->_config; }
void Interpreter::set_config(InterpreterConfig config) { this->_config = config; }

auto Interpreter::environment() const -> Environment& { return impl->env; }
auto Interpreter::source_code() const -> std::string_view { return impl->source_code; }

static auto visit_siblings(ts::Cursor& cursor, const std::function<void(ts::Node)>& fn) -> bool;
static auto visit_children(ts::Cursor& cursor, const std::function<void(ts::Node)>& fn) -> bool {
    if (cursor.goto_first_child()) {
        fn(cursor.current_node());
    }
    return visit_siblings(cursor, fn);
};

static auto visit_siblings(ts::Cursor& cursor, const std::function<void(ts::Node)>& fn) -> bool {
    while (cursor.goto_next_sibling()) {
        fn(cursor.current_node());
        visit_children(cursor, fn);
    }
    return cursor.goto_parent();
};

// TODO refactor this and make it more usable
template <typename Fn> static void visit_tree(const ts::Tree& tree, Fn fn) {
    ts::Cursor cursor(tree);

    fn(cursor.current_node());
    visit_children(cursor, fn);
}

auto Interpreter::parse(std::string source_code) -> ParseResult {
    this->impl->source_code = std::move(source_code);
    this->impl->tree = this->impl->parser.parse_string(this->impl->source_code);

    ParseResult result;
    if (this->impl->tree.root_node().has_error()) {
        result.errors.emplace_back("Tree contains parse error");

        visit_tree(this->impl->tree, [&result](ts::Node node) {
            if (node.type() == "ERROR"s || node.is_missing()) {
                std::stringstream error;
                error << "Error in node: ";
                error << ts::debug_print_node(node);
                result.errors.emplace_back(error.str());
            }
        });
    }
    return result;
}
void Interpreter::apply_source_changes(std::vector<SourceChange> source_changes) {
    // TODO apply source change
    std::cout << "apply_source_changes\n";
}
auto Interpreter::evaluate() -> EvalResult {
    details::Interpreter interpreter{this->config()};
    return interpreter.run(this->impl->tree, this->impl->env.get_raw_impl().inner);
}

} // namespace minilua
