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
        std::cerr << ts::debug_print_node(child.raw()) << "\n";
        EvalResult sub_result = this->visit_statement(child.raw(), env);
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
                return this->visit_variable_declaration(node.raw(), env);
            },
            [this, &env](ast::DoStatement node) {
                return this->visit_do_statement(node.raw(), env);
            },
            [this, &env](ast::IfStatement node) {
                return this->visit_if_statement(node.raw(), env);
            },
            [this, &env](ast::WhileStatement node) {
                return this->visit_while_statement(node.raw(), env);
            },
            [this, &env](ast::RepeatStatement node) {
                return this->visit_repeat_until_statement(node.raw(), env);
            },
            [this, &env](ast::ForStatement node) -> EvalResult { throw UNIMPLEMENTED("for"); },
            [this, &env](ast::ForInStatement node) -> EvalResult { throw UNIMPLEMENTED("for in"); },
            [this, &env](ast::GoTo node) -> EvalResult { throw UNIMPLEMENTED("goto"); },
            [this, &env](ast::Break node) { return this->visit_break_statement(env); },
            [this, &env](ast::Label node) -> EvalResult { throw UNIMPLEMENTED("goto"); },
            [this, &env](ast::FunctionStatement node) {
                return this->visit_function_expression(node.raw(), env);
            },
            [this, &env](ast::LocalFunctionStatement node) {
                return this->visit_function_expression(node.raw(), env);
            },
            [this, &env](ast::FunctionCall node) -> EvalResult {
                return this->visit_function_call(node.raw(), env);
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

auto Interpreter::visit_do_statement(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == "do_statement"s);
    this->trace_enter_node(node);

    EvalResult result;

    ts::Cursor cursor{node};
    // skip first "do" node
    if (!cursor.goto_first_child() || !cursor.goto_next_sibling()) {
        return result;
    }

    Env block_env = this->enter_block(env);

    ts::Node current_node = cursor.current_node();
    while (current_node.type() != "end"s) {
        auto body_result = this->visit_statement(current_node, block_env);
        result.combine(body_result);

        if (result.do_break) {
            this->trace_exit_node(node, std::nullopt, "break");
            return result;
        }
        if (result.do_return) {
            this->trace_exit_node(node, std::nullopt, "return");
            return result;
        }

        if (!cursor.goto_next_sibling()) {
            throw InterpreterException("syntax error: found no end node of if statement");
        }
        current_node = cursor.current_node();
    }

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_if_statement(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == "if_statement"s);
    this->trace_enter_node(node);

    EvalResult result;

    assert(node.child(0).value().type() == "if"s);
    auto condition_node = node.child(1).value();
    assert(node.child(2).value().type() == "then"s);

    auto condition_result = this->visit_expression(condition_node.child(0).value(), env);
    result.combine(condition_result);

    // NOTE we create a cursor from the root if_statement node because we need
    // to be able to see the siblings of the children
    ts::Cursor cursor{node};
    // navigate to first child after "then"
    cursor.goto_first_child();
    cursor.skip_n_siblings(3);

    // "then block"
    if (condition_result.value) {
        auto then_result = this->visit_if_arm(cursor, env);
        result.combine(then_result);

        this->trace_exit_node(node);
        return result;
    } else {
        // step through (but ignore) body
        cursor.skip_siblings_while([](ts::Node node) {
            return node.type() != "elseif"s && node.type() != "else"s && node.type() != "end"s;
        });
    }

    // elseif blocks
    do {
        ts::Node current_node = cursor.current_node();
        if (current_node.type() != "elseif"s) {
            break;
        }

        auto [elseif_result, was_executed] = this->visit_elseif_statement(current_node, env);

        result.combine(elseif_result);

        if (was_executed) {
            this->trace_exit_node(node);
            return result;
        }
    } while (cursor.goto_next_sibling());

    // else block
    if (cursor.current_node().type() == "else"s) {
        auto else_result = this->visit_else_statement(cursor.current_node(), env);
        result.combine(else_result);

        if (result.do_break) {
            this->trace_exit_node(node, std::nullopt, "break");
            return result;
        }
        if (result.do_return) {
            this->trace_exit_node(node, std::nullopt, "return");
            return result;
        }
    }

    auto last_node = node.child(node.child_count() - 1).value();
    if (last_node.type() != "end"s) {
        throw InterpreterException(
            "Last node of if is not `end` but `" + std::string(last_node.type()) + "`");
    }

    this->trace_exit_node(node);
    return result;
}
auto Interpreter::visit_if_arm(ts::Cursor& cursor, Env& env) -> EvalResult {
    // NOTE expects cursor to be at the first node of the body or on "end", "elseif" or "else"
    EvalResult result;

    Env block_env = this->enter_block(env);

    ts::Node current_node = cursor.current_node();
    while (current_node.type() != "end"s && current_node.type() != "elseif"s &&
           current_node.type() != "else"s) {
        auto body_result = this->visit_statement(current_node, block_env);
        result.combine(body_result);

        if (result.do_break) {
            return result;
        }
        if (result.do_return) {
            return result;
        }

        if (!cursor.goto_next_sibling()) {
            throw InterpreterException("syntax error: found no end node of if statement");
        }
        current_node = cursor.current_node();
    }

    return result;
}
// returns true if the elseif body was executed
auto Interpreter::visit_elseif_statement(ts::Node node, Env& env) -> std::pair<EvalResult, bool> {
    assert(node.type() == "elseif"s);
    this->trace_enter_node(node);

    EvalResult result;

    assert(node.child(0).value().type() == "elseif"s);

    auto condition_node = node.child(1).value();
    assert(condition_node.type() == "condition_expression"s);

    assert(node.child(2).value().type() == "then"s);

    auto condition_result = this->visit_expression(condition_node.child(0).value(), env);
    result.combine(condition_result);

    if (!condition_result.value) {
        return std::make_pair(result, false);
    }

    ts::Cursor cursor(node);
    cursor.goto_first_child();
    if (cursor.skip_n_siblings(3) < 3) {
        // TODO should never happen
        return std::make_pair(result, true);
    }

    Env block_env = this->enter_block(env);

    do {
        auto body_node = cursor.current_node();
        auto body_result = this->visit_statement(body_node, block_env);
        result.combine(body_result);

        if (result.do_break) {
            this->trace_exit_node(node, std::nullopt, "break");
            return std::make_pair(result, true);
        }
        if (result.do_return) {
            this->trace_exit_node(node, std::nullopt, "return");
            return std::make_pair(result, true);
        }
    } while (cursor.goto_next_sibling());

    this->trace_exit_node(node);
    return std::make_pair(result, true);
}
auto Interpreter::visit_else_statement(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == "else"s);
    this->trace_enter_node(node);

    EvalResult result;

    assert(node.child(0).value().type() == "else"s);

    ts::Cursor cursor(node);
    cursor.goto_first_child();
    if (!cursor.goto_next_sibling()) {
        return result;
    }

    Env block_env = this->enter_block(env);

    do {
        auto body_node = cursor.current_node();
        auto body_result = this->visit_statement(body_node, block_env);
        result.combine(body_result);

        if (result.do_break) {
            this->trace_exit_node(node, std::nullopt, "break");
            return result;
        }
        if (result.do_return) {
            this->trace_exit_node(node, std::nullopt, "return");
            return result;
        }
    } while (cursor.goto_next_sibling());

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_while_statement(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == "while_statement"s);
    this->trace_enter_node(node);

    EvalResult result;

    assert(node.child(0).value().type() == "while"s);

    auto condition_node = node.child(1).value();
    assert(condition_node.type() == "condition_expression"s);

    assert(node.child(2).value().type() == "do"s);

    ts::Cursor cursor(node);

    while (true) {
        auto condition_result = this->visit_expression(condition_node.child(0).value(), env);
        result.combine(condition_result);

        if (!condition_result.value) {
            return result;
        }

        cursor.reset(node);
        cursor.goto_first_child();
        if (cursor.skip_n_siblings(3) < 3) {
            return result;
        }

        Env block_env = this->enter_block(env);

        do {
            ts::Node body_node = cursor.current_node();

            if (body_node.type() == "end"s) {
                break;
            }

            auto body_result = this->visit_statement(body_node, block_env);
            result.combine(body_result);

            if (result.do_break) {
                this->trace_exit_node(node, std::nullopt, "break");
                result.do_break = false;
                return result;
            }
            if (result.do_return) {
                this->trace_exit_node(node, std::nullopt, "return");
                return result;
            }
        } while (cursor.goto_next_sibling());
    }

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_repeat_until_statement(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == "repeat_statement"s);
    this->trace_enter_node(node);

    EvalResult result;

    assert(node.child(0).value().type() == "repeat"s);

    ts::Cursor cursor(node);

    while (true) {
        cursor.reset(node);
        cursor.goto_first_child();
        if (!cursor.goto_next_sibling()) {
            throw InterpreterException("syntax error at start of repeat until block");
        }

        Env block_env = this->enter_block(env);

        do {
            ts::Node body_node = cursor.current_node();

            if (body_node.type() == "until"s) {
                break;
            }

            auto body_result = this->visit_statement(body_node, block_env);
            result.combine(body_result);

            if (result.do_break) {
                this->trace_exit_node(node, std::nullopt, "break");
                result.do_break = false;
                return result;
            }
            if (result.do_return) {
                this->trace_exit_node(node, std::nullopt, "return");
                return result;
            }
        } while (cursor.goto_next_sibling());

        assert(cursor.current_node().type() == "until"s);
        if (!cursor.goto_next_sibling()) {
            throw InterpreterException("syntax error at end of repeat until block");
        }
        auto condition_node = cursor.current_node();

        // the condition is part of the same block and can access local variables
        // declared in the repeat block
        auto condition_result = this->visit_expression(condition_node.child(0).value(), block_env);
        result.combine(condition_result);

        if (condition_result.value) {
            return result;
        }
    }

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_break_statement(Env& env) -> EvalResult {
    EvalResult result;
    result.do_break = true;
    return result;
}
auto Interpreter::visit_return_statement(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == "return_statement"s);
    this->trace_enter_node(node);

    ts::Cursor cursor(node);
    cursor.goto_first_child();

    assert(cursor.current_node().type() == "return"s);

    EvalResult result;
    result.do_return = Vallist();

    if (!cursor.goto_next_sibling()) {
        return result;
    }

    std::vector<Value> return_values;

    while (true) {
        auto sub_node = cursor.current_node();
        auto sub_result = this->visit_expression(sub_node, env);
        result.combine(sub_result);
        return_values.push_back(sub_result.value);

        if (cursor.skip_n_siblings(2) != 2) {
            break;
        }
    }

    result.do_return = Vallist(return_values);

    this->trace_exit_node(node);
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
        std::visit(
            overloaded{
                [this, &env, &value](ast::Identifier ident) {
                    env.set_var(this->visit_identifier(ident, env), value);
                },
                [](auto node) { throw UNIMPLEMENTED(node.raw().type()); }},
            target);
    }

    this->trace_exit_node(decl.raw());
    return result;
}

auto Interpreter::visit_local_variable_declaration(ts::Node node, Env& env) -> EvalResult {
    assert(node.type() == std::string("local_variable_declaration"));
    this->trace_enter_node(node);

    EvalResult result;

    assert(node.child(0).value().type() == "local"s);
    auto declarator = node.child(1).value();

    std::optional<ts::Node> expr;
    if (node.child(2)) {
        assert(node.child(2).value().type() == "="s);
        expr = node.child(3).value();
    }

    auto variable_name = this->visit_variable_declarator(declarator, env);
    Value initial_value;

    if (expr) {
        auto expr_result = this->visit_expression(*expr, env);
        result.combine(expr_result);
        initial_value = expr_result.value;
    }

    env.set_local(variable_name, initial_value);

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_variable_declarator(ts::Node node, Env& env) -> std::string {
    assert(node.type() == "variable_declarator"s || node.type() == "local_variable_declarator"s);
    this->trace_enter_node(node);
    return this->visit_identifier(node.child(0).value(), env);
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

auto Interpreter::visit_function_call(ts::Node node, Env& env) -> CallResult {
    assert(node.type() == std::string("function_call"));
    this->trace_enter_node(node);

    auto function_name = this->visit_identifier(node.named_child(0).value(), env);

    std::vector<Value> arguments;
    auto argument = node.named_child(1).value().child(0);
    // skip node `(` at the start
    argument = argument->next_sibling();

    while (argument.has_value() && argument->type() != ")"s) {
        auto expr = this->visit_expression(argument.value(), env);
        // TODO source_changes
        arguments.push_back(expr.value);

        // skip nodes `,` in the middle and node `)` at the end
        argument = argument->next_sibling();
        argument = argument->next_sibling();
    }

    this->trace_function_call(function_name, arguments);

    // call function
    // this will produce an error if the obj is not callable
    auto obj = env.get_var(function_name);
    Environment environment = make_environment(env);
    auto ctx = CallContext(&environment).make_new(Vallist(arguments));

    CallResult result;
    try {
        result = obj.call(ctx);
    } catch (const std::runtime_error& e) {
        std::string pos = node.range().start.point.pretty(true);
        throw InterpreterException(
            std::string("failed to call ") + function_name + " (" + pos + ") : " + e.what());
    }

    this->trace_function_call_result(function_name, result);

    this->trace_exit_node(node);
    return result;
}

} // namespace minilua::details
