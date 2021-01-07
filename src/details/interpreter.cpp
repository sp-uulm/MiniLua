#include "interpreter.hpp"
#include "tree_sitter/tree_sitter.hpp"

#include <cassert>
#include <iostream>
#include <set>

namespace minilua::details {

// TODO remove the unimplemented stuff once everything is implemented
class UnimplementedException : public std::runtime_error {
public:
    UnimplementedException(const std::string& where, const std::string& what)
        : std::runtime_error("unimplemented: \"" + what + "\" in " + where) {}
};

#define UNIMPLEMENTED(what)                                                                        \
    UnimplementedException(                                                                        \
        std::string(__func__) + " (" + std::string(__FILE__) + ":" + std::to_string(__LINE__) +    \
            ")",                                                                                   \
        what)

// class Interpreter
Interpreter::Interpreter(const InterpreterConfig& config) : config(config) {}

auto Interpreter::run(const ts::Tree& tree, Environment& env) -> EvalResult {
    return this->visit_root(tree.root_node(), env);
}

auto Interpreter::tracer() const -> std::ostream& { return *this->config.target; }
void Interpreter::trace_enter_node(ts::Node node) const {
    if (this->config.trace_nodes) {
        this->tracer() << "Enter node: " << ts::debug_print_node(node) << "\n";
    }
}
void Interpreter::trace_exit_node(ts::Node node) const {
    if (this->config.trace_nodes) {
        this->tracer() << "Exit node: " << ts::debug_print_node(node) << "\n";
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

static const std::set<std::string> IGNORE_NODES{";", "comment"};

static auto should_ignore_node(ts::Node node) -> bool {
    return IGNORE_NODES.find(node.type()) != IGNORE_NODES.end();
}

auto Interpreter::visit_root(ts::Node node, Environment& env) -> EvalResult {
    assert(node.type() == std::string("program"));
    this->trace_enter_node(node);

    for (auto child : node.children()) {
        if (child.type() == std::string("variable_declaration")) {
            this->visit_variable_declaration(child, env);
        } else if (child.type() == std::string("function_call")) {
            this->visit_function_call(child, env);
        } else if (should_ignore_node(child)) {
        } else {
            throw UNIMPLEMENTED(child.type());
        }
    }

    this->trace_exit_node(node);

    // TODO pass result through
    return EvalResult();
}

auto Interpreter::visit_variable_declaration(ts::Node node, Environment& env) -> EvalResult {
    assert(node.type() == std::string("variable_declaration"));
    this->trace_enter_node(node);

    auto declarator = node.named_child(0).value();
    auto expr = node.named_child(1).value();

    auto value = this->visit_expression(expr, env);

    env.add(this->visit_variable_declarator(declarator, env), value.value);

    this->trace_exit_node(node);
    return EvalResult();
}
auto Interpreter::visit_variable_declarator(ts::Node node, Environment& env) -> std::string {
    assert(node.type() == std::string("variable_declarator"));
    this->trace_enter_node(node);
    return this->visit_identifier(node.child(0).value(), env);
}

auto Interpreter::visit_identifier(ts::Node node, Environment& env) -> std::string {
    assert(node.type() == std::string("identifier"));
    this->trace_enter_node(node);
    this->trace_exit_node(node);
    return node.text();
}

auto Interpreter::visit_expression(ts::Node node, Environment& env) -> EvalResult {
    this->trace_enter_node(node);

    EvalResult result;

    if (node.type() == std::string("number")) {
        // TODO parse number
        auto value = parse_number(node.text());
        result.value = value;
    } else if (node.type() == std::string("identifier")) {
        auto variable_name = this->visit_identifier(node, env);
        result.value = env.get(variable_name);
    } else {
        throw UNIMPLEMENTED(node.type());
    }

    this->trace_exit_node(node);
    return result;
}

auto Interpreter::visit_function_call(ts::Node node, Environment& env) -> CallResult {
    assert(node.type() == std::string("function_call"));
    this->trace_enter_node(node);

    auto function_name = this->visit_identifier(node.named_child(0).value(), env);

    std::vector<Value> arguments;
    auto argument = node.named_child(1).value().child(0);
    // skip node `(` at the start
    argument = argument->next_sibling();

    while (argument.has_value()) {
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
    auto obj = env.get(function_name);
    auto ctx = CallContext(&env).make_new(Vallist(arguments));

    // TODO meta tables (might be handles by Value::call)
    auto result = obj.call(ctx);

    this->trace_exit_node(node);
    return result;
}

} // namespace minilua::details
