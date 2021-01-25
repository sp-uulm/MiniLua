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
EvalResult::EvalResult() : value(), do_break(false), do_return(std::nullopt), source_change() {}
EvalResult::EvalResult(const CallResult& call_result)
    : value(std::get<0>(call_result.values().tuple<1>())), do_break(false), do_return(std::nullopt),
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
    this->value = other.value;
    this->do_break = other.do_break;
    this->do_return = other.do_return;
    this->source_change = combine_source_changes(this->source_change, other.source_change);
}

EvalResult::operator minilua::EvalResult() const {
    minilua::EvalResult result;
    result.value = this->value;
    result.source_change = this->source_change;
    return result;
}

auto operator<<(std::ostream& o, const EvalResult& self) -> std::ostream& {
    o << "EvalResult{ "
      << ".value = " << self.value << ", .do_break = " << self.do_break << ", .do_return = ";
    if (self.do_return) {
        o << *self.do_return;
    } else {
        o << "nullopt";
    }

    o << ", .source_change = ";
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
    const std::string& function_name, const std::vector<Value>& arguments) const {
    if (this->config.trace_calls) {
        this->tracer() << "Calling function: " << function_name << " with arguments (";
        for (const auto& arg : arguments) {
            this->tracer() << arg << ", ";
        }
        this->tracer() << ")\n";
    }
}
void Interpreter::trace_function_call_result(
    const std::string& function_name, const CallResult& result) const {
    if (this->config.trace_calls) {
        this->tracer() << "Function call to: " << function_name << " resulted in "
                       << result.values();
        if (result.source_change().has_value()) {
            this->tracer() << " with source changes " << result.source_change().value();
        }
        this->tracer() << "\n";
    }
}

auto Interpreter::enter_block(Env& env) -> Env {
    if (this->config.trace_enter_block) {
        this->tracer() << "Enter block: " << env << "\n";
    }
    return Env(env);
}

// helper functions
static const std::set<std::string> IGNORE_NODES{";", "comment"};

static auto should_ignore_node(ts::Node node) -> bool {
    return IGNORE_NODES.find(node.type()) != IGNORE_NODES.end();
}

static auto convert_range(ts::Range range) -> Range {
    return Range{
        .start = {range.start.point.row, range.start.point.column, range.start.byte},
        .end = {range.end.point.row, range.end.point.column, range.end.byte},
    };
}

static auto make_environment(Env& env) -> Environment {
    return Environment(Environment::Impl{env});
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

    if (body.ret()) {
        // result.combine(this->visit_return_statement(body.ret().value(), env));
        // TODO properly return the vallist
    }

    this->trace_exit_node(program.raw());
    return result;
}

auto Interpreter::visit_statement(ast::Statement statement, Env& env) -> EvalResult {
    this->trace_enter_node(statement.raw());

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
            [this, &env](ast::ForStatement node) -> EvalResult { throw UNIMPLEMENTED("for"); },
            [this, &env](ast::ForInStatement node) -> EvalResult { throw UNIMPLEMENTED("for in"); },
            [this, &env](ast::GoTo node) -> EvalResult { throw UNIMPLEMENTED("goto"); },
            [this, &env](ast::Break node) { return this->visit_break_statement(env); },
            [this, &env](ast::Label node) -> EvalResult { throw UNIMPLEMENTED("goto"); },
            [this, &env](ast::FunctionStatement node) {
                // TODO desugar this to variable and assignment
                return this->visit_function_expression(node.raw(), env);
            },
            [this, &env](ast::LocalFunctionStatement node) {
                // TODO desugar this to variable and assignment
                return this->visit_function_expression(node.raw(), env);
            },
            [this, &env](ast::FunctionCall node) -> EvalResult {
                return this->visit_function_call(node, env);
            },
            [this, &env](ast::Expression node) { return this->visit_expression(node.raw(), env); },
        },
        statement.options());

    this->trace_exit_node(statement.raw());

    if (!result.do_return) {
        result.value = Nil();
    }

    return result;
}

auto Interpreter::visit_do_statement(ast::DoStatement do_stmt, Env& env) -> EvalResult {
    this->trace_enter_node(do_stmt.raw());

    auto result = this->visit_block(do_stmt.body(), env);

    this->trace_exit_node(do_stmt.raw());
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
            this->trace_exit_node(stmt.raw(), std::nullopt, "break");
            return result;
        }
        if (result.do_return) {
            this->trace_exit_node(stmt.raw(), std::nullopt, "return");
            return result;
        }
    }

    auto return_stmt = block.ret();
    if (return_stmt) {
        auto sub_result = this->visit_return_statement(return_stmt->raw(), block_env);
        result.combine(sub_result);
    }

    return result;
}

auto Interpreter::visit_if_statement(ast::IfStatement if_stmt, Env& env) -> EvalResult {
    this->trace_enter_node(if_stmt.raw());

    EvalResult result;

    {
        // if condition condition
        auto condition = if_stmt.cond();
        auto condition_result = this->visit_expression(condition.raw(), env);
        result.combine(condition_result);

        // "then" block
        if (condition_result.value) {
            auto body_result = this->visit_block(if_stmt.body(), env);
            result.combine(body_result);

            this->trace_exit_node(if_stmt.raw());
            return result;
        }
    }

    // "else if" blocks
    for (auto elseif_stmt : if_stmt.elseifs()) {
        // else if condition
        auto condition = elseif_stmt.cond();
        auto condition_result = this->visit_expression(condition.raw(), env);
        result.combine(condition_result);

        if (condition_result.value) {
            auto body_result = this->visit_block(elseif_stmt.body(), env);
            result.combine(body_result);

            this->trace_exit_node(if_stmt.raw());
            return result;
        }
    }

    // "else" block
    auto else_stmt = if_stmt.else_();
    if (else_stmt) {
        auto body_result = this->visit_block(else_stmt->body(), env);
        result.combine(body_result);
    }

    this->trace_exit_node(if_stmt.raw());
    return result;
}

auto Interpreter::visit_while_statement(ast::WhileStatement while_stmt, Env& env) -> EvalResult {
    this->trace_enter_node(while_stmt.raw());

    EvalResult result;

    auto condition = while_stmt.exit_cond();

    while (true) {
        auto condition_result = this->visit_expression(condition.raw(), env);
        result.combine(condition_result);

        if (!condition_result.value) {
            this->trace_exit_node(while_stmt.raw());
            return result;
        }

        auto block_result = this->visit_block(while_stmt.body(), env);
        result.combine(block_result);

        if (result.do_break) {
            this->trace_exit_node(while_stmt.raw(), std::nullopt, "break");
            result.do_break = false;
            return result;
        }
        if (result.do_return) {
            this->trace_exit_node(while_stmt.raw(), std::nullopt, "return");
            return result;
        }
    }

    this->trace_exit_node(while_stmt.raw());
    return result;
}

auto Interpreter::visit_repeat_until_statement(ast::RepeatStatement repeat_stmt, Env& env)
    -> EvalResult {
    this->trace_enter_node(repeat_stmt.raw());

    EvalResult result;

    auto body = repeat_stmt.body();
    auto condition = repeat_stmt.until_cond();

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
        auto condition_result = this->visit_expression(condition.raw(), block_env);
        result.combine(condition_result);

        if (condition_result.value) {
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
auto Interpreter::visit_return_statement(ast::Return return_stmt, Env& env) -> EvalResult {
    this->trace_enter_node(return_stmt.raw());

    EvalResult result;
    std::vector<Value> return_values;

    for (auto expr : return_stmt.explist()) {
        auto sub_result = this->visit_expression(expr.raw(), env);
        result.combine(sub_result);
        return_values.push_back(sub_result.value);
    }

    result.do_return = Vallist(return_values);

    this->trace_exit_node(return_stmt.raw());
    return result;
}

auto Interpreter::visit_variable_declaration(ast::VariableDeclaration decl, Env& env)
    -> EvalResult {
    this->trace_enter_node(decl.raw());

    EvalResult result;

    auto exprs = decl.declarations();

    std::vector<Value> expr_results;
    expr_results.reserve(exprs.size());
    std::transform(
        exprs.begin(), exprs.end(), std::back_inserter(expr_results),
        [this, &result, &env](ast::Expression expr) {
            auto sub_result = this->visit_expression(expr.raw(), env);
            result.combine(sub_result);
            return sub_result.value;
        });

    const auto vallist = Vallist(expr_results);
    auto targets = decl.declarators();

    for (int i = 0; i < decl.declarators().size(); ++i) {
        auto target = targets[i].var();
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

    this->trace_exit_node(decl.raw());
    return result;
}

auto Interpreter::visit_identifier(ast::Identifier ident, Env& env) -> std::string {
    this->trace_enter_node(ident.raw());
    this->trace_exit_node(ident.raw());
    return ident.str();
}

auto Interpreter::visit_expression(ts::Node node, Env& env) -> EvalResult {
    this->trace_enter_node(node);

    EvalResult result;

    if (node.type() == "number"s) {
        auto value = parse_number_literal(node.text());
        Origin origin = LiteralOrigin{.location = convert_range(node.range())};
        result.value = value.with_origin(origin);
    } else if (node.type() == "true"s) {
        Origin origin = LiteralOrigin{.location = convert_range(node.range())};
        result.value = Value(true).with_origin(origin);
    } else if (node.type() == "false"s) {
        Origin origin = LiteralOrigin{.location = convert_range(node.range())};
        result.value = Value(false).with_origin(origin);
    } else if (node.type() == "nil"s) {
        Origin origin = LiteralOrigin{.location = convert_range(node.range())};
        result.value = Value(Nil()).with_origin(origin);
    } else if (node.type() == "string"s) {
        try {
            auto value = parse_string_literal(node.text());
            Origin origin = LiteralOrigin{.location = convert_range(node.range())};
            result.value = value.with_origin(origin);
        } catch (const std::runtime_error& e) {
            throw InterpreterException(e.what());
        }
    } else if (node.type() == "table"s) {
        result = this->visit_table_constructor(node, env);
    } else if (node.type() == "identifier"s) {
        auto variable_name = this->visit_identifier(node, env);
        result.value = env.get_var(variable_name);
    } else if (node.type() == "unary_operation"s) {
        result = this->visit_unary_operation(node, env);
    } else if (node.type() == "binary_operation"s) {
        result = this->visit_binary_operation(node, env);
    } else if (node.type() == std::string("function_call")) {
        result = this->visit_function_call(node, env);
    } else if (node.type() == "function_call"s) {
        result = this->visit_function_call(node, env);
    } else if (node.type() == "field_expression"s) {
        result = this->visit_field_expression(node, env);
    } else if (node.type() == "function"s || node.type() == "function_definition"s) {
        result = this->visit_function_expression(node, env);
    } else if (node.type() == "spread"s) {
        // TODO implement actual spread operator (this is for varargs)
        result = this->visit_vararg_expression(node, env);
    } else {
        throw UNIMPLEMENTED(node.type());
    }

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_vararg_expression(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == "spread"s);
    this->trace_enter_node(node);

    EvalResult result;

    auto varargs = env.get_varargs();
    if (!varargs.has_value()) {
        throw InterpreterException("cannot use '...' outside a vararg function");
    }

    // TODO return the whole vallist
    result.value = varargs->get(0);

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_function_expression(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == "function"s || node.type() == "function_definition"s);
    this->trace_enter_node(node);

    EvalResult result;

    ts::Cursor cursor(node);
    cursor.goto_first_child();
    cursor.goto_next_sibling();

    std::optional<std::string> ident;

    ts::Node current_node = cursor.current_node();
    if (current_node.type() == "function_name"s) {
        ident = this->visit_identifier(current_node.child(0).value(), env);
        cursor.goto_next_sibling();
    }

    current_node = cursor.current_node();
    assert(current_node.type() == "parameters"s);

    cursor.goto_first_child();
    cursor.goto_next_sibling();

    std::vector<std::string> parameters;
    bool vararg = false;

    while (true) {
        current_node = cursor.current_node();
        if (current_node.type() == ")"s) {
            break;
        }

        if (current_node.type() == "identifier"s) {
            auto ident = this->visit_identifier(current_node, env);
            parameters.push_back(ident);
        } else if (current_node.type() == "spread"s) {
            vararg = true;
            cursor.goto_next_sibling();
            assert(cursor.current_node().type() == ")"s);
        }

        // skip comma
        if (cursor.skip_n_siblings(2) != 2) {
            break;
        }
    }
    cursor.goto_next_sibling();

    cursor.goto_parent();
    cursor.goto_next_sibling();

    std::vector<ts::Node> body;

    while (true) {
        current_node = cursor.current_node();
        if (current_node.type() == "end"s) {
            break;
        }

        body.push_back(current_node);

        if (!cursor.goto_next_sibling()) {
            break;
        }
    }

    Value func = Function(
        [body = std::move(body), parameters = std::move(parameters), vararg,
         this](const CallContext& ctx) -> CallResult {
            // setup parameters as local variables
            Env env(ctx.environment().get_raw_impl().inner);
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

            EvalResult result;

            for (const auto& stmt : body) {
                auto sub_result = this->visit_statement(stmt, env);
                result.combine(sub_result);
                if (sub_result.do_return) {
                    break;
                }
            }

            auto return_value = result.do_return.value_or(Vallist());
            return CallResult(return_value, result.source_change);
        });

    if (ident) {
        env.set_global(*ident, func);
    } else {
        result.value = func;
    }

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_field_expression(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == "field_expression"s);
    this->trace_enter_node(node);

    EvalResult result;

    auto lhs_result = this->visit_expression(node.child(0).value(), env);
    result.combine(lhs_result);

    assert(node.child(1).value().type() == "."s);

    ts::Node property_node = node.child(2).value();
    assert(property_node.type() == "property_identifier"s);
    std::string key = property_node.text();

    result.value = lhs_result.value[key];

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_table_constructor(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == "table"s);
    this->trace_enter_node(node);

    EvalResult result;

    ts::Cursor cursor(node);
    cursor.goto_first_child();

    assert(cursor.current_node().type() == "{"s);

    cursor.goto_next_sibling();

    Table table;

    int consecutive_key = 1;

    while (true) {
        if (cursor.current_node().type() == "}"s) {
            break;
        }
        assert(cursor.current_node().type() == "field"s);

        Value key;
        Value value;

        if (cursor.current_node().child_count() == 1) {
            // entries without key
            key = consecutive_key;

            cursor.goto_first_child();

            auto sub_result = this->visit_expression(cursor.current_node(), env);
            value = sub_result.value;
            result.combine(sub_result);

            cursor.goto_parent();

            ++consecutive_key;
        } else if (cursor.current_node().child_count() == 3) {
            // entries of the form expr = expr
            cursor.goto_first_child();

            ts::Node current_node = cursor.current_node();
            assert(current_node.type() == "identifier"s);
            key = this->visit_identifier(current_node, env);

            cursor.goto_next_sibling();
            assert(cursor.current_node().type() == "="s);
            cursor.goto_next_sibling();

            auto sub_result = this->visit_expression(cursor.current_node(), env);
            value = sub_result.value;
            result.combine(sub_result);

            cursor.goto_parent();
        } else if (cursor.current_node().child_count() == 5) { // NOLINT
            // entries of the form [expr] = expr
            cursor.goto_first_child();
            assert(cursor.current_node().type() == "["s);

            cursor.goto_next_sibling();
            auto key_result = this->visit_expression(cursor.current_node(), env);
            key = key_result.value;
            result.combine(key_result);

            cursor.goto_next_sibling();
            assert(cursor.current_node().type() == "]"s);

            cursor.goto_next_sibling();
            assert(cursor.current_node().type() == "="s);
            cursor.goto_next_sibling();

            auto sub_result = this->visit_expression(cursor.current_node(), env);
            value = sub_result.value;
            result.combine(sub_result);

            cursor.goto_parent();
        } else {
            throw InterpreterException("syntax error in table constructor");
        }

        // TODO if last entry is an expression and returns a vallist all elements should be
        // inserted consecutively

        table.set(key, value);

        cursor.goto_next_sibling();
        if (cursor.current_node().type() == "}"s) {
            break;
        }

        assert(cursor.current_node().type() == ","s || cursor.current_node().type() == ";"s);

        cursor.goto_next_sibling();
        if (cursor.current_node().type() == "}"s) {
            break;
        }
    }

    assert(cursor.current_node().type() == "}"s);

    result.value = table;

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_binary_operation(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == std::string("binary_operation"));
    this->trace_enter_node(node);

    EvalResult result;

    auto origin = convert_range(node.range());

    auto lhs_node = node.child(0).value();
    auto operator_node = node.child(1).value();
    auto rhs_node = node.child(2).value();

    EvalResult lhs_result = this->visit_expression(lhs_node, env);
    EvalResult rhs_result = this->visit_expression(rhs_node, env);

    auto impl_operator = [&result, &lhs_result, &rhs_result, &origin](auto f) {
        auto value = std::invoke(f, lhs_result.value, rhs_result.value, origin);
        result.combine(rhs_result);
        result.value = value;
    };

    if (operator_node.type() == "=="s) {
        impl_operator(&Value::equals);
    } else if (operator_node.type() == "~="s) {
        impl_operator(&Value::unequals);
    } else if (operator_node.type() == ">="s) {
        impl_operator(&Value::greater_than_or_equal);
    } else if (operator_node.type() == ">"s) {
        impl_operator(&Value::greater_than);
    } else if (operator_node.type() == "<="s) {
        impl_operator(&Value::less_than_or_equal);
    } else if (operator_node.type() == "<"s) {
        impl_operator(&Value::less_than);
    } else if (operator_node.type() == "+"s) {
        impl_operator(&Value::add);
    } else if (operator_node.type() == "-"s) {
        impl_operator(&Value::sub);
    } else if (operator_node.type() == "*"s) {
        impl_operator(&Value::mul);
    } else if (operator_node.type() == "/"s) {
        impl_operator(&Value::div);
    } else if (operator_node.type() == "^"s) {
        impl_operator(&Value::pow);
    } else if (operator_node.type() == "%"s) {
        impl_operator(&Value::mod);
    } else if (operator_node.type() == "&"s) {
        impl_operator(&Value::bit_and);
    } else if (operator_node.type() == "|"s) {
        impl_operator(&Value::bit_or);
    } else if (operator_node.type() == "and"s) {
        impl_operator(&Value::logic_and);
    } else if (operator_node.type() == "or"s) {
        impl_operator(&Value::logic_or);
    } else if (operator_node.type() == ".."s) {
        impl_operator(&Value::concat);
    } else {
        throw InterpreterException(
            "encountered unknown binary operator `" + std::string(operator_node.type()) + "`");
    }

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_unary_operation(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == std::string("unary_operation"));
    this->trace_enter_node(node);

    auto operator_node = node.child(0).value();
    auto expr = node.child(1).value();

    EvalResult result = this->visit_expression(expr, env);

    if (operator_node.type() == "-"s) {
        result.value = result.value.negate(convert_range(node.range()));
    } else if (operator_node.type() == "not"s) {
        result.value = result.value.invert(convert_range(node.range()));
    } else if (operator_node.type() == "#"s) {
        result.value = result.value.len(convert_range(node.range()));
    } else {
        throw InterpreterException(
            "encountered unknown unary operator `" + std::string(operator_node.type()) + "`");
    }

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_prefix(ast::Prefix prefix, Env& env) -> EvalResult {
    this->trace_enter_node(prefix.raw());

    EvalResult result = std::visit(
        overloaded{
            [](ast::Self) -> EvalResult { throw UNIMPLEMENTED("self"); },
            [](ast::GlobalVariable var) -> EvalResult { throw UNIMPLEMENTED("global variables"); },
            [this, &env](ast::VariableDeclarator variable_decl) {
                return std::visit(
                    overloaded{
                        [this, &env](ast::Identifier ident) {
                            EvalResult result;
                            result.value = env.get_var(this->visit_identifier(ident, env));
                            return result;
                        },
                        [](ast::FieldExpression field) -> EvalResult {
                            throw UNIMPLEMENTED("field expression");
                        },
                        [](ast::TableIndex table_index) -> EvalResult {
                            throw UNIMPLEMENTED("table index");
                        }},
                    variable_decl.var());
            },
            [this, &env](ast::FunctionCall call) {
                return EvalResult(this->visit_function_call(call, env));
            },
            [this, &env](ast::Expression expr) { return this->visit_expression(expr.raw(), env); }},
        prefix.options());

    this->trace_exit_node(prefix.raw());
    return result;
}

auto Interpreter::visit_function_call(ast::FunctionCall call, Env& env) -> EvalResult {
    this->trace_enter_node(call.raw());

    EvalResult result;

    auto function_obj_result = this->visit_prefix(call.id(), env);
    result.combine(function_obj_result);

    if (call.method()) {
        throw UNIMPLEMENTED("method calls");
    }

    auto argument_exprs = call.args();
    std::vector<Value> arguments;

    for (auto arg_expr : argument_exprs) {
        auto expr_result = this->visit_expression(arg_expr.raw(), env);
        result.combine(expr_result);
        arguments.push_back(expr_result.value);
    }

    // TODO add back function call tracing
    // this->trace_function_call(function_name, arguments);

    // call function
    // this will produce an error if the obj is not callable
    auto obj = function_obj_result.value;
    Environment environment = make_environment(env);
    auto ctx = CallContext(&environment).make_new(Vallist(arguments));

    try {
        CallResult call_result = obj.call(ctx);
        result.combine(EvalResult(call_result));
    } catch (const std::runtime_error& e) {
        std::string pos = call.raw().range().start.point.pretty(true);
        throw InterpreterException("failed to call function  ("s + pos + ") : " + e.what());
    }

    // TODO add back function call tracing
    // this->trace_function_call_result(function_name, result);

    this->trace_exit_node(call.raw());
    return result;
}

} // namespace minilua::details
