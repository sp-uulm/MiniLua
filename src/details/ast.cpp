#include "ast.hpp"
#include "../tree_sitter_lua.hpp"
#include "MiniLua/values.hpp"
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tree_sitter/tree_sitter.hpp>
#include <utility>

namespace minilua::details::ast {

using namespace std::string_literals;

// helper function
static auto convert_range(ts::Range range) -> Range {
    return Range{
        .start = {range.start.point.row, range.start.point.column, range.start.byte},
        .end = {range.end.point.row, range.end.point.column, range.end.byte},
    };
}
auto operator<<(std::ostream& os, const GEN_CAUSE& cause) -> std::ostream& {
    switch (cause) {
    case FOR_LOOP_DESUGAR:
        os << "for_statement desugaring";
        break;
    case FOR_IN_LOOP_DESUGAR:
        os << "for_in_statement desugaring";
        break;
    case FUNCTION_STATEMENT_DESUGAR:
        os << "function_statement desugaring";
        break;
    case METHOD_CALL_CONVERSION:
        os << "conversion of method_call to normal function_call";
        break;
    }
    return os;
}
static auto ast_class_to_string(const std::string& name, const minilua::Range& range)
    -> std::string {
    std::stringstream ss;
    ss << "(" << name << " " << range << ")";
    return ss.str();
}
static auto
ast_class_to_string(const std::string& name, const minilua::Range& range, GEN_CAUSE cause)
    -> std::string {
    std::stringstream ss;
    ss << "(" << name << " " << range << "generated for " << cause << ")";
    return ss.str();
}
static auto ast_class_to_string(
    const std::string& name, const minilua::Range& range, const std::string& content)
    -> std::string {
    std::stringstream ss;
    ss << "(" << name << " " << range << "|" << content << ")";
    return ss.str();
}
static auto ast_class_to_string(
    const std::string& name, const minilua::Range& range, const std::string& content,
    GEN_CAUSE cause) -> std::string {
    std::stringstream ss;
    ss << "(" << name << " " << range << "generated cause:" << cause << "|" << content << ")";
    return ss.str();
}
// Body
Body::Body(std::vector<ts::Node> node_vec) : content(std::move(node_vec)) {}
Body::Body(std::vector<Statement> stats, std::optional<Return> ret)
    : content(std::make_pair<>(std::move(stats), ret)) {}
auto Body::return_statement() -> std::optional<Return> {
    return std::visit(
        overloaded{
            [](std::vector<ts::Node> nodes) -> std::optional<Return> {
                if (nodes.empty() ||
                    nodes[nodes.size() - 1].type_id() != ts::NODE_RETURN_STATEMENT) {
                    return std::nullopt;
                } else {
                    return Return(nodes[nodes.size() - 1]);
                }
            },
            [](const BodyPair& pair) -> std::optional<Return> { return pair.second; }},
        this->content);
}
auto Body::statements() -> std::vector<Statement> {
    return std::visit(
        overloaded{
            [](std::vector<ts::Node> nodes) -> std::vector<Statement> {
                std::vector<ts::Node>::iterator end;
                std::vector<Statement> res;
                if (nodes.empty()) {
                    return res;
                }
                if ((nodes.end() - 1)->type_id() == ts::NODE_RETURN_STATEMENT) {
                    res.reserve(nodes.size() - 1);
                    end = nodes.end() - 1;
                } else {
                    res.reserve(nodes.size());
                    end = nodes.end();
                }
                std::transform(nodes.begin(), end, std::back_inserter(res), [](ts::Node node) {
                    return Statement(node);
                });
                return res;
            },
            [](const BodyPair& pair) -> std::vector<Statement> { return pair.first; }},
        this->content);
}
// Identifier
Identifier::IdStruct::IdStruct(std::string identifier, minilua::Range range, GEN_CAUSE gen_cause)
    : identifier(std::move(identifier)), range(std::move(range)), gen_cause(gen_cause) {}

Identifier::Identifier(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_IDENTIFIER && node.type_id() != ts::NODE_METHOD &&
        node.type_id() != ts::NODE_PROPERTY_IDENTIFIER &&
        node.type_id() != ts::NODE_FUNCTION_NAME_FIELD) {
        throw std::runtime_error("not an identifier node" + std::to_string(node.type_id()));
    }
}
Identifier::Identifier(const std::string& str, Range range, GEN_CAUSE cause)
    : content(IdStruct(str, std::move(range), cause)) {}
auto Identifier::string() const -> std::string {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::string { return node.text(); },
            [](const IdStruct& id_struct) -> std::string { return id_struct.identifier; }},
        this->content);
}
auto Identifier::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const IdStruct& id_struct) -> minilua::Range { return id_struct.range; }},
        this->content);
}
auto Identifier::debug_print() const -> std::string {
    std::string class_name = "identifier";
    return std::visit(
        overloaded{
            [class_name](const IdStruct& id_struct) {
                return ast_class_to_string(
                    class_name, id_struct.range, id_struct.identifier, id_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()), node.text());
            }},
        this->content);
}
// Program
Program::Program(ts::Node node) : program(node) {
    if (node.type_id() != ts::NODE_PROGRAM) {
        throw std::runtime_error("not a program node");
    }
}
auto Program::body() const -> Body { return Body(this->program.named_children()); }
auto Program::range() const -> minilua::Range { return convert_range(this->program.range()); }
auto Program::debug_print() const -> std::string {
    return ast_class_to_string("program", this->range());
}
// BinaryOperation
BinaryOperation::BinOpStruct::BinOpStruct(
    const Expression& left, BinOpEnum bin_operator, const Expression& right, minilua::Range range,
    GEN_CAUSE gen_cause)
    : left(std::make_shared<Expression>(left)), bin_operator(bin_operator),
      right(std::make_shared<Expression>(right)), range(std::move(range)), gen_cause(gen_cause) {}

BinaryOperation::BinaryOperation(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_BINARY_OPERATION) {
        throw std::runtime_error("not a binary_operation node");
    }
    assert(node.named_child_count() == 3);
}
BinaryOperation::BinaryOperation(
    const Expression& left, BinOpEnum op_enum, const Expression& right, minilua::Range range,
    GEN_CAUSE cause)
    : content(BinOpStruct(left, op_enum, right, std::move(range), cause)) {}
auto BinaryOperation::left() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression { return Expression(node.named_child(0).value()); },
            [](const BinOpStruct& bin_struct) -> Expression { return *bin_struct.left; }},
        this->content);
}
auto BinaryOperation::right() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression { return Expression(node.named_child(2).value()); },
            [](const BinOpStruct& bin_struct) -> Expression { return *bin_struct.right; }},
        this->content);
}
auto BinaryOperation::binary_operator() const -> BinOpEnum {
    return std::visit(
        overloaded{
            [](ts::Node node) -> BinOpEnum {
                auto bin_op = node.named_child(1)->type_id();
                if (bin_op == ts::NODE_BIN_OP_ADDITION) {
                    return BinOpEnum::ADD;
                } else if (bin_op == ts::NODE_BIN_OP_SUBTRACTION) {
                    return BinOpEnum::SUB;
                } else if (bin_op == ts::NODE_BIN_OP_DIVISION) {
                    return BinOpEnum::DIV;
                } else if (bin_op == ts::NODE_BIN_OP_MULTIPLICATION) {
                    return BinOpEnum::MUL;
                } else if (bin_op == ts::NODE_BIN_OP_MODULO) {
                    return BinOpEnum::MOD;
                } else if (bin_op == ts::NODE_BIN_OP_POWER) {
                    return BinOpEnum::POW;
                } else if (bin_op == ts::NODE_BIN_OP_LT) {
                    return BinOpEnum::LT;
                } else if (bin_op == ts::NODE_BIN_OP_GT) {
                    return BinOpEnum::GT;
                } else if (bin_op == ts::NODE_BIN_OP_LEQ) {
                    return BinOpEnum::LEQ;
                } else if (bin_op == ts::NODE_BIN_OP_GEQ) {
                    return BinOpEnum::GEQ;
                } else if (bin_op == ts::NODE_BIN_OP_EQ) {
                    return BinOpEnum::EQ;
                } else if (bin_op == ts::NODE_BIN_OP_NEQ) {
                    return BinOpEnum::NEQ;
                } else if (bin_op == ts::NODE_BIN_OP_CONCAT) {
                    return BinOpEnum::CONCAT;
                } else if (bin_op == ts::NODE_BIN_OP_LOGICAL_AND) {
                    return BinOpEnum::AND;
                } else if (bin_op == ts::NODE_BIN_OP_LOGICAL_OR) {
                    return BinOpEnum::OR;
                } else if (bin_op == ts::NODE_BIN_OP_SHIFT_LEFT) {
                    return BinOpEnum::SHIFT_LEFT;
                } else if (bin_op == ts::NODE_BIN_OP_SHIFT_RIGHT) {
                    return BinOpEnum::SHIFT_RIGHT;
                } else if (bin_op == ts::NODE_BIN_OP_INTEGER_DIVISION) {
                    return BinOpEnum::INT_DIV;
                } else if (bin_op == ts::NODE_BIN_OP_BITWISE_OR) {
                    return BinOpEnum::BIT_OR;
                } else if (bin_op == ts::NODE_BIN_OP_BITWISE_AND) {
                    return BinOpEnum::BIT_AND;
                } else if (bin_op == ts::NODE_BIN_OP_BITWISE_XOR) {
                    return BinOpEnum::BIT_XOR;
                } else {
                    throw std::runtime_error("Unknown Binary Operator");
                }
            },
            [](const BinOpStruct& bin_struct) -> BinOpEnum { return bin_struct.bin_operator; }},
        this->content);
}
auto BinaryOperation::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const BinOpStruct& bin_struct) -> minilua::Range { return bin_struct.range; }},
        this->content);
}
auto BinaryOperation::debug_print() const -> std::string {
    std::string class_name = "binary_operation";
    return std::visit(
        overloaded{
            [class_name](const BinOpStruct& bin_struct) {
                return ast_class_to_string(
                    "binary_operation", bin_struct.range, bin_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string("binary_operation", convert_range(node.range()));
            }},
        this->content);
}
// UnaryOperation
UnaryOperation::UnOpStruct::UnOpStruct(
    UnOpEnum un_operator, const Expression& operand, minilua::Range range, GEN_CAUSE gen_cause)
    : un_operator(un_operator), operand(std::make_shared<Expression>(operand)),
      range(std::move(range)), gen_cause(gen_cause) {}

UnaryOperation::UnaryOperation(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_UNARY_OPERATION) {
        throw std::runtime_error("not an unary_operation node");
    }
    assert(node.named_child_count() == 2);
}
UnaryOperation::UnaryOperation(
    UnOpEnum op_enum, const Expression& exp, minilua::Range range, GEN_CAUSE cause)
    : content(UnOpStruct(op_enum, exp, std::move(range), cause)) {}
auto UnaryOperation::unary_operator() const -> UnOpEnum {
    return std::visit(
        overloaded{
            [](ts::Node node) -> UnOpEnum {
                auto un_op = node.named_child(0)->type_id();
                if (un_op == ts::NODE_UN_OP_LOGICAL_NOT) {
                    return UnOpEnum::NOT;
                } else if (un_op == ts::NODE_UN_OP_NEGATIVE) {
                    return UnOpEnum::NEG;
                } else if (un_op == ts::NODE_UN_OP_BITWISE_NOT) {
                    return UnOpEnum::BWNOT;
                } else if (un_op == ts::NODE_UN_OP_LENGTH) {
                    return UnOpEnum::LEN;
                } else {
                    throw std::runtime_error("unknown Unary Operator: " + node.child(0)->text());
                }
            },
            [](const UnOpStruct& un_struct) -> UnOpEnum { return un_struct.un_operator; }},
        this->content);
}

auto UnaryOperation::expression() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression { return Expression(node.child(1).value()); },
            [](const UnOpStruct& un_struct) -> Expression { return *un_struct.operand; }},
        this->content);
}
auto UnaryOperation::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const UnOpStruct& un_struct) -> minilua::Range { return un_struct.range; }},
        this->content);
}
auto UnaryOperation::debug_print() const -> std::string {
    std::string class_name = "unary_operation";
    return std::visit(
        overloaded{
            [class_name](const UnOpStruct& un_struct) {
                return ast_class_to_string(class_name, un_struct.range, un_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}

// ForStatement
ForStatement::ForStatement(ts::Node node) : for_statement(node) {
    if (node.type_id() != ts::NODE_FOR_STATEMENT) {
        throw std::runtime_error("not a for_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_LOOP_EXPRESSION);
}
auto ForStatement::body() const -> Body {
    std::vector<ts::Node> body = this->for_statement.named_children();
    body.erase(body.begin());
    return Body(body);
}

auto ForStatement::loop_expression() const -> LoopExpression {
    return LoopExpression(this->for_statement.named_child(0).value());
}
auto ForStatement::range() const -> minilua::Range {
    return convert_range(this->for_statement.range());
}
auto ForStatement::debug_print() const -> std::string {
    return ast_class_to_string("for_statement", this->range());
}

LoopExpression::LoopExpression(ts::Node node) : loop_exp(node) {
    if (node.type_id() != ts::NODE_LOOP_EXPRESSION) {
        throw std::runtime_error("not a loop_expression node");
    }
    assert(node.named_child_count() == 3 || node.named_child_count() == 4);
}
auto LoopExpression::variable() const -> Identifier {
    return Identifier(this->loop_exp.named_child(0).value());
}
auto LoopExpression::end() const -> Expression {
    return Expression(this->loop_exp.named_child(2).value());
}
auto LoopExpression::start() const -> Expression {
    return Expression(this->loop_exp.named_child(1).value());
}
auto LoopExpression::step() const -> std::optional<Expression> {
    if (this->loop_exp.named_child_count() == 4) {
        return Expression(this->loop_exp.named_child(3).value());
    } else {
        return std::nullopt;
    }
}
auto LoopExpression::range() const -> minilua::Range {
    return convert_range(this->loop_exp.range());
}
auto LoopExpression::debug_print() const -> std::string {
    return ast_class_to_string("loop_expression", this->range());
}

InLoopExpression::InLoopExpression(ts::Node node) : loop_exp(node) {
    if (node.type_id() != ts::NODE_LOOP_EXPRESSION) {
        throw std::runtime_error("not a in_loop_expression node");
    }
    assert(node.named_child_count() == 2);
}
auto InLoopExpression::loop_exps() const -> std::vector<Expression> {
    std::vector<ts::Node> children = this->loop_exp.named_child(1).value().named_children();
    std::vector<Expression> exps;
    exps.reserve(children.size());
    std::transform(children.begin(), children.end(), std::back_inserter(exps), [](ts::Node node) {
        return Expression(node);
    });
    return exps;
}
auto InLoopExpression::loop_vars() const -> std::vector<Identifier> {
    std::vector<ts::Node> children = this->loop_exp.named_child(0).value().named_children();
    std::vector<Identifier> loop_vars;
    loop_vars.reserve(children.size());
    std::transform(
        children.begin(), children.end(), std::back_inserter(loop_vars),
        [](ts::Node node) { return Identifier(node); });
    return loop_vars;
}
auto InLoopExpression::range() const -> minilua::Range {
    return convert_range(this->loop_exp.range());
}
auto InLoopExpression::debug_print() const -> std::string {
    return ast_class_to_string("in_loop_expression", this->range());
}

// ForInStatement
ForInStatement::ForInStatement(ts::Node node) : for_in(node) {
    if (node.type_id() != ts::NODE_FOR_IN_STATEMENT) {
        throw std::runtime_error("not a for_in_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0)->type_id() == ts::NODE_LOOP_EXPRESSION);
}
auto ForInStatement::loop_expression() const -> InLoopExpression {
    return InLoopExpression(this->for_in.named_child(0).value());
}
auto ForInStatement::body() const -> Body {
    std::vector<ts::Node> children = this->for_in.named_children();
    children.erase(children.begin());
    return Body(children);
}
auto ForInStatement::range() const -> minilua::Range { return convert_range(this->for_in.range()); }
auto ForInStatement::debug_print() const -> std::string {
    return ast_class_to_string("for_in_statement", this->range());
}

// WhileStatement
WhileStatement::WhileStruct::WhileStruct(
    const Expression& condition, const Body& body, minilua::Range range, GEN_CAUSE gen_cause)
    : condition(std::make_shared<Expression>(condition)), body(std::make_shared<Body>(body)),
      range(std::move(range)), gen_cause(gen_cause) {}

WhileStatement::WhileStatement(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_WHILE_STATEMENT) {
        throw std::runtime_error("not a while_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}
WhileStatement::WhileStatement(
    const Expression& cond, const Body& body, minilua::Range range, GEN_CAUSE cause)
    : content(WhileStruct(cond, body, std::move(range), cause)) {}
auto WhileStatement::body() const -> Body {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Body {
                std::vector<ts::Node> body = node.named_children();
                body.erase(body.begin());
                return Body(body);
            },
            [](const WhileStruct& while_struct) -> Body { return *while_struct.body; }},
        this->content);
}
auto WhileStatement::repeat_conditon() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression {
                return Expression(node.named_child(0).value().named_child(0).value());
            },
            [](const WhileStruct& while_struct) -> Expression { return *while_struct.condition; }},
        this->content);
}

auto WhileStatement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const WhileStruct& while_struct) -> minilua::Range { return while_struct.range; }},
        this->content);
}
auto WhileStatement::debug_print() const -> std::string {
    std::string class_name = "while_statement";
    return std::visit(
        overloaded{
            [class_name](const WhileStruct& while_struct) {
                return ast_class_to_string(class_name, while_struct.range, while_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}

RepeatStatement::RepeatStatement(ts::Node node) : repeat_statement(node) {
    if (node.type_id() != ts::NODE_REPEAT_STATEMENT) {
        throw std::runtime_error("not a repeat_statement node");
    }
    assert(
        node.named_child_count() >= 1 &&
        (node.named_children().end() - 1)->type_id() == ts::NODE_CONDITION_EXPRESSION);
}
auto RepeatStatement::body() const -> Body {
    std::vector<ts::Node> body = this->repeat_statement.named_children();
    body.pop_back();
    return Body(body);
}
auto RepeatStatement::repeat_condition() const -> Expression {
    return Expression(
        this->repeat_statement.named_child(this->repeat_statement.named_child_count() - 1)
            .value()
            .named_child(0)
            .value());
}
auto RepeatStatement::range() const -> minilua::Range {
    return convert_range(this->repeat_statement.range());
}
auto RepeatStatement::debug_print() const -> std::string {
    return ast_class_to_string("repeat_statement", this->range());
}

// If
IfStatement::IfStruct::IfStruct(
    const Expression& condition, const Body& body, minilua::Range range, GEN_CAUSE gen_cause)
    : condition(std::make_shared<Expression>(condition)), body(std::make_shared<Body>(body)),
      range(std::move(range)), gen_cause(gen_cause) {}

IfStatement::IfStatement(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_IF_STATEMENT) {
        throw std::runtime_error("not an if_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}
IfStatement::IfStatement(
    const Expression& cond, const Body& body, minilua::Range range, GEN_CAUSE cause)
    : content(IfStruct(cond, body, std::move(range), cause)) {}
auto IfStatement::condition() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression {
                return Expression(node.named_child(0).value().named_child(0).value());
            },
            [](const IfStruct& if_struct) -> Expression { return *if_struct.condition; }},
        this->content);
}
auto IfStatement::else_statement() const -> std::optional<Else> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<Else> {
                if (node.named_child(node.named_child_count() - 1).has_value() &&
                    node.named_child(node.named_child_count() - 1)->type_id() == ts::NODE_ELSE) {
                    return Else(node.named_child(node.named_child_count() - 1).value());
                } else {
                    return std::nullopt;
                }
            },
            [](const IfStruct& /*if_struct*/) -> std::optional<Else> { return std::nullopt; }},
        this->content);
}
auto IfStatement::elseifs() const -> std::vector<ElseIf> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::vector<ElseIf> {
                if (node.named_child_count() == 1) {
                    return std::vector<ElseIf>();
                }
                std::vector<ts::Node> if_children = node.named_children();
                std::vector<ts::Node>::iterator end;
                std::vector<ElseIf> res;
                if ((if_children.end() - 1)->type_id() == ts::NODE_ELSE) {
                    end = if_children.end() - 1;
                } else {
                    end = if_children.end();
                }
                auto begin = end;
                while ((begin - 1)->type_id() == ts::NODE_ELSEIF &&
                       begin - 1 != if_children.begin()) {
                    begin--;
                }
                if (begin == end) {
                    return std::vector<ElseIf>();
                }
                std::transform(begin, end, std::back_inserter(res), [](ts::Node node) {
                    return ElseIf(node);
                });
                return res;
            },
            [](const IfStruct& /*if_struct*/) -> std::vector<ElseIf> {
                return std::vector<ElseIf>();
            }},
        this->content);
}
auto IfStatement::body() const -> Body {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Body {
                if (node.named_child_count() == 1 ||
                    (node.named_child(1).has_value() &&
                     node.named_child(1)->type_id() == ts::NODE_ELSEIF)) {
                    return Body(std::vector<ts::Node>());
                }
                std::vector<ts::Node> if_children = node.named_children();
                std::vector<ts::Node>::iterator end;

                if ((if_children.end() - 1)->type_id() == ts::NODE_ELSE) {
                    end = if_children.end() - 1;
                } else {
                    end = if_children.end();
                }
                while ((end - 1)->type_id() == ts::NODE_ELSEIF) {
                    end--;
                }
                return Body(std::vector<ts::Node>(if_children.begin() + 1, end));
            },
            [](const IfStruct& if_struct) -> Body { return *if_struct.body; }},
        this->content);
}
auto IfStatement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const IfStruct& if_struct) -> minilua::Range { return if_struct.range; }},
        this->content);
}
auto IfStatement::debug_print() const -> std::string {
    std::string class_name = "if_statement";
    return std::visit(
        overloaded{
            [class_name](const IfStruct& if_struct) {
                return ast_class_to_string(class_name, if_struct.range, if_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}

// Else
Else::Else(ts::Node node) : else_statement(node) {
    if (node.type_id() != ts::NODE_ELSE) {
        throw std::runtime_error("Not an else_statement node");
    }
}
auto Else::body() const -> Body { return Body(else_statement.named_children()); }
auto Else::range() const -> minilua::Range { return convert_range(else_statement.range()); }
auto Else::debug_print() const -> std::string {
    return ast_class_to_string("else_statement", this->range());
}

// ElseIf
ElseIf::ElseIf(ts::Node node) : else_if(node) {
    if (node.type_id() != ts::NODE_ELSEIF) {
        throw std::runtime_error("not a else_if node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}
auto ElseIf::body() const -> Body {
    std::vector<ts::Node> body = this->else_if.named_children();
    body.erase(body.begin());
    return Body(body);
}
auto ElseIf::condition() const -> Expression {
    return Expression(this->else_if.named_child(0).value().named_child(0).value());
}
auto ElseIf::range() const -> minilua::Range { return convert_range(this->else_if.range()); }
auto ElseIf::debug_print() const -> std::string {
    return ast_class_to_string("else_if_statement", this->range());
}
// Return
Return::Return(ts::Node node) : expressions(node) {}
auto Return::exp_list() const -> std::vector<Expression> {
    std::vector<ts::Node> exps = this->expressions.named_children();
    std::vector<ts::Node>::iterator end;
    std::vector<Expression> res;
    end = exps.end();
    res.reserve(exps.size());
    std::transform(
        exps.begin(), end, std::back_inserter(res), [](ts::Node node) { return Expression(node); });
    return res;
}
auto Return::range() const -> minilua::Range { return convert_range(this->expressions.range()); }
auto Return::debug_print() const -> std::string {
    return ast_class_to_string("return_statement", this->range());
}

// VariableDeclaration
VariableDeclaration::VDStruct::VDStruct(
    std::vector<VariableDeclarator> declarators, std::vector<Expression> declarations,
    minilua::Range range, GEN_CAUSE gen_cause)
    : declarators(std::move(declarators)), declarations(std::move(declarations)),
      range(std::move(range)), gen_cause(gen_cause) {}

VariableDeclaration::VariableDeclaration(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_VARIABLE_DECLARATION &&
        node.type_id() != ts::NODE_LOCAL_VARIABLE_DECLARATION) {
        throw std::runtime_error("not a variable_declaration node");
    } else {
        local_dec = node.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION;
    }
}
VariableDeclaration::VariableDeclaration(
    bool local, const std::vector<VariableDeclarator>& v_declarators,
    const std::vector<Expression>& v_declarations, minilua::Range range, GEN_CAUSE cause)
    : content(VDStruct(v_declarators, v_declarations, std::move(range), cause)), local_dec(local) {}
auto VariableDeclaration::declarations() const -> std::vector<Expression> {
    return std::visit(
        overloaded{
            [this](ts::Node node) -> std::vector<Expression> {
                std::vector<Expression> res;
                if (this->local_dec) {
                    std::vector<ts::Node> nodes = node.named_children();
                    res.reserve(nodes.size() - 1);
                    std::transform(
                        nodes.begin() + 1, nodes.end(), std::back_inserter(res),
                        [](ts::Node node) { return Expression(node); });
                    return res;
                } else {
                    std::vector<ts::Node> nodes = node.named_children();
                    auto it = nodes.begin();
                    while (it->type_id() == ts::NODE_VARIABLE_DECLARATOR) {
                        it++;
                    }
                    res.reserve(nodes.size() - (it - nodes.begin()));
                    std::transform(it, nodes.end(), std::back_inserter(res), [](ts::Node node) {
                        return Expression(node);
                    });
                    return res;
                }
            },
            [](const VDStruct& vd_struct) -> std::vector<Expression> {
                return vd_struct.declarations;
            }},
        this->content);
}
auto VariableDeclaration::declarators() const -> std::vector<VariableDeclarator> {
    return std::visit(
        overloaded{
            [this](ts::Node node) -> std::vector<VariableDeclarator> {
                if (this->local_dec) {
                    std::vector<ts::Node> nodes = node.named_child(0).value().named_children();
                    std::vector<VariableDeclarator> res;
                    res.reserve(nodes.size());
                    std::transform(
                        nodes.begin(), nodes.end(), std::back_inserter(res),
                        [](ts::Node node) { return VariableDeclarator(node); });
                    return res;
                } else {
                    std::vector<ts::Node> nodes = node.named_children();
                    std::vector<VariableDeclarator> res;
                    auto it = nodes.begin();
                    while (it->type_id() == ts::NODE_VARIABLE_DECLARATOR) {
                        it++;
                    }
                    res.reserve(it - nodes.begin());
                    std::transform(nodes.begin(), it, std::back_inserter(res), [](ts::Node node) {
                        return VariableDeclarator(node);
                    });
                    return res;
                }
            },
            [](const VDStruct& vd_struct) -> std::vector<VariableDeclarator> {
                return vd_struct.declarators;
            }},
        this->content);
}
auto VariableDeclaration::local() const -> bool { return this->local_dec; }
auto VariableDeclaration::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const VDStruct& vd_struct) -> minilua::Range { return vd_struct.range; }},
        this->content);
}
auto VariableDeclaration::debug_print() const -> std::string {
    std::string class_name = "variable_declaration";
    return std::visit(
        overloaded{
            [class_name](const VDStruct& vd_struct) {
                return ast_class_to_string(class_name, vd_struct.range, vd_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}

// VariableDeclarator
VariableDeclarator::VDStruct::VDStruct(Identifier id, minilua::Range range, GEN_CAUSE gen_cause)
    : vd_variant(id), range(std::move(range)), gen_cause(gen_cause) {}
VariableDeclarator::VDStruct::VDStruct(
    FieldExpression fe, minilua::Range range, GEN_CAUSE gen_cause)
    : vd_variant(fe), range(std::move(range)), gen_cause(gen_cause) {}
VariableDeclarator::VariableDeclarator(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_VARIABLE_DECLARATOR && node.type_id() != ts::NODE_IDENTIFIER &&
        node.type_id() != ts::NODE_FIELD_EXPRESSION && node.type_id() != ts::NODE_TABLE_INDEX) {
        throw std::runtime_error("not a variable declarator");
    }
}
VariableDeclarator::VariableDeclarator(Identifier id, GEN_CAUSE cause)
    : content(VDStruct(id, id.range(), cause)) {}
VariableDeclarator::VariableDeclarator(FieldExpression fe, GEN_CAUSE cause)
    : content(VDStruct(fe, fe.range(), cause)) {}
auto VariableDeclarator::options() const -> std::variant<Identifier, FieldExpression, TableIndex> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> VarDecVariant {
                if (node.type_id() == ts::NODE_IDENTIFIER) {
                    return Identifier(node);
                } else if (node.type_id() == ts::NODE_FIELD_EXPRESSION) {
                    return FieldExpression(node);
                } else if (node.type_id() == ts::NODE_TABLE_INDEX) {
                    return TableIndex(node);
                }
                if (node.named_child(0).has_value()) {
                    if (node.named_child(0)->type_id() == ts::NODE_IDENTIFIER) {
                        return Identifier(node.named_child(0).value());
                    } else if (node.named_child(0)->type_id() == ts::NODE_FIELD_EXPRESSION) {
                        return FieldExpression(node.named_child(0).value());
                    } else if (node.named_child(0)->type_id() == ts::NODE_TABLE_INDEX) {
                        return TableIndex(node.named_child(0).value());
                    } else {
                        throw std::runtime_error("invalid variable declarator");
                    }
                } else {
                    throw std::runtime_error("invalid variable declarator");
                }
            },
            [](const VDStruct& vd_struct) -> VarDecVariant { return vd_struct.vd_variant; }},
        this->content);
}
auto VariableDeclarator::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) { return convert_range(node.range()); },
            [](const VDStruct& vd_struct) { return vd_struct.range; }},
        this->content);
}
auto VariableDeclarator::debug_print() const -> std::string {
    std::string class_name = "variable_declarator";
    auto var = this->options();
    if (auto* id = std::get_if<Identifier>(&var)) {
        auto id_text = id->string();
        return std::visit(
            overloaded{
                [class_name, id_text](const VDStruct& vd_struct) {
                    return ast_class_to_string(
                        class_name, vd_struct.range, id_text, vd_struct.gen_cause);
                },
                [class_name, id_text](ts::Node node) {
                    return ast_class_to_string(class_name, convert_range(node.range()), id_text);
                }},
            this->content);
    } else {
        return std::visit(
            overloaded{
                [class_name](const VDStruct& vd_struct) {
                    return ast_class_to_string(class_name, vd_struct.range, vd_struct.gen_cause);
                },
                [class_name](ts::Node node) {
                    return ast_class_to_string(class_name, convert_range(node.range()));
                }},
            this->content);
    }
}

// TableIndex
TableIndex::TableIndex(ts::Node node) : table_index(node) {
    if (node.type_id() != ts::NODE_TABLE_INDEX) {
        throw std::runtime_error("not a table_index node");
    }
    assert(node.named_child_count() == 2);
}
auto TableIndex::table() const -> Prefix {
    return Prefix(this->table_index.named_child(0).value());
}
auto TableIndex::index() const -> Expression {
    return Expression(this->table_index.named_child(1).value());
}
auto TableIndex::range() const -> minilua::Range {
    return convert_range(this->table_index.range());
}
auto TableIndex::debug_print() const -> std::string {
    return ast_class_to_string("table_index", this->range());
}

// DoStatement
DoStatement::DoStruct::DoStruct(const Body& body, minilua::Range range, GEN_CAUSE gen_cause)
    : body(std::make_shared<Body>(body)), range(std::move(range)), gen_cause(gen_cause) {}
DoStatement::DoStatement(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_DO_STATEMENT) {
        throw std::runtime_error("not a do_statement node");
    }
}
DoStatement::DoStatement(const Body& body, minilua::Range range, GEN_CAUSE cause)
    : content(DoStruct(body, std::move(range), cause)) {}
auto DoStatement::body() const -> Body {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Body { return Body(node.named_children()); },
            [](const DoStruct& doStruct) -> Body { return *doStruct.body; }},
        this->content);
}
auto DoStatement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const DoStruct& doStruct) -> minilua::Range { return doStruct.range; }},
        this->content);
}
auto DoStatement::debug_print() const -> std::string {
    std::string class_name = "do_statement";
    return std::visit(
        overloaded{
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            },
            [class_name](const DoStruct& doStruct) {
                return ast_class_to_string(class_name, doStruct.range, doStruct.gen_cause);
            }},
        this->content);
}

// FieldExpression
FieldExpression::FieldExpStruct::FieldExpStruct(
    const Prefix& prefix, Identifier identifier, minilua::Range range, GEN_CAUSE gen_cause)
    : table(std::make_shared<Prefix>(prefix)), property(std::move(identifier)),
      range(std::move(range)), gen_cause(gen_cause) {}

FieldExpression::FieldExpression(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_FIELD_EXPRESSION) {
        throw std::runtime_error("not a field_expression node");
    }
    assert(node.named_child_count() == 2);
}
FieldExpression::FieldExpression(
    const Prefix& prefix, Identifier identifier, minilua::Range range, GEN_CAUSE cause)
    : content(FieldExpStruct(prefix, std::move(identifier), std::move(range), cause)) {}
auto FieldExpression::table_id() const -> Prefix {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Prefix { return Prefix(node.named_child(0).value()); },
            [](const FieldExpStruct& fe_struct) -> Prefix { return *fe_struct.table; }},
        this->content);
}
auto FieldExpression::property_id() const -> Identifier {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Identifier { return Identifier(node.named_child(1).value()); },
            [](const FieldExpStruct& fe_struct) -> Identifier { return fe_struct.property; }},
        this->content);
}
auto FieldExpression::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const FieldExpStruct& fe_struct) -> minilua::Range { return fe_struct.range; }},
        this->content);
}
auto FieldExpression::debug_print() const -> std::string {
    std::string class_name = "field_expression";
    return std::visit(
        overloaded{
            [class_name](const FieldExpStruct& fe_struct) {
                return ast_class_to_string(class_name, fe_struct.range, fe_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}
// Label
Label::Label(ts::Node node) : label(node) {
    if (node.type_id() != ts::NODE_LABEL_STATEMENT) {
        throw std::runtime_error("not a label node");
    }
    assert(node.named_child_count() == 1);
}
auto Label::id() const -> Identifier { return Identifier(this->label.named_child(0).value()); }
auto Label::range() const -> minilua::Range { return convert_range(this->label.range()); }
auto Label::debug_print() const -> std::string {
    return ast_class_to_string("label", this->range());
}

// GoTo
GoTo::GoTo(ts::Node node) : go_to(node) {
    if (node.type_id() != ts::NODE_GOTO_STATEMENT) {
        throw std::runtime_error("not a go_to node");
    }
    assert(node.named_child_count() == 1);
}
auto GoTo::label() const -> Identifier { return Identifier(this->go_to.named_child(0).value()); }
auto GoTo::range() const -> minilua::Range { return convert_range(this->go_to.range()); }
auto GoTo::debug_print() const -> std::string {
    return ast_class_to_string("goto_statement", this->range());
}

// Parameters
Parameters::ParamStruct::ParamStruct(
    std::vector<Identifier> identifiers, bool spread, minilua::Range range, GEN_CAUSE gen_cause)
    : identifiers(std::move(identifiers)), spread(spread), range(std::move(range)),
      gen_cause(gen_cause) {}

Parameters::Parameters(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_PARAMETERS) {
        throw std::runtime_error("not a parameters node");
    }
}
Parameters::Parameters(
    std::vector<Identifier> params, bool spread, minilua::Range range, GEN_CAUSE cause)
    : content(ParamStruct(std::move(params), spread, std::move(range), cause)) {}
auto Parameters::spread() const -> bool {
    return std::visit(
        overloaded{
            [](ts::Node node) -> bool {
                return node.named_child_count() > 0 &&
                       ((node.named_child(node.named_child_count() - 1))->type_id() ==
                        ts::NODE_SPREAD);
            },
            [](const ParamStruct& param_struct) { return param_struct.spread; }},
        this->content);
}
auto Parameters::params() const -> std::vector<Identifier> {
    const bool spread = this->spread();
    return std::visit(
        overloaded{
            [spread](ts::Node node) -> std::vector<Identifier> {
                std::vector<Identifier> res;
                std::vector<ts::Node> children = node.named_children();
                if (spread) {
                    if (node.named_child_count() < 1) {
                        return res;
                    }
                    res.reserve(node.named_child_count() - 1);
                    std::transform(
                        children.begin(), children.end() - 1, std::back_inserter(res),
                        [](ts::Node id) { return Identifier(id); });
                    return res;
                } else {
                    res.reserve(node.named_child_count());
                    std::transform(
                        children.begin(), children.end(), std::back_inserter(res),
                        [](ts::Node id) { return Identifier(id); });
                    return res;
                }
            },
            [](const ParamStruct& param_struct) -> std::vector<Identifier> {
                return param_struct.identifiers;
            }},
        this->content);
}
auto Parameters::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const ParamStruct& param_struct) -> minilua::Range { return param_struct.range; }},
        this->content);
}
auto Parameters::debug_print() const -> std::string {
    std::string class_name = "parameters";
    return std::visit(
        overloaded{
            [class_name](const ParamStruct& param_struct) {
                return ast_class_to_string(class_name, param_struct.range, param_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}

// FunctionName
FunctionName::FunctionName(ts::Node node) : func_name(node) {
    if (node.type_id() != ts::NODE_FUNCTION_NAME && node.type_id() != ts::NODE_IDENTIFIER) {
        throw std::runtime_error("Not a function_name node");
    }
}
auto FunctionName::method() const -> std::optional<Identifier> {
    if (this->func_name.type_id() == ts::NODE_IDENTIFIER) {
        return std::nullopt;
    }
    if (this->func_name.named_child(this->func_name.named_child_count() - 1)->type_id() ==
        ts::NODE_METHOD) {
        return Identifier(
            this->func_name.named_child(this->func_name.named_child_count() - 1).value());
    } else {
        return std::nullopt;
    }
}
auto FunctionName::identifier() const -> std::vector<Identifier> {
    if (this->func_name.type_id() == ts::NODE_IDENTIFIER) {
        return std::vector<Identifier>{Identifier(this->func_name)};
    }
    if (this->func_name.named_child(0)->type_id() == ts::NODE_IDENTIFIER) {
        return std::vector<Identifier>{Identifier(this->func_name.named_child(0).value())};
    } else {
        std::vector<Identifier> res;
        std::vector<ts::Node> children = this->func_name.named_child(0).value().named_children();
        res.reserve(children.size());
        std::transform(
            children.begin(), children.end(), std::back_inserter(res),
            [](ts::Node node) { return Identifier(node); });
        return res;
    }
}
auto FunctionName::range() const -> minilua::Range {
    return convert_range(this->func_name.range());
}
auto FunctionName::debug_print() const -> std::string {
    return ast_class_to_string("functionname", this->range());
}

// FunctionDefinition
FunctionDefinition::FuncDefStruct::FuncDefStruct(
    Parameters parameters, const Body& body, minilua::Range range, GEN_CAUSE gen_cause)
    : parameters(std::move(parameters)), body(std::make_shared<Body>(body)),
      range(std::move(range)), gen_cause(gen_cause) {}

FunctionDefinition::FunctionDefinition(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_FUNCTION_DEFINITION) {
        throw std::runtime_error("not a function_definition node");
    }
}
FunctionDefinition::FunctionDefinition(
    const Parameters& params, const Body& body, minilua::Range range, GEN_CAUSE cause)
    : content(FuncDefStruct(params, body, std::move(range), cause)) {}
auto FunctionDefinition::parameters() const -> Parameters {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Parameters { return Parameters(node.named_child(0).value()); },
            [](const FuncDefStruct& fd_struct) -> Parameters { return fd_struct.parameters; }},
        this->content);
}
auto FunctionDefinition::body() const -> Body {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Body {
                std::vector<ts::Node> children = node.named_children();
                return Body(std::vector<ts::Node>(children.begin() + 1, children.end()));
            },
            [](const FuncDefStruct& fd_struct) -> Body { return *fd_struct.body; }},
        this->content);
}
auto FunctionDefinition::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const FuncDefStruct& fd_struct) -> minilua::Range { return fd_struct.range; }},
        this->content);
}
auto FunctionDefinition::debug_print() const -> std::string {
    std::string class_name = "function_definition";
    return std::visit(
        overloaded{
            [class_name](const FuncDefStruct& fd_struct) {
                return ast_class_to_string(class_name, fd_struct.range, fd_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}

// FunctionStatement
FunctionStatement::FunctionStatement(ts::Node node) : func_stat(node) {
    if (node.type_id() != ts::NODE_FUNCTION && node.type_id() != ts::NODE_LOCAL_FUNCTION) {
        throw std::runtime_error("not a function(_statement) node");
    }
    is_local = node.type_id() == ts::NODE_LOCAL_FUNCTION;
}
auto FunctionStatement::body() const -> Body {
    std::vector<ts::Node> children = this->func_stat.named_children();
    return Body(std::vector<ts::Node>(children.begin() + 2, children.end()));
}
auto FunctionStatement::name() const -> FunctionName {
    return FunctionName(this->func_stat.named_child(0).value());
}
auto FunctionStatement::parameters() const -> Parameters {
    return Parameters(this->func_stat.named_child(1).value());
}
auto FunctionStatement::local() const -> bool { return this->is_local; }
auto FunctionStatement::range() const -> minilua::Range {
    return convert_range(this->func_stat.range());
}
auto FunctionStatement::debug_print() const -> std::string {
    return ast_class_to_string("function_statement", this->range());
}

// FunctionCall
FunctionCall::FuncCallStruct::FuncCallStruct(
    const Prefix& prefix, std::optional<Identifier> method, std::vector<Expression> args,
    minilua::Range range, GEN_CAUSE gen_cause)
    : prefix(std::make_shared<Prefix>(prefix)), method(std::move(method)), args(std::move(args)),
      range(std::move(range)), gen_cause(gen_cause) {}

FunctionCall::FunctionCall(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_FUNCTION_CALL) {
        throw std::runtime_error("not a function_call node");
    }
    assert(node.named_child_count() == 2 || node.named_child_count() == 3);
}
FunctionCall::FunctionCall(
    const Prefix& pfx, std::optional<Identifier> method, std::vector<Expression> args,
    minilua::Range range, GEN_CAUSE cause)
    : content(FuncCallStruct(pfx, std::move(method), std::move(args), std::move(range), cause)) {}
auto FunctionCall::prefix() const -> Prefix {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Prefix { return Prefix(node.named_child(0).value()); },
            [](const FuncCallStruct& fc_struct) -> Prefix { return *fc_struct.prefix; }},
        this->content);
}
auto FunctionCall::method() const -> std::optional<Identifier> {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::optional<Identifier> {
                if (node.named_child_count() == 3) {
                    return Identifier(node.named_child(1).value());
                } else {
                    return std::nullopt;
                }
            },
            [](const FuncCallStruct& fc_struct) { return fc_struct.method; }},
        this->content);
}
auto FunctionCall::id() const -> Prefix {
    auto prefix = this->prefix();
    auto method = this->method();
    if (method.has_value()) {
        prefix = Prefix(
            VariableDeclarator(
                FieldExpression(prefix, method.value(), this->range(), METHOD_CALL_CONVERSION),
                METHOD_CALL_CONVERSION),
            METHOD_CALL_CONVERSION);
    }
    return prefix;
}
auto FunctionCall::args() const -> std::vector<Expression> {
    auto exp_list = std::visit(
        overloaded{
            [](ts::Node node) -> std::vector<Expression> {
                std::vector<ts::Node> arg_nodes =
                    node.named_child(node.named_child_count() - 1).value().named_children();
                std::vector<Expression> args;
                args.reserve(arg_nodes.size());
                std::transform(
                    arg_nodes.begin(), arg_nodes.end(), std::back_inserter(args),
                    [](ts::Node node) { return Expression(node); });
                return args;
            },
            [](const FuncCallStruct& fc_struct) -> std::vector<Expression> {
                return fc_struct.args;
            }},
        this->content);
    if (this->method().has_value()) {
        exp_list.insert(exp_list.begin(), Expression(this->prefix(), METHOD_CALL_CONVERSION));
    }
    return exp_list;
}
auto FunctionCall::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const FuncCallStruct& fc_struct) -> minilua::Range { return fc_struct.range; }},
        this->content);
}
auto FunctionCall::debug_print() const -> std::string {
    std::string class_name = "function_call";
    return std::visit(
        overloaded{
            [class_name](const FuncCallStruct& fc_struct) {
                return ast_class_to_string(class_name, fc_struct.range, fc_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}

// Table
Table::Table(ts::Node node) : table(node) {
    if (node.type_id() != ts::NODE_TABLE) {
        throw std::runtime_error("not a table node");
    }
}
auto Table::fields() const -> std::vector<Field> {
    if (this->table.named_child_count() < 1) {
        return std::vector<Field>();
    }
    std::vector<Field> fields;
    fields.reserve(this->table.named_child_count());
    std::vector<ts::Node> nodes = this->table.named_children();
    std::transform(nodes.begin(), nodes.end(), std::back_inserter(fields), [](ts::Node node) {
        return Field(node);
    });
    return fields;
}
auto Table::range() const -> minilua::Range { return convert_range(this->table.range()); }
auto Table::debug_print() const -> std::string {
    return ast_class_to_string("table", this->range());
}

// Field
Field::Field(ts::Node node) : field(node) {
    if (node.type_id() != ts::NODE_FIELD) {
        throw std::runtime_error("not a field node");
    }
    assert(node.named_child_count() == 1 || node.named_child_count() == 2);
}
auto Field::content() const -> std::variant<
    std::pair<Expression, Expression>, std::pair<Identifier, Expression>, Expression> {
    if (this->field.named_child_count() < 2) {
        return Expression(this->field.named_child(0).value());
    } else if (this->field.child(0)->text() == "[") {
        return std::pair<Expression, Expression>(
            Expression(this->field.named_child(0).value()),
            Expression(field.named_child(1).value()));
    } else {
        return std::pair<Identifier, Expression>(
            Identifier(this->field.named_child(0).value()),
            Expression(field.named_child(1).value()));
    }
}
auto Field::range() const -> minilua::Range { return convert_range(this->field.range()); }
auto Field::debug_print() const -> std::string {
    return ast_class_to_string("field", this->range());
}

// Prefix
Prefix::PrefixStruct::PrefixStruct(FunctionCall fc, minilua::Range range, GEN_CAUSE gen_cause)
    : prefix_variant(fc), range(std::move(range)), gen_cause(gen_cause) {}
Prefix::PrefixStruct::PrefixStruct(VariableDeclarator vd, minilua::Range range, GEN_CAUSE gen_cause)
    : prefix_variant(vd), range(std::move(range)), gen_cause(gen_cause) {}

Prefix::Prefix(ts::Node node) : content(node) {
    if (!(node.type_id() == ts::NODE_FUNCTION_CALL || node.type_id() == ts::NODE_IDENTIFIER ||
          node.type_id() == ts::NODE_FIELD_EXPRESSION || node.type_id() == ts::NODE_TABLE_INDEX)) {
        throw std::runtime_error("Not a prefix-node");
    }
}
Prefix::Prefix(VariableDeclarator vd, GEN_CAUSE cause)
    : content(PrefixStruct(vd, vd.range(), cause)) {}
Prefix::Prefix(FunctionCall fc, GEN_CAUSE cause) : content(PrefixStruct(fc, fc.range(), cause)) {}
auto Prefix::options() const -> PrefixVariant {
    return std::visit(
        overloaded{
            [](ts::Node node) -> PrefixVariant {
                if (node.type_id() == ts::NODE_FUNCTION_CALL) {
                    return FunctionCall(node);
                } else if (node.child_count() > 0 && node.child(0)->text() == "(") {
                    return Expression(node.child(1).value());
                } else if (
                    node.type_id() == ts::NODE_IDENTIFIER ||
                    node.type_id() == ts::NODE_FIELD_EXPRESSION ||
                    node.type_id() == ts::NODE_TABLE_INDEX) {
                    return VariableDeclarator(node);
                } else {
                    throw std::runtime_error("Not a prefix-node");
                }
            },
            [](const PrefixStruct& pfx_struct) -> PrefixVariant {
                auto mod_var = pfx_struct.prefix_variant;
                return std::visit(
                    overloaded{
                        [](VariableDeclarator vd) -> PrefixVariant { return vd; },
                        [](FunctionCall fc) -> PrefixVariant { return fc; },
                        [](const std::shared_ptr<Expression>& exp) -> PrefixVariant {
                            return *exp;
                        }},
                    mod_var);
            }},
        this->content);
}
auto Prefix::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) { return convert_range(node.range()); },
            [](const PrefixStruct& pfx_struct) { return pfx_struct.range; }},
        this->content);
}
auto Prefix::debug_print() const -> std::string {
    std::string class_name = "prefix";
    return std::visit(
        overloaded{
            [class_name](const PrefixStruct& pfx_struct) {
                return ast_class_to_string(class_name, pfx_struct.range, pfx_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}
auto Prefix::to_string() const -> std::string {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::string { return node.text(); },
            [](const PrefixStruct& pfx_struct) -> std::string {
                auto variant = pfx_struct.prefix_variant;
                if (auto* var_dec = std::get_if<VariableDeclarator>(&variant)) {
                    auto options = var_dec->options();
                    if (auto* id = std::get_if<Identifier>(&options)) {
                        return id->string();
                    } else {
                        return "non printable prefix";
                    }
                } else {
                    return "non printable prefix";
                }
            }},
        this->content);
}

// Expression
Expression::ExpStruct::ExpStruct(BinaryOperation bin_op, minilua::Range range, GEN_CAUSE gen_cause)
    : exp_variant(bin_op), range(std::move(range)), gen_cause(gen_cause) {}
Expression::ExpStruct::ExpStruct(UnaryOperation un_op, minilua::Range range, GEN_CAUSE gen_cause)
    : exp_variant(un_op), range(std::move(range)), gen_cause(gen_cause) {}
Expression::ExpStruct::ExpStruct(
    FunctionDefinition func_def, minilua::Range range, GEN_CAUSE gen_cause)
    : exp_variant(func_def), range(std::move(range)), gen_cause(gen_cause) {}
Expression::ExpStruct::ExpStruct(Prefix prefix, minilua::Range range, GEN_CAUSE gen_cause)
    : exp_variant(prefix), range(std::move(range)), gen_cause(gen_cause) {}
Expression::ExpStruct::ExpStruct(Literal literal, minilua::Range range, GEN_CAUSE gen_cause)
    : exp_variant(literal), range(std::move(range)), gen_cause(gen_cause) {}
Expression::ExpStruct::ExpStruct(Identifier id, minilua::Range range, GEN_CAUSE gen_cause)
    : exp_variant(id), range(std::move(range)), gen_cause(gen_cause) {}

Expression::Expression(ts::Node node) : content(node) {
    if (!(node.type_id() == ts::NODE_SPREAD || node.type_id() == ts::NODE_FUNCTION_DEFINITION ||
          node.type_id() == ts::NODE_TABLE || node.type_id() == ts::NODE_BINARY_OPERATION ||
          node.type_id() == ts::NODE_UNARY_OPERATION || node.type_id() == ts::NODE_STRING ||
          node.type_id() == ts::NODE_NUMBER || node.type_id() == ts::NODE_NIL ||
          node.type_id() == ts::NODE_FALSE || node.type_id() == ts::NODE_TRUE ||
          node.type_id() == ts::NODE_IDENTIFIER || node.type_id() == ts::NODE_FUNCTION_CALL ||
          node.type_id() == ts::NODE_FIELD_EXPRESSION || node.type_id() == ts::NODE_TABLE_INDEX ||
          node.child(0)->text() == "(")) {
        throw std::runtime_error("Not an expression-node");
    }
}
Expression::Expression(UnaryOperation un, GEN_CAUSE gen_cause)
    : content(ExpStruct(un, un.range(), gen_cause)) {}
Expression::Expression(BinaryOperation bin, GEN_CAUSE gen_cause)
    : content(ExpStruct(bin, bin.range(), gen_cause)) {}
Expression::Expression(FunctionDefinition fd, GEN_CAUSE gen_cause)
    : content(ExpStruct(fd, fd.range(), gen_cause)) {}
Expression::Expression(Literal literal, GEN_CAUSE gen_cause)
    : content(ExpStruct(literal, literal.range(), gen_cause)) {}
Expression::Expression(Identifier id, GEN_CAUSE gen_cause)
    : content(ExpStruct(id, id.range(), gen_cause)) {}
Expression::Expression(Prefix pfx, GEN_CAUSE gen_cause)
    : content(ExpStruct(pfx, pfx.range(), gen_cause)) {}
auto Expression::options() const -> ExpressionVariant {
    return std::visit(
        overloaded{
            [](ts::Node node) -> ExpressionVariant {
                if (node.type_id() == ts::NODE_SPREAD) {
                    return Spread();
                } else if (node.type_id() == ts::NODE_FUNCTION_DEFINITION) {
                    return FunctionDefinition(node);
                } else if (node.type_id() == ts::NODE_TABLE) {
                    return Table(node);
                } else if (node.type_id() == ts::NODE_BINARY_OPERATION) {
                    return BinaryOperation(node);
                } else if (node.type_id() == ts::NODE_UNARY_OPERATION) {
                    return UnaryOperation(node);
                } else if (node.type_id() == ts::NODE_STRING) {
                    return Literal(LiteralType::STRING, node.text(), convert_range(node.range()));
                } else if (node.type_id() == ts::NODE_NUMBER) {
                    return Literal(LiteralType::NUMBER, node.text(), convert_range(node.range()));
                } else if (node.type_id() == ts::NODE_NIL) {
                    return Literal(LiteralType::NIL, node.text(), convert_range(node.range()));
                } else if (node.type_id() == ts::NODE_TRUE) {
                    return Literal(LiteralType::TRUE, node.text(), convert_range(node.range()));
                } else if (node.type_id() == ts::NODE_FALSE) {
                    return Literal(LiteralType::FALSE, node.text(), convert_range(node.range()));
                } else if (node.type_id() == ts::NODE_IDENTIFIER) {
                    return Identifier(node);
                } else if (
                    node.type_id() == ts::NODE_FUNCTION_CALL ||
                    node.type_id() == ts::NODE_FIELD_EXPRESSION ||
                    node.type_id() == ts::NODE_TABLE_INDEX ||
                    (node.child(0).has_value() && node.child(0)->text() == "(")) {
                    return Prefix(node);
                } else {
                    throw std::runtime_error("Not an expression-node");
                }
            },
            [](const ExpStruct& exp_struct) -> ExpressionVariant {
                return exp_struct.exp_variant;
            }},
        this->content);
}
auto Expression::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const ExpStruct& exp_struct) -> minilua::Range { return exp_struct.range; }},
        this->content);
}
auto Expression::debug_print() const -> std::string {
    std::string class_name = "expression";
    auto var = this->options();
    if (auto* id = std::get_if<Identifier>(&var)) {
        auto id_text = id->string();
        return std::visit(
            overloaded{
                [class_name, id_text](const ExpStruct& exp_struct) {
                    return ast_class_to_string(
                        class_name, exp_struct.range, id_text, exp_struct.gen_cause);
                },
                [class_name, id_text](ts::Node node) {
                    return ast_class_to_string(class_name, convert_range(node.range()), id_text);
                }},
            this->content);
    } else if (auto* literal = std::get_if<Literal>(&var)) {
        auto literal_text = literal->content();
        return std::visit(
            overloaded{
                [class_name, literal_text](const ExpStruct& exp_struct) {
                    return ast_class_to_string(
                        class_name, exp_struct.range, literal_text, exp_struct.gen_cause);
                },
                [class_name, literal_text](ts::Node node) {
                    return ast_class_to_string(
                        class_name, convert_range(node.range()), literal_text);
                }},
            this->content);
    } else {
        return std::visit(
            overloaded{
                [class_name](const ExpStruct& exp_struct) {
                    return ast_class_to_string(class_name, exp_struct.range, exp_struct.gen_cause);
                },
                [class_name](ts::Node node) {
                    return ast_class_to_string(class_name, convert_range(node.range()));
                }},
            this->content);
    }
}
auto Expression::to_string() const -> std::string {
    return std::visit(
        overloaded{
            [](ts::Node node) { return node.text(); },
            [](const ExpStruct& exp_struct) {
                auto var = exp_struct.exp_variant;
                return std::visit(
                    overloaded{
                        [](Spread /*spread*/) -> std::string { return "..."; },
                        [](const Prefix& prefix) -> std::string { return prefix.to_string(); },
                        [](const FunctionDefinition& /*fd*/) -> std::string {
                            return "function_definition";
                        },
                        [](Table /*table*/) -> std::string { return "table"; },
                        [](const BinaryOperation& /*bin_op*/) -> std::string {
                            return "binary_operation";
                        },
                        [](const UnaryOperation& /*un_op*/) -> std::string {
                            return "unary_operation";
                        },
                        [](const Literal& literal) -> std::string { return literal.content(); },
                        [](const Identifier& id) -> std::string { return id.string(); }},
                    var);
            }},
        this->content);
}

// Statement
Statement::StatStruct::StatStruct(VariableDeclaration vd, minilua::Range range, GEN_CAUSE gen_cause)
    : stat_var(vd), range(std::move(range)), gen_cause(gen_cause) {}
Statement::StatStruct::StatStruct(FunctionCall fc, minilua::Range range, GEN_CAUSE gen_cause)
    : stat_var(fc), range(std::move(range)), gen_cause(gen_cause) {}
Statement::StatStruct::StatStruct(
    WhileStatement while_stat, minilua::Range range, GEN_CAUSE gen_cause)
    : stat_var(while_stat), range(std::move(range)), gen_cause(gen_cause) {}
Statement::StatStruct::StatStruct(IfStatement if_stat, minilua::Range range, GEN_CAUSE gen_cause)
    : stat_var(if_stat), range(std::move(range)), gen_cause(gen_cause) {}
Statement::StatStruct::StatStruct(DoStatement do_stat, minilua::Range range, GEN_CAUSE gen_cause)
    : stat_var(do_stat), range(std::move(range)), gen_cause(gen_cause) {}
Statement::StatStruct::StatStruct(Break break_stat, minilua::Range range, GEN_CAUSE gen_cause)
    : stat_var(break_stat), range(std::move(range)), gen_cause(gen_cause) {}

Statement::Statement(ts::Node node) : content(node) {
    if (!(node.type_id() == ts::NODE_EXPRESSION ||
          node.type_id() == ts::NODE_VARIABLE_DECLARATION ||
          node.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION ||
          node.type_id() == ts::NODE_DO_STATEMENT || node.type_id() == ts::NODE_IF_STATEMENT ||
          node.type_id() == ts::NODE_WHILE_STATEMENT ||
          node.type_id() == ts::NODE_REPEAT_STATEMENT || node.type_id() == ts::NODE_FOR_STATEMENT ||
          node.type_id() == ts::NODE_FOR_IN_STATEMENT ||
          node.type_id() == ts::NODE_GOTO_STATEMENT || node.type_id() == ts::NODE_BREAK_STATEMENT ||
          node.type_id() == ts::NODE_LABEL_STATEMENT || node.type_id() == ts::NODE_FUNCTION ||
          node.type_id() == ts::NODE_LOCAL_FUNCTION || node.type_id() == ts::NODE_FUNCTION_CALL ||
          (node.child(0).has_value() && node.child(0)->text() == ";"))) {
        throw std::runtime_error("Not a statement-node " + std::string(node.type()));
    }
}
Statement::Statement(IfStatement if_statement, GEN_CAUSE gen_cause)
    : content(StatStruct(if_statement, if_statement.range(), gen_cause)) {}
Statement::Statement(FunctionCall func_call, GEN_CAUSE gen_cause)
    : content(StatStruct(func_call, func_call.range(), gen_cause)) {}
Statement::Statement(WhileStatement while_statement, GEN_CAUSE gen_cause)
    : content(StatStruct(while_statement, while_statement.range(), gen_cause)) {}
Statement::Statement(VariableDeclaration var_dec, GEN_CAUSE gen_cause)
    : content(StatStruct(var_dec, var_dec.range(), gen_cause)) {}
Statement::Statement(DoStatement do_statement, GEN_CAUSE gen_cause)
    : content(StatStruct(do_statement, do_statement.range(), gen_cause)) {}
Statement::Statement(Break brk, minilua::Range range, GEN_CAUSE gen_cause)
    : content(StatStruct(brk, std::move(range), gen_cause)) {}
auto Statement::options() const -> StatementVariant {
    return std::visit(
        overloaded{
            [](ts::Node node) -> StatementVariant {
                if (node.type_id() == ts::NODE_EXPRESSION) {
                    return Expression(node.named_child(0).value());
                } else if (
                    node.type_id() == ts::NODE_VARIABLE_DECLARATION ||
                    node.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION) {
                    return VariableDeclaration(node);
                } else if (node.type_id() == ts::NODE_DO_STATEMENT) {
                    return DoStatement(node);
                } else if (node.type_id() == ts::NODE_IF_STATEMENT) {
                    return IfStatement(node);
                } else if (node.type_id() == ts::NODE_WHILE_STATEMENT) {
                    return WhileStatement(node);
                } else if (node.type_id() == ts::NODE_REPEAT_STATEMENT) {
                    return RepeatStatement(node);
                } else if (node.type_id() == ts::NODE_FOR_STATEMENT) {
                    return ForStatement(node);
                } else if (node.type_id() == ts::NODE_FOR_IN_STATEMENT) {
                    return ForInStatement(node);
                } else if (node.type_id() == ts::NODE_GOTO_STATEMENT) {
                    return GoTo(node);
                } else if (node.type_id() == ts::NODE_BREAK_STATEMENT) {
                    return Break();
                } else if (node.type_id() == ts::NODE_LABEL_STATEMENT) {
                    return Label(node);
                } else if (
                    node.type_id() == ts::NODE_FUNCTION ||
                    node.type_id() == ts::NODE_LOCAL_FUNCTION) {
                    return FunctionStatement(node);
                } else if (node.type_id() == ts::NODE_FUNCTION_CALL) {
                    return FunctionCall(node);
                } else {
                    throw std::runtime_error("Not a statement-node");
                }
            },
            [](const StatStruct& stat_struct) -> StatementVariant { return stat_struct.stat_var; }},
        this->content);
}
auto Statement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) { return convert_range(node.range()); },
            [](const StatStruct& stat_struct) { return stat_struct.range; }},
        this->content);
}
auto Statement::debug_print() const -> std::string {
    std::string class_name = "statement";
    return std::visit(
        overloaded{
            [class_name](const StatStruct& stat_struct) {
                return ast_class_to_string(class_name, stat_struct.range, stat_struct.gen_cause);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}

Literal::Literal(LiteralType type, std::string string, minilua::Range range)
    : literal_content(std::move(string)), literal_type(type), literal_range(std::move(range)) {}
auto Literal::type() const -> LiteralType { return this->literal_type; }
auto Literal::content() const -> std::string { return this->literal_content; }
auto Literal::range() const -> minilua::Range { return this->literal_range; }
} // namespace minilua::details::ast
