#ifndef MINILUA_DETAILS_INTERPRETER_H
#define MINILUA_DETAILS_INTERPRETER_H

#include "../internal_env.hpp"
#include "MiniLua/environment.hpp"
#include "MiniLua/interpreter.hpp"
#include "ast.hpp"
#include "tree_sitter/tree_sitter.hpp"

namespace minilua::details {

struct EvalResult {
    Value value;
    bool do_break;
    std::optional<Vallist> do_return;
    std::optional<SourceChangeTree> source_change;

    EvalResult();
    explicit EvalResult(const CallResult&);

    /**
     * Combines another 'EvalResult' into this one.
     *
     * This will combine the source changes and override the other fields.
     */
    void combine(const EvalResult& other);

    operator minilua::EvalResult() const;
};

auto operator<<(std::ostream&, const EvalResult&) -> std::ostream&;

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

private:
    auto visit_root(ast::Program program, Env& env) -> EvalResult;

    auto visit_identifier(ast::Identifier ident, Env& env) -> std::string;

    auto visit_statement(ast::Statement statement, Env& env) -> EvalResult;

    auto visit_variable_declaration(ast::VariableDeclaration decl, Env& env) -> EvalResult;

    auto visit_break_statement(Env& env) -> EvalResult;
    auto visit_return_statement(ast::Return return_stmt, Env& env) -> EvalResult;

    auto visit_do_statement(ast::DoStatement stmt, Env& env) -> EvalResult;

    auto visit_block(ast::Body block, Env& env) -> EvalResult;
    auto visit_block_with_local_env(ast::Body block, Env& env) -> EvalResult;

    auto visit_if_statement(ast::IfStatement if_stmt, Env& env) -> EvalResult;

    auto visit_while_statement(ast::WhileStatement while_stmt, Env& env) -> EvalResult;
    auto visit_repeat_until_statement(ast::RepeatStatement repeat_stmt, Env& env) -> EvalResult;

    auto visit_prefix(ast::Prefix prefix, Env& env) -> EvalResult;

    auto visit_expression(ts::Node node, Env& env) -> EvalResult;
    auto visit_unary_operation(ts::Node node, Env& env) -> EvalResult;
    auto visit_binary_operation(ts::Node node, Env& env) -> EvalResult;
    auto visit_function_call(ast::FunctionCall call, Env& env) -> EvalResult;
    auto visit_field_expression(ts::Node node, Env& env) -> EvalResult;
    auto visit_function_expression(ts::Node node, Env& env) -> EvalResult;
    auto visit_vararg_expression(ts::Node node, Env& env) -> EvalResult;

    auto visit_table_constructor(ts::Node node, Env& env) -> EvalResult;

    // helper methods for debugging/tracing
    [[nodiscard]] auto tracer() const -> std::ostream&;
    void
    trace_enter_node(ts::Node node, std::optional<std::string> method_name = std::nullopt) const;
    void trace_exit_node(
        ts::Node node, std::optional<std::string> method_name = std::nullopt,
        std::optional<std::string> reason = std::nullopt) const;
    void trace_function_call(
        const std::string& function_name, const std::vector<Value>& arguments) const;
    void
    trace_function_call_result(const std::string& function_name, const CallResult& result) const;

    auto enter_block(Env& env) -> Env;
};

} // namespace minilua::details

#endif
