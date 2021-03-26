#ifndef MINILUA_DETAILS_INTERPRETER_H
#define MINILUA_DETAILS_INTERPRETER_H

#include "../internal_env.hpp"
#include "MiniLua/environment.hpp"
#include "MiniLua/interpreter.hpp"
#include "ast.hpp"
#include "tree_sitter/tree_sitter.hpp"

/**
 * @brief Users of the library should ignore this namespace. It is only usable internally.
 */
namespace minilua::details {

/**
 * Add the stdlib to the given table.
 *
 * This will only add the parts that are directly implemented in lua.
 */
void add_stdlib(Table& table);

/**
 * Internal results of the interpreter.
 *
 * Will be converted to the public minilua::EvalResult when the interpreter
 * finishes.
 *
 * NOTE: The fields are public because we only used them internally.
 */
struct EvalResult {
    /**
     * Holds the *return* values of an expession.
     *
     * This is a Vallist and not just a Value because some expressions can
     * return multiple values. For places where we only expect a single value
     * we will only take the first value of the Vallist (or nil if the list
     * is empty).
     *
     * NOTE: This might hold values that should not be accessible. Take care
     * where in the interpreter you read/forward the values.
     */
    Vallist values;

    /**
     * Flags to indicate if the interpreter should break/return.
     *
     * This is set when encountering a ` or `return` and unset when the break
     * or return is *applied* (i.e. in a loop or when returning from a function).
     */
    bool do_break;
    bool do_return;

    /**
     * Tracks optional source changes that are generated during execution.
     *
     * This will contain all changes generated from any *executed* element the
     * interpreter encounters.
     *
     * Use EvalResult::combine to easily merge multiple [EvalResults](@ref EvalResult)
     * and their source changes.
     */
    std::optional<SourceChangeTree> source_change;

    EvalResult();
    explicit EvalResult(Vallist values);
    explicit EvalResult(const CallResult&);

    /**
     * Combines two [EvalResults](@ref EvalResult) (`this` and `other`) into `this`.
     *
     * This will combine the source changes and override all other fields.
     */
    void combine(const EvalResult& other);

    // convertion operator
    operator minilua::EvalResult() const;
};

auto operator<<(std::ostream&, const EvalResult&) -> std::ostream&;

/**
 * This is in a class so we can track some state. E.g. including lua files or
 * do some caching.
 *
 * However most of the methods are basically pure functions and they all take
 * the current node they are visiting and the current environment.
 *
 * The `visit_` methods evaluate a part of the lua [AST](@ref minilua::details::ast)
 * and delegate smaller parts to other `visit_` methods. The methods return an
 * EvalResult which can be combined with other EvalResults to easily track
 * source changes (etc.). The values of expressions are all [Vallists](@ref Vallist)
 * because some expressions can return more than one value (e.g. a function call).
 *
 * `visit_block` and `visit_block_with_local_env` are helper methods to create
 * a new scope (for local variables).
 *
 * Additionally there are some `trace_` methods to help in debugging and logging.
 * And NodeTracer to trace entering and exiting the `visit_` methods.
 */
struct Interpreter {
private:
    const InterpreterConfig& config;
    ts::Parser& parser;

public:
    Interpreter(const InterpreterConfig& config, ts::Parser& parser);
    auto run(const ts::Tree& tree, Env& user_env) -> EvalResult;

private:
    /**
     * Setup the stdlib and overwrite (global) variables with the user defined
     * once.
     */
    auto setup_environment(Env& user_env) -> Env;

    auto load_stdlib() -> ts::Tree;
    void execute_stdlib(Env& env);

    /**
     * Cleans up the environment (i.e. garbage collection).
     *
     * This will call `__gc` metamethod on all tables (if they exist).
     */
    void cleanup_environment(Env& env);

    /**
     * Run a file.
     *
     * The file has to be loaded and parsed into a tree and the file
     * in Env should be set before calling this method.
     */
    auto run_file(const ts::Tree& tree, Env& env) -> EvalResult;

    auto visit_root(ast::Program program, Env& env) -> EvalResult;

    // general
    auto visit_identifier(ast::Identifier ident, Env& env) -> std::string;
    auto visit_prefix(ast::Prefix prefix, Env& env) -> EvalResult;

    // statements
    auto visit_statement(ast::Statement statement, Env& env) -> EvalResult;
    auto visit_variable_declaration(ast::VariableDeclaration decl, Env& env) -> EvalResult;
    auto visit_do_statement(ast::DoStatement stmt, Env& env) -> EvalResult;
    auto visit_function_statement(ast::FunctionStatement function_statement, Env& env)
        -> EvalResult;

    /**
     * Creates a new scope before entering the block.
     */
    auto visit_block(ast::Body block, Env& env) -> EvalResult;
    /**
     * Uses `env` as the scope for entering the block.
     *
     * Usefull for scopes that are slightly larger than the actual block
     * (e.g. repeat until statements) or *artificially* created scopes
     * (e.g. when executing a function).
     */
    auto visit_block_with_local_env(ast::Body block, Env& env) -> EvalResult;

    // control flow
    auto visit_break_statement() -> EvalResult;
    auto visit_return_statement(ast::Return return_stmt, Env& env) -> EvalResult;
    auto visit_if_statement(ast::IfStatement if_stmt, Env& env) -> EvalResult;
    auto visit_while_statement(ast::WhileStatement while_stmt, Env& env) -> EvalResult;
    auto visit_repeat_until_statement(ast::RepeatStatement repeat_stmt, Env& env) -> EvalResult;

    // expressions
    auto visit_expression(ast::Expression expr, Env& env) -> EvalResult;
    auto visit_unary_operation(ast::UnaryOperation unary_op, Env& env) -> EvalResult;
    auto visit_binary_operation(ast::BinaryOperation bin_op, Env& env) -> EvalResult;
    auto visit_function_call(ast::FunctionCall call, Env& env) -> EvalResult;
    auto visit_field_expression(ast::FieldExpression field_expression, Env& env) -> EvalResult;
    auto visit_table_index(ast::TableIndex table_index, Env& env) -> EvalResult;
    auto visit_function_expression(ast::FunctionDefinition function_definition, Env& env)
        -> EvalResult;
    auto visit_vararg_expression(Env& env) -> EvalResult;

    auto visit_table_constructor(ast::Table table_constructor, Env& env) -> EvalResult;
    auto visit_literal(ast::Literal literal, Env& env) -> EvalResult;

    /**
     * Evaluates a list of expressions (like return values, function parameters, etc).
     *
     * The result contains a vallist with all the evaluated values. If the expression evaluate to
     * a Vallist only the first value is taken (except for the last expression). If the last
     * expression evaluates to a Vallist it will be appended.
     *
     * Example:
     *
     * Expressions evaluate to (here `{}` denotes a Vallist):
     *
     * ```lua
     * {42, 2, 3}, "hi", nil, 5, {22, 17}, {7, 8, 9}
     * ```
     *
     * The result will be:
     *
     * ```lua
     * 42, "hi", nil, 5, 22, 7, 8, 9
     * ```
     */
    auto visit_expression_list(std::vector<ast::Expression> expressions, Env& env) -> EvalResult;
    auto visit_parameter_list(std::vector<ast::Identifier> raw_params, Env& env)
        -> std::vector<std::string>;

    // helper methods for debugging/tracing
    [[nodiscard]] auto tracer() const -> std::ostream&;
    void
    trace_enter_node(ts::Node node, std::optional<std::string> method_name = std::nullopt) const;
    void trace_exit_node(
        ts::Node node, std::optional<std::string> method_name = std::nullopt,
        std::optional<std::string> reason = std::nullopt) const;
    void trace_function_call(ast::Prefix prefix, const std::vector<Value>& arguments) const;
    void trace_function_call_result(ast::Prefix prefix, const CallResult& result) const;
    void trace_exprlists(std::vector<ast::Expression>& exprlist, const Vallist& result) const;
    void trace_metamethod_call(const std::string& name, const Vallist& arguments) const;

    void trace_enter_block(Env& env);

    /**
     * Helper class that will trace entry and exit of a method.
     *
     * # Usage
     *
     * Add the following to be beginning of the method you want to trace:
     *
     * ```cpp
     * auto _ = NodeTracer(this, ast_element.raw(), "method_name");
     * ```
     *
     * The constructor will call Interpreter::trace_enter_node and the destructor (which
     * is always run before exiting the method) will call Interpreter::trace_exit_node.
     */
    class NodeTracer {
        Interpreter& interpreter;
        ts::Node node;
        std::optional<std::string> method_name;

    public:
        NodeTracer(
            Interpreter* interpreter, ts::Node node,
            std::optional<std::string> method_name = std::nullopt);
        ~NodeTracer();
    };

    friend struct FunctionImpl;
};

/**
 * The *implementation* of a lua function.
 *
 * This is a bit more ergonomic than creating a lambda.
 */
struct FunctionImpl {
    ast::Body body;
    /**
     * Store a copy of the environment to correctly capture the local variables
     * that are accessible when creating the function.
     */
    Env env;
    std::vector<std::string> parameters;
    bool vararg;
    Interpreter& interpreter;

    auto operator()(const CallContext& ctx) -> CallResult;
};

} // namespace minilua::details

#endif
