#include "MiniLua/interpreter.hpp"
#include "details/interpreter.hpp"
#include "details/tree_sitter_interop.hpp"
#include "tree_sitter/tree_sitter.hpp"
#include "tree_sitter_lua.hpp"

#include <algorithm>
#include <iterator>
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
        : parser(ts::LUA_LANGUAGE), source_code(std::move(initial_source_code)),
          tree(parser.parse_string(this->source_code)),
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
auto Interpreter::source_code() const -> const std::string& { return impl->source_code; }

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

auto Interpreter::apply_source_changes(std::vector<SourceChange> source_changes)
    -> std::unordered_map<Range, Range> {
    std::vector<ts::Edit> edits;
    edits.reserve(source_changes.size());

    std::transform(
        source_changes.begin(), source_changes.end(), std::back_inserter(edits), to_ts_edit);

    auto edit_result = this->impl->tree.edit(edits);

    this->impl->source_code = this->impl->tree.source();

    std::unordered_map<Range, Range> range_map;
    for (const auto& applied_edit : edit_result.applied_edits) {
        range_map[from_ts_range(applied_edit.before)] = from_ts_range(applied_edit.after);
    }
    return range_map;
}
auto Interpreter::evaluate() -> EvalResult {
    details::Interpreter interpreter{this->config(), this->impl->parser};
    return interpreter.run(this->impl->tree, this->impl->env.get_raw_impl().inner);
}

} // namespace minilua
