#ifndef MINILUA_DETAILS_INTERPRETER_H
#define MINILUA_DETAILS_INTERPRETER_H

#include "MiniLua/environment.hpp"
#include "MiniLua/interpreter.hpp"
#include "tree_sitter/tree_sitter.hpp"

namespace minilua::details {

/**
 * This is in a class so we can track some state. E.g. including lua files or
 * do some caching.
 *
 * However most of the methods are basically pure functions and they all take
 * the current node they are visiting and the current environment.
 */
struct Interpreter {
    const InterpreterConfig& config;

public:
    Interpreter(const InterpreterConfig& config);
    auto run(const ts::Tree& tree, Environment& env) -> EvalResult;

    auto visit_root(ts::Node node, Environment& env) -> EvalResult;

    auto visit_identifier(ts::Node node, Environment& env) -> std::string;

    auto visit_variable_declaration(ts::Node node, Environment& env) -> EvalResult;
    auto visit_variable_declarator(ts::Node node, Environment& env) -> std::string;

    auto visit_expression(ts::Node node, Environment& env) -> EvalResult;
    auto visit_number(ts::Node node, Environment& env) -> EvalResult;

    auto visit_function_call(ts::Node node, Environment& env) -> CallResult;

private:
    [[nodiscard]] auto tracer() const -> std::ostream&;
    void trace_enter_node(ts::Node node) const;
    void trace_exit_node(ts::Node node) const;
    void trace_function_call(
        const std::string& function_name, const std::vector<Value>& arguments) const;
    void
    trace_function_call_result(const std::string& function_name, const CallResult& result) const;

    auto combine_source_changes(
        const std::optional<SourceChangeTree>& lhs, const std::optional<SourceChangeTree>& rhs)
        -> std::optional<SourceChangeTree>;
};

} // namespace minilua::details

#endif
