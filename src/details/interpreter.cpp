#include "interpreter.hpp"
#include "MiniLua/environment.hpp"
#include "MiniLua/interpreter.hpp"
#include "ast.hpp"
#include "tree_sitter/tree_sitter.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <utility>

namespace minilua::details {

// TODO remove the unimplemented stuff once everything is implemented
class UnimplementedException : public InterpreterException {
public:
    UnimplementedException(const std::string& where, const std::string& what)
        : InterpreterException("unimplemented: \"" + what + "\" in " + where) {}
};

#define UNIMPLEMENTED(what)                                                                        \
    UnimplementedException(                                                                        \
        std::string(__func__) + " (" + std::string(__FILE__) + ":" + std::to_string(__LINE__) +    \
            ")",                                                                                   \
        what)

// struct EvalResult
EvalResult::EvalResult() : values(), do_break(false), do_return(false), source_change() {}
EvalResult::EvalResult(const CallResult& call_result)
    : values(call_result.values()), do_break(false), do_return(false),
      source_change(call_result.source_change()) {}

static auto combine_source_changes(
    const std::optional<SourceChangeTree>& lhs, const std::optional<SourceChangeTree>& rhs)
    -> std::optional<SourceChangeTree> {
    if (lhs.has_value() && rhs.has_value()) {
        return SourceChangeCombination({*lhs, *rhs});
    } else if (lhs.has_value()) {
        return lhs;
    } else {
        return rhs;
    }
}

void EvalResult::combine(const EvalResult& other) {
    this->values = other.values;
    this->do_break = other.do_break;
    this->do_return = other.do_return;
    this->source_change = combine_source_changes(this->source_change, other.source_change);
}

EvalResult::operator minilua::EvalResult() const {
    minilua::EvalResult result;
    result.value = this->values.get(0);
    result.source_change = this->source_change;
    return result;
}

auto operator<<(std::ostream& o, const EvalResult& self) -> std::ostream& {
    o << "EvalResult{ "
      << ".value = " << self.values << ", .do_break = " << self.do_break
      << ", .do_return = " << self.do_return << ", .source_change = ";

    if (self.source_change.has_value()) {
        o << *self.source_change;
    } else {
        o << "nullopt";
    }

    return o << "}";
}

// class Interpreter
Interpreter::Interpreter(const InterpreterConfig& config) : config(config) {}

auto Interpreter::run(const ts::Tree& tree, Env& env) -> EvalResult {
    try {
        return this->visit_root(ast::Program(tree.root_node()), env);
    } catch (const InterpreterException&) {
        throw;
    } catch (const std::exception& e) {
        throw InterpreterException("unknown error: "s + e.what());
    }
}

auto Interpreter::tracer() const -> std::ostream& { return *this->config.target; }
void Interpreter::trace_enter_node(ts::Node node, std::optional<std::string> method_name) const {
    if (this->config.trace_nodes) {
        this->tracer() << "Enter node: " << ts::debug_print_node(node);
        if (method_name) {
            this->tracer() << " (method: " << method_name.value() << ")";
        }
        this->tracer() << "\n";
    }
}
void Interpreter::trace_exit_node(
    ts::Node node, std::optional<std::string> method_name,
    std::optional<std::string> reason) const {
    if (this->config.trace_nodes) {
        this->tracer() << "Exit node: " << ts::debug_print_node(node);
        if (method_name) {
            this->tracer() << " (method: " << method_name.value() << ")";
        }
        if (reason) {
            this->tracer() << " reason: " << reason.value();
        }
        this->tracer() << "\n";
    }
}
void Interpreter::trace_function_call(
    ast::Prefix prefix, const std::vector<Value>& arguments) const {
    auto function_name = std::string(prefix.raw().value().text());
    if (this->config.trace_calls) {
        this->tracer() << "Calling function: " << function_name << " with arguments (";
        for (const auto& arg : arguments) {
            this->tracer() << arg << ", ";
        }
        this->tracer() << ")\n";
    }
}
void Interpreter::trace_function_call_result(ast::Prefix prefix, const CallResult& result) const {
    if (this->config.trace_calls) {
        auto function_name = std::string(prefix.raw().value().text());
        this->tracer() << "Function call to: " << function_name << " resulted in "
                       << result.values();
        if (result.source_change().has_value()) {
            this->tracer() << " with source changes " << result.source_change().value();
        }
        this->tracer() << "\n";
    }
}
void Interpreter::trace_exprlists(
    std::vector<ast::Expression>& exprlist, const Vallist& result) const {
    if (this->config.trace_exprlists) {
        this->tracer() << "Exprlist: (";
        const auto* sep = "";
        for (auto& expr : exprlist) {
            this->tracer() << sep << expr.raw().value().text();
            sep = ", ";
        }
        this->tracer() << ") resulted in (";

        sep = "";
        for (const auto& value : result) {
            this->tracer() << sep << value;
            sep = ", ";
        }
        this->tracer() << ")\n";
    }
}

auto Interpreter::enter_block(Env& env) -> Env {
    if (this->config.trace_enter_block) {
        this->tracer() << "Enter block: " << env << "\n";
    }
    return Env(env);
}

// helper functions
static auto convert_range(ts::Range range) -> Range {
    return Range{
        .start = {range.start.point.row, range.start.point.column, range.start.byte},
        .end = {range.end.point.row, range.end.point.column, range.end.byte},
    };
}

// interpreter implementation
auto Interpreter::visit_root(ast::Program program, Env& env) -> EvalResult {
    this->trace_enter_node(program.raw());

    EvalResult result;

    auto body = program.body();

    for (auto child : body.statements()) {
        EvalResult sub_result = this->visit_statement(child, env);
        result.combine(sub_result);
    }

    if (body.return_statement()) {
        // result.combine(this->visit_return_statement(body.ret().value(), env));
        // TODO properly return the vallist
    }

    this->trace_exit_node(program.raw());
    return result;
}

auto Interpreter::visit_statement(ast::Statement statement, Env& env) -> EvalResult {
    this->trace_enter_node(statement.raw().value());

    auto result = std::visit(
        overloaded{
            [this, &env](ast::VariableDeclaration node) {
                return this->visit_variable_declaration(node, env);
            },
            [this, &env](ast::DoStatement node) { return this->visit_do_statement(node, env); },
            [this, &env](ast::IfStatement node) { return this->visit_if_statement(node, env); },
            [this, &env](ast::WhileStatement node) {
                return this->visit_while_statement(node, env);
            },
            [this, &env](ast::RepeatStatement node) {
                return this->visit_repeat_until_statement(node, env);
            },
            [this, &env](ast::ForStatement node) -> EvalResult {
                // TODO desugar this
                throw UNIMPLEMENTED("for");
            },
            [this, &env](ast::ForInStatement node) -> EvalResult {
                // TODO desugar this
                throw UNIMPLEMENTED("for in");
            },
            [this, &env](ast::GoTo node) -> EvalResult { throw UNIMPLEMENTED("goto"); },
            [this, &env](ast::Break node) { return this->visit_break_statement(env); },
            [this, &env](ast::Label node) -> EvalResult { throw UNIMPLEMENTED("label"); },
            [this, &env](ast::FunctionStatement node) {
                // TODO desugar this to variable and assignment
                return this->visit_function_statement(node, env);
            },
            [this, &env](ast::FunctionCall node) -> EvalResult {
                return this->visit_function_call(node, env);
            },
            [this, &env](ast::Expression node) { return this->visit_expression(node, env); },
        },
        statement.options());

    this->trace_exit_node(statement.raw().value());

    if (!result.do_return) {
        result.values = Vallist();
    }

    return result;
}

auto Interpreter::visit_do_statement(ast::DoStatement do_stmt, Env& env) -> EvalResult {
    this->trace_enter_node(do_stmt.raw().value());

    auto result = this->visit_block(do_stmt.body(), env);

    this->trace_exit_node(do_stmt.raw().value());
    return result;
}

auto Interpreter::visit_block(ast::Body block, Env& env) -> EvalResult {
    Env block_env = this->enter_block(env);
    return this->visit_block_with_local_env(std::move(block), block_env);
}
auto Interpreter::visit_block_with_local_env(ast::Body block, Env& block_env) -> EvalResult {
    EvalResult result;

    for (auto stmt : block.statements()) {
        auto sub_result = this->visit_statement(stmt, block_env);
        result.combine(sub_result);

        if (result.do_break) {
            this->trace_exit_node(stmt.raw().value(), std::nullopt, "break");
            return result;
        }
        if (result.do_return) {
            this->trace_exit_node(stmt.raw().value(), std::nullopt, "return");
            return result;
        }
    }

    auto return_stmt = block.return_statement();
    if (return_stmt) {
        auto sub_result = this->visit_return_statement(return_stmt.value(), block_env);
        result.combine(sub_result);
    }

    return result;
}

auto Interpreter::visit_if_statement(ast::IfStatement if_stmt, Env& env) -> EvalResult {
    this->trace_enter_node(if_stmt.raw().value());

    EvalResult result;

    {
        // if condition condition
        auto condition = if_stmt.condition();
        auto condition_result = this->visit_expression(condition, env);
        result.combine(condition_result);

        // "then" block
        if (condition_result.values.get(0)) {
            auto body_result = this->visit_block(if_stmt.body(), env);
            result.combine(body_result);

            this->trace_exit_node(if_stmt.raw().value());
            return result;
        }
    }

    // "else if" blocks
    for (auto elseif_stmt : if_stmt.elseifs()) {
        // else if condition
        auto condition = elseif_stmt.condition();
        auto condition_result = this->visit_expression(condition, env);
        result.combine(condition_result);

        if (condition_result.values.get(0)) {
            auto body_result = this->visit_block(elseif_stmt.body(), env);
            result.combine(body_result);

            this->trace_exit_node(if_stmt.raw().value());
            return result;
        }
    }

    // "else" block
    auto else_stmt = if_stmt.else_statement();
    if (else_stmt) {
        auto body_result = this->visit_block(else_stmt->body(), env);
        result.combine(body_result);
    }

    this->trace_exit_node(if_stmt.raw().value());
    return result;
}

auto Interpreter::visit_while_statement(ast::WhileStatement while_stmt, Env& env) -> EvalResult {
    this->trace_enter_node(while_stmt.raw().value());

    EvalResult result;

    auto condition = while_stmt.repeat_conditon();

    while (true) {
        auto condition_result = this->visit_expression(condition, env);
        result.combine(condition_result);

        if (!condition_result.values.get(0)) {
            this->trace_exit_node(while_stmt.raw().value());
            return result;
        }

        auto block_result = this->visit_block(while_stmt.body(), env);
        result.combine(block_result);

        if (result.do_break) {
            this->trace_exit_node(while_stmt.raw().value(), std::nullopt, "break");
            result.do_break = false;
            return result;
        }
        if (result.do_return) {
            this->trace_exit_node(while_stmt.raw().value(), std::nullopt, "return");
            return result;
        }
    }

    this->trace_exit_node(while_stmt.raw().value());
    return result;
}

auto Interpreter::visit_repeat_until_statement(ast::RepeatStatement repeat_stmt, Env& env)
    -> EvalResult {
    this->trace_enter_node(repeat_stmt.raw());

    EvalResult result;

    auto body = repeat_stmt.body();
    auto condition = repeat_stmt.repeat_condition();

    while (true) {
        Env block_env = this->enter_block(env);

        auto block_result = this->visit_block_with_local_env(body, block_env);
        result.combine(block_result);

        if (result.do_break) {
            this->trace_exit_node(repeat_stmt.raw(), std::nullopt, "break");
            result.do_break = false;
            return result;
        }
        if (result.do_return) {
            this->trace_exit_node(repeat_stmt.raw(), std::nullopt, "return");
            return result;
        }

        // the condition is part of the same block and can access local variables
        // declared in the repeat block
        auto condition_result = this->visit_expression(condition, block_env);
        result.combine(condition_result);

        if (condition_result.values.get(0)) {
            return result;
        }
    }

    this->trace_exit_node(repeat_stmt.raw());
    return result;
}

auto Interpreter::visit_break_statement(Env& env) -> EvalResult {
    EvalResult result;
    result.do_break = true;
    return result;
}

auto Interpreter::visit_expression_list(std::vector<ast::Expression> expressions, Env& env)
    -> EvalResult {
    EvalResult result;
    std::vector<Value> return_values;

    if (!expressions.empty()) {
        for (int i = 0; i < expressions.size() - 1; ++i) {
            const auto expr = expressions[i];
            const auto sub_result = this->visit_expression(expr, env);
            result.combine(sub_result);
            return_values.push_back(sub_result.values.get(0));
        }

        // if the last element has a vallist (i.e. because it was a function call) the vallist is
        // appended
        auto expr = expressions[expressions.size() - 1];
        const auto sub_result = this->visit_expression(expr, env);
        result.combine(sub_result);
        return_values.insert(
            return_values.end(), sub_result.values.begin(), sub_result.values.end());
    }

    result.values = return_values;

    this->trace_exprlists(expressions, result.values);

    return result;
}

auto Interpreter::visit_return_statement(ast::Return return_stmt, Env& env) -> EvalResult {
    this->trace_enter_node(return_stmt.raw());

    auto result = this->visit_expression_list(return_stmt.exp_list(), env);
    result.do_return = true;

    this->trace_exit_node(return_stmt.raw());
    return result;
}

auto Interpreter::visit_variable_declaration(ast::VariableDeclaration decl, Env& env)
    -> EvalResult {
    this->trace_enter_node(decl.raw().value());

    EvalResult result = this->visit_expression_list(decl.declarations(), env);
    const auto vallist = result.values;

    auto targets = decl.declarators();

    for (int i = 0; i < decl.declarators().size(); ++i) {
        auto target = targets[i].options();
        const auto& value = vallist.get(i);

        if (decl.local()) {
            std::visit(
                overloaded{
                    [this, &env, &value](ast::Identifier ident) {
                        env.set_local(this->visit_identifier(ident, env), value);
                    },
                    [](ast::FieldExpression /*node*/) {
                        throw InterpreterException(
                            "Field expression not allowed as target of local declaration");
                    },
                    [](ast::TableIndex /*node*/) {
                        throw InterpreterException(
                            "Table access not allowed as target of local declaration");
                    },
                },
                target);
        } else {
            std::visit(
                overloaded{
                    [this, &env, &value](ast::Identifier ident) {
                        env.set_var(this->visit_identifier(ident, env), value);
                    },
                    [](auto node) { throw UNIMPLEMENTED(node.raw().type()); }},
                target);
        }
    }

    this->trace_exit_node(decl.raw().value());
    return result;
}

auto Interpreter::visit_identifier(ast::Identifier ident, Env& env) -> std::string {
    this->trace_enter_node(ident.raw().value());
    this->trace_exit_node(ident.raw().value());
    return ident.string();
}

auto Interpreter::visit_expression(ast::Expression expr, Env& env) -> EvalResult {
    this->trace_enter_node(expr.raw().value());

    auto node = expr.raw();

    EvalResult result = std::visit(
        overloaded{
            [this, &env](ast::Spread) -> EvalResult { return this->visit_vararg_expression(env); },
            [this, &env](ast::Prefix prefix) { return this->visit_prefix(prefix, env); },
            [this, &env](ast::FunctionDefinition function_definition) {
                return this->visit_function_expression(function_definition, env);
            },
            [this, &env](ast::Table table) { return this->visit_table_constructor(table, env); },
            [this, &env](ast::BinaryOperation binary_op) {
                return this->visit_binary_operation(binary_op, env);
            },
            [this, &env](ast::UnaryOperation unary_op) {
                return this->visit_unary_operation(unary_op, env);
            },
            [&node](ast::Literal literal) {
                EvalResult result;
                Value value;
                switch (literal.type()) {
                case ast::LiteralType::TRUE:
                    value = Value(Bool(true));
                    break;
                case ast::LiteralType::FALSE:
                    value = Value(Bool(false));
                    break;
                case ast::LiteralType::NIL:
                    value = Value(Nil());
                    break;
                case ast::LiteralType::NUMBER:
                    value = parse_number_literal(literal.content());
                    break;
                case ast::LiteralType::STRING:
                    value = parse_string_literal(literal.content());
                    break;
                }
                auto origin = LiteralOrigin{.location = literal.range()};
                result.values = Vallist(value.with_origin(origin));
                return result;
            },
            [this, &env](ast::Identifier ident) {
                auto variable_name = this->visit_identifier(ident, env);
                EvalResult result;
                result.values = Vallist(env.get_var(variable_name));
                return result;
            },
        },
        expr.options());

    this->trace_exit_node(expr.raw().value());
    return result;
}

auto Interpreter::visit_vararg_expression(Env& env) -> EvalResult {
    EvalResult result;

    auto varargs = env.get_varargs();
    if (!varargs.has_value()) {
        throw InterpreterException("cannot use '...' outside a vararg function");
    }

    result.values = varargs.value();

    return result;
}

auto Interpreter::visit_function_expression(ast::FunctionDefinition function_definition, Env& env)
    -> EvalResult {
    this->trace_enter_node(function_definition.raw());

    EvalResult result;

    auto parameters = function_definition.parameters();

    if (parameters.leading_self()) {
        throw UNIMPLEMENTED("self as function parameter");
    }

    std::vector<std::string> actual_parameters;
    {
        auto raw_params = parameters.params();
        actual_parameters.reserve(raw_params.size());
        std::transform(
            raw_params.begin(), raw_params.end(), std::back_inserter(actual_parameters),
            [this, &env](auto ident) { return this->visit_identifier(ident, env); });
    }

    bool vararg = parameters.spread();

    auto body = function_definition.body();

    Value func = Function(
        [body = std::move(body), parameters = std::move(actual_parameters), vararg,
         this](const CallContext& ctx) -> CallResult {
            // setup parameters as local variables
            Env env = Env(ctx.environment().get_raw_impl().inner);
            for (int i = 0; i < parameters.size(); ++i) {
                env.set_local(parameters[i], ctx.arguments().get(i));
            }

            if (vararg && parameters.size() < ctx.arguments().size()) {
                std::vector<Value> varargs;
                varargs.reserve(ctx.arguments().size() - parameters.size());
                std::copy(
                    ctx.arguments().begin() + parameters.size(), ctx.arguments().end(),
                    std::back_inserter(varargs));
                env.set_varargs(varargs);
            } else {
                env.set_varargs(std::nullopt);
            }

            if (env.get_varargs()) {
                std::cerr << "varargs: " << *env.get_varargs() << "\n";
            }

            auto result = this->visit_block_with_local_env(body, env);

            auto return_value = Vallist();
            if (result.do_return) {
                return_value = result.values;
            }
            return CallResult(return_value, result.source_change);
        });

    result.values = Vallist(func);

    this->trace_exit_node(function_definition.raw());
    return result;
}

// TODO remove once we can desugar function statements
auto Interpreter::visit_function_statement(ast::FunctionStatement function_statement, Env& env)
    -> EvalResult {
    this->trace_enter_node(function_statement.raw());

    EvalResult result;

    auto parameters = function_statement.parameters();

    if (parameters.leading_self()) {
        throw UNIMPLEMENTED("self as function parameter");
    }

    std::vector<std::string> actual_parameters;
    {
        auto raw_params = parameters.params();
        actual_parameters.reserve(raw_params.size());
        std::transform(
            raw_params.begin(), raw_params.end(), std::back_inserter(actual_parameters),
            [this, &env](auto ident) { return this->visit_identifier(ident, env); });
    }

    bool vararg = parameters.spread();

    auto body = function_statement.body();

    Value func = Function(
        [body = std::move(body), parameters = std::move(actual_parameters), vararg,
         this](const CallContext& ctx) -> CallResult {
            // setup parameters as local variables
            Env env = Env(ctx.environment().get_raw_impl().inner);
            for (int i = 0; i < parameters.size(); ++i) {
                env.set_local(parameters[i], ctx.arguments().get(i));
            }

            if (vararg && parameters.size() < ctx.arguments().size()) {
                std::vector<Value> varargs;
                varargs.reserve(ctx.arguments().size() - parameters.size());
                std::copy(
                    ctx.arguments().begin() + parameters.size(), ctx.arguments().end(),
                    std::back_inserter(varargs));
                env.set_varargs(varargs);
            } else {
                env.set_varargs(std::nullopt);
            }

            if (env.get_varargs()) {
                std::cerr << "varargs: " << *env.get_varargs() << "\n";
            }

            auto result = this->visit_block_with_local_env(body, env);

            auto return_value = Vallist();
            if (result.do_return) {
                return_value = result.values;
            }
            return CallResult(return_value, result.source_change);
        });

    auto function_name = function_statement.name();
    auto identifiers = function_name.identifier();

    if (identifiers.size() != 1 || function_name.method()) {
        throw UNIMPLEMENTED("function complicated name");
    }

    auto ident = this->visit_identifier(identifiers[0], env);
    env.set_global(ident, func);

    this->trace_exit_node(function_statement.raw());
    return result;
}

auto Interpreter::visit_table_index(ast::TableIndex table_index, Env& env) -> EvalResult {
    this->trace_enter_node(table_index.raw());

    EvalResult result;

    auto prefix_result = this->visit_prefix(table_index.table(), env);
    result.combine(prefix_result);

    auto table = prefix_result.values.get(0);

    auto index_result = this->visit_expression(table_index.index(), env);
    result.combine(index_result);

    auto index = index_result.values.get(0);

    result.values = Vallist(table[index]);

    this->trace_exit_node(table_index.raw());
    return result;
}

auto Interpreter::visit_field_expression(ast::FieldExpression field_expression, Env& env)
    -> EvalResult {
    this->trace_enter_node(field_expression.raw());

    EvalResult result;

    auto table_result = this->visit_prefix(field_expression.table_id(), env);
    result.combine(table_result);

    std::string key = this->visit_identifier(field_expression.property_id(), env);

    result.values = Vallist(table_result.values.get(0)[key]);

    this->trace_exit_node(field_expression.raw());
    return result;
}

auto Interpreter::visit_table_constructor(ast::Table table_constructor, Env& env) -> EvalResult {
    this->trace_enter_node(table_constructor.raw());

    EvalResult result;

    Table table;

    const auto fields = table_constructor.fields();
    if (!fields.empty()) {
        // TODO move the consecutive_key logic to table because it is not completely correct
        int consecutive_key = 1;

        for (int i = 0; i < fields.size() - 1; ++i) {
            auto field = fields[i];
            auto [key, value] = std::visit(
                overloaded{
                    [this, &env, &result](std::pair<ast::Expression, ast::Expression> field)
                        -> std::pair<Value, Value> {
                        auto key_result = this->visit_expression(field.first, env);
                        result.combine(key_result);

                        auto value_result = this->visit_expression(field.second, env);
                        result.combine(value_result);

                        return std::make_pair(key_result.values.get(0), value_result.values.get(0));
                    },
                    [this, &env, &result](std::pair<ast::Identifier, ast::Expression> field)
                        -> std::pair<Value, Value> {
                        auto key = this->visit_identifier(field.first, env);

                        auto value_result = this->visit_expression(field.second, env);
                        result.combine(value_result);

                        return std::make_pair(key, value_result.values.get(0));
                    },
                    [this, &env, &result,
                     &consecutive_key](ast::Expression item) -> std::pair<Value, Value> {
                        auto item_result = this->visit_expression(item, env);
                        result.combine(item_result);
                        auto key = consecutive_key;
                        consecutive_key++;
                        return std::make_pair(key, item_result.values.get(0));
                    },
                },
                field.content());

            table.set(key, value);
        }

        // if last entry is an expression and returns a vallist the vallist is appended
        auto field = fields[fields.size() - 1];
        std::visit(
            overloaded{
                [this, &env, &result, &table](std::pair<ast::Expression, ast::Expression> field) {
                    auto key_result = this->visit_expression(field.first, env);
                    result.combine(key_result);

                    auto value_result = this->visit_expression(field.second, env);
                    result.combine(value_result);

                    table.set(key_result.values.get(0), value_result.values.get(0));
                },
                [this, &env, &result, &table](std::pair<ast::Identifier, ast::Expression> field) {
                    auto key = this->visit_identifier(field.first, env);

                    auto value_result = this->visit_expression(field.second, env);
                    result.combine(value_result);

                    table.set(key, value_result.values.get(0));
                },
                [this, &env, &result, &consecutive_key, &table](ast::Expression item) {
                    auto item_result = this->visit_expression(item, env);
                    result.combine(item_result);

                    for (auto value : item_result.values) {
                        auto key = consecutive_key;
                        consecutive_key++;
                        table.set(key, value);
                    }
                },
            },
            field.content());
    }

    result.values = Vallist(table);

    this->trace_exit_node(table_constructor.raw());
    return result;
}

auto Interpreter::visit_binary_operation(ast::BinaryOperation bin_op, Env& env) -> EvalResult {
    this->trace_enter_node(bin_op.raw().value());

    EvalResult result;

    auto origin = convert_range(bin_op.raw().value().range());

    auto lhs_result = this->visit_expression(bin_op.left(), env);
    auto rhs_result = this->visit_expression(bin_op.right(), env);

    auto impl_operator = [&result, &lhs_result, &rhs_result, &origin](auto f) {
        Value value = std::invoke(f, lhs_result.values.get(0), rhs_result.values.get(0), origin);
        result.combine(rhs_result);
        result.values = Vallist(value);
    };

    switch (bin_op.binary_operator()) {
    case ast::BinOpEnum::ADD:
        impl_operator(&Value::add);
        break;
    case ast::BinOpEnum::SUB:
        impl_operator(&Value::sub);
        break;
    case ast::BinOpEnum::MUL:
        impl_operator(&Value::mul);
        break;
    case ast::BinOpEnum::DIV:
        impl_operator(&Value::div);
        break;
    case ast::BinOpEnum::MOD:
        impl_operator(&Value::mod);
        break;
    case ast::BinOpEnum::POW:
        impl_operator(&Value::pow);
        break;
    case ast::BinOpEnum::LT:
        impl_operator(&Value::less_than);
        break;
    case ast::BinOpEnum::GT:
        impl_operator(&Value::greater_than);
        break;
    case ast::BinOpEnum::LEQ:
        impl_operator(&Value::less_than_or_equal);
        break;
    case ast::BinOpEnum::GEQ:
        impl_operator(&Value::greater_than_or_equal);
        break;
    case ast::BinOpEnum::EQ:
        impl_operator(&Value::equals);
        break;
    case ast::BinOpEnum::NEQ:
        impl_operator(&Value::unequals);
        break;
    case ast::BinOpEnum::CONCAT:
        impl_operator(&Value::concat);
        break;
    case ast::BinOpEnum::AND:
        impl_operator(&Value::logic_and);
        break;
    case ast::BinOpEnum::OR:
        impl_operator(&Value::logic_or);
        break;
    case ast::BinOpEnum::SHIFT_LEFT:
        throw UNIMPLEMENTED("shift left");
        break;
    case ast::BinOpEnum::SHIFT_RIGHT:
        throw UNIMPLEMENTED("shift right");
        break;
    case ast::BinOpEnum::BIT_XOR:
        throw UNIMPLEMENTED("bitwise xor");
        break;
    case ast::BinOpEnum::BIT_OR:
        impl_operator(&Value::bit_or);
        break;
    case ast::BinOpEnum::BIT_AND:
        impl_operator(&Value::bit_and);
        break;
    case ast::BinOpEnum::INT_DIV:
        throw UNIMPLEMENTED("intdiv");
        break;
    }

    this->trace_exit_node(bin_op.raw().value());
    return result;
}

auto Interpreter::visit_unary_operation(ast::UnaryOperation unary_op, Env& env) -> EvalResult {
    this->trace_enter_node(unary_op.raw().value());

    EvalResult result = this->visit_expression(unary_op.expression(), env);

    auto range = convert_range(unary_op.raw().value().range());

    switch (unary_op.unary_operator()) {
    case ast::UnOpEnum::NOT:
        result.values = Vallist(result.values.get(0).invert(range));
        break;
    case ast::UnOpEnum::NEG:
        result.values = Vallist(result.values.get(0).negate(range));
        break;
    case ast::UnOpEnum::LEN:
        result.values = Vallist(result.values.get(0).len(range));
        break;
    case ast::UnOpEnum::BWNOT:
        throw UNIMPLEMENTED("bitwise not");
        break;
    }

    this->trace_exit_node(unary_op.raw().value());
    return result;
}

auto Interpreter::visit_prefix(ast::Prefix prefix, Env& env) -> EvalResult {
    this->trace_enter_node(prefix.raw().value());

    EvalResult result = std::visit(
        overloaded{
            [](ast::Self) -> EvalResult { throw UNIMPLEMENTED("self"); },
            [this, &env](ast::VariableDeclarator variable_decl) {
                return std::visit(
                    overloaded{
                        [this, &env](ast::Identifier ident) {
                            EvalResult result;
                            result.values =
                                Vallist(env.get_var(this->visit_identifier(ident, env)));
                            return result;
                        },
                        [this, &env](ast::FieldExpression field) {
                            // TODO desugar to table index
                            return this->visit_field_expression(field, env);
                        },
                        [this, &env](ast::TableIndex table_index) -> EvalResult {
                            return this->visit_table_index(table_index, env);
                        }},
                    variable_decl.options());
            },
            [this, &env](ast::FunctionCall call) { return this->visit_function_call(call, env); },
            [this, &env](ast::Expression expr) { return this->visit_expression(expr, env); }},
        prefix.options());

    this->trace_exit_node(prefix.raw().value());
    return result;
}

auto Interpreter::visit_function_call(ast::FunctionCall call, Env& env) -> EvalResult {
    this->trace_enter_node(call.raw().value());

    EvalResult result;

    auto function_obj_result = this->visit_prefix(call.id(), env);
    result.combine(function_obj_result);

    if (call.method()) {
        throw UNIMPLEMENTED("method calls");
    }

    EvalResult exprlist_result = this->visit_expression_list(call.args(), env);
    auto arguments = exprlist_result.values;

    this->trace_function_call(call.id(), std::vector<Value>(arguments.begin(), arguments.end()));

    // call function
    // this will produce an error if the obj is not callable
    auto obj = function_obj_result.values.get(0);
    auto environment = Environment(env);
    auto ctx = CallContext(&environment).make_new(arguments);

    try {
        CallResult call_result = obj.call(ctx);
        result.combine(EvalResult(call_result));

        this->trace_function_call_result(call.id(), call_result);
    } catch (const std::runtime_error& e) {
        std::string pos = call.raw().value().range().start.point.pretty(true);
        throw InterpreterException("failed to call function  ("s + pos + ") : " + e.what());
    }

    this->trace_exit_node(call.raw().value());
    return result;
}

} // namespace minilua::details
