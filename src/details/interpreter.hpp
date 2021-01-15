#ifndef MINILUA_DETAILS_INTERPRETER_H
#define MINILUA_DETAILS_INTERPRETER_H

#include "../internal_env.hpp"
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
    auto run(const ts::Tree& tree, Env& env) -> EvalResult;

    auto visit_root(ts::Node node, Env& env) -> EvalResult;

    auto visit_identifier(ts::Node node, Env& env) -> std::string;

    auto visit_statement(ts::Node node, Env& env) -> EvalResult;

    auto visit_variable_declaration(ts::Node node, Env& env) -> EvalResult;
    auto visit_variable_declarator(ts::Node node, Env& env) -> std::string;

    auto visit_if_statement(ts::Node node, Env& env) -> EvalResult;
    auto visit_if_arm(ts::Cursor& cursor, Env& env) -> EvalResult;
    auto visit_elseif_statement(ts::Node node, Env& env) -> std::pair<EvalResult, bool>;
    auto visit_else_statement(ts::Node node, Env& env) -> EvalResult;

    auto visit_while_statement(ts::Node node, Env& env) -> EvalResult;

    auto visit_expression(ts::Node node, Env& env) -> EvalResult;
    auto visit_unary_operation(ts::Node node, Env& env) -> EvalResult;
    auto visit_binary_operation(ts::Node node, Env& env) -> EvalResult;
    auto visit_function_call(ts::Node node, Env& env) -> CallResult;

    auto visit_number(ts::Node node, Env& env) -> EvalResult;

private:
    [[nodiscard]] auto tracer() const -> std::ostream&;
    void
    trace_enter_node(ts::Node node, std::optional<std::string> method_name = std::nullopt) const;
    void
    trace_exit_node(ts::Node node, std::optional<std::string> method_name = std::nullopt) const;
    void trace_function_call(
        const std::string& function_name, const std::vector<Value>& arguments) const;
    void
    trace_function_call_result(const std::string& function_name, const CallResult& result) const;

    auto combine_source_changes(
        const std::optional<SourceChangeTree>& lhs, const std::optional<SourceChangeTree>& rhs)
        -> std::optional<SourceChangeTree>;

    auto enter_block(Env& env) -> Env;
};

} // namespace minilua::details

#endif
