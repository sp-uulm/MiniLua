#include "ast.hpp"
#include "MiniLua/values.hpp"
#include <cassert>
#include <iostream>
#include <sstream>
#include <tree_sitter/tree_sitter.hpp>
#include <utility>

namespace minilua::details::ast {
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
        os << "desugar";
    case FOR_IN_LOOP_DESUGAR:
        break;
    case FUNCTION_STATEMENT_DESUGAR:
        break;
    case PLACEHOLDER:
        break;
    }
    return os;
}
static auto ast_class_to_string(const std::string& name, minilua::Range range) -> std::string {
    std::stringstream ss;
    ss << "(" << name << " " << range << ")";
    return ss.str();
}
static auto ast_class_to_string(const std::string& name, minilua::Range range, GEN_CAUSE cause)
    -> std::string {
    std::stringstream ss;
    ss << "(" << name << " " << range << "generated cause:" << cause << ")";
    return ss.str();
}
static auto
ast_class_to_string(const std::string& name, minilua::Range range, const std::string& content)
    -> std::string {
    std::stringstream ss;
    ss << "(" << name << " " << range << "|" << content << ")";
    return ss.str();
}
static auto ast_class_to_string(
    const std::string& name, minilua::Range range, const std::string& content, GEN_CAUSE cause)
    -> std::string {
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
Identifier::Identifier(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_IDENTIFIER && node.type_id() != ts::NODE_METHOD &&
        node.type_id() != ts::NODE_PROPERTY_IDENTIFIER &&
        node.type_id() != ts::NODE_FUNCTION_NAME_FIELD) {
        throw std::runtime_error("not an identifier node" + to_string(node.type_id()));
    }
}
Identifier::Identifier(const std::string& str, Range range)
    : content(std::make_tuple(str, range)) {}
auto Identifier::string() const -> std::string {
    return std::visit(
        overloaded{
            [](ts::Node node) -> std::string { return node.text(); },
            [](IdTuple tuple) -> std::string { return std::get<STRING>(tuple); }},
        this->content);
}
auto Identifier::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](IdTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto Identifier::debug_print() const -> std::string {
    return std::visit(
        overloaded{
            [this](const IdTuple& /*tuple*/) {
                return ast_class_to_string(
                    "identifier", this->range(), this->string(), GEN_CAUSE::FOR_LOOP_DESUGAR);
            },
            [this](ts::Node /*node*/) {
                return ast_class_to_string("identifier", this->range(), this->string());
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
BinaryOperation::BinaryOperation(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_BINARY_OPERATION) {
        throw std::runtime_error("not a binary_operation node");
    }
    assert(node.child_count() == 3);
}
BinaryOperation::BinaryOperation(
    const Expression& left, BinOpEnum op_enum, const Expression& right, minilua::Range range)
    : content(std::make_tuple(
          std::make_shared<Expression>(left), op_enum, std::make_shared<Expression>(right),
          range)) {}
auto BinaryOperation::left() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression { return Expression(node.child(0).value()); },
            [](BinOpTuple tuple) -> Expression { return *std::get<LEFT>(tuple); }},
        this->content);
}
auto BinaryOperation::right() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression { return Expression(node.child(2).value()); },
            [](BinOpTuple tuple) -> Expression { return *std::get<RIGHT>(tuple); }},
        this->content);
}
auto BinaryOperation::binary_operator() const -> BinOpEnum {
    return std::visit(
        overloaded{
            [](ts::Node node) -> BinOpEnum {
                std::string op_str = node.child(1)->text();
                if (op_str == "+"s) {
                    return BinOpEnum::ADD;
                } else if (op_str == "-"s) {
                    return BinOpEnum::SUB;
                } else if (op_str == "/"s) {
                    return BinOpEnum::DIV;
                } else if (op_str == "*"s) {
                    return BinOpEnum::MUL;
                } else if (op_str == "%"s) {
                    return BinOpEnum::MOD;
                } else if (op_str == "^"s) {
                    return BinOpEnum::POW;
                } else if (op_str == "<"s) {
                    return BinOpEnum::LT;
                } else if (op_str == ">"s) {
                    return BinOpEnum::GT;
                } else if (op_str == "<="s) {
                    return BinOpEnum::LEQ;
                } else if (op_str == ">="s) {
                    return BinOpEnum::GEQ;
                } else if (op_str == "==") {
                    return BinOpEnum::EQ;
                } else if (op_str == "~="s) {
                    return BinOpEnum::NEQ;
                } else if (op_str == ".."s) {
                    return BinOpEnum::CONCAT;
                } else if (op_str == "and"s) {
                    return BinOpEnum::AND;
                } else if (op_str == "or"s) {
                    return BinOpEnum::OR;
                } else if (op_str == "<<"s) {
                    return BinOpEnum::SHIFT_LEFT;
                } else if (op_str == ">>"s) {
                    return BinOpEnum::SHIFT_RIGHT;
                } else if (op_str == "//"s) {
                    return BinOpEnum::INT_DIV;
                } else if (op_str == "|"s) {
                    return BinOpEnum::BIT_OR;
                } else if (op_str == "&"s) {
                    return BinOpEnum::BIT_AND;
                } else if (op_str == "~"s) {
                    return BinOpEnum::BIT_XOR;
                } else {
                    throw std::runtime_error("Unknown Binary Operator: " + op_str);
                }
            },
            [](BinOpTuple tuple) -> BinOpEnum { return std::get<OPERATOR>(tuple); }},
        this->content);
}
auto BinaryOperation::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](BinOpTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto BinaryOperation::debug_print() const -> std::string {
    return std::visit(
        overloaded{
            [this](const BinOpTuple& /*tuple*/) {
                return ast_class_to_string(
                    "binary_operation", this->range(), GEN_CAUSE::PLACEHOLDER);
            },
            [this](ts::Node /*node*/) {
                return ast_class_to_string("binary_operation", this->range());
            }},
        this->content);
}
// UnaryOperation
UnaryOperation::UnaryOperation(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_UNARY_OPERATION) {
        throw std::runtime_error("not an unary_operation node");
    }
    assert(node.child_count() == 2);
}
UnaryOperation::UnaryOperation(UnOpEnum op_enum, const Expression& exp, minilua::Range range)
    : content(std::make_tuple(op_enum, std::make_shared<Expression>(exp), range)) {}
auto UnaryOperation::unary_operator() const -> UnOpEnum {
    return std::visit(
        overloaded{
            [](ts::Node node) -> UnOpEnum {
                if (node.child(0)->text() == "not"s) {
                    return UnOpEnum::NOT;
                } else if (node.child(0)->text() == "-"s) {
                    return UnOpEnum::NEG;
                } else if (node.child(0)->text() == "~"s) {
                    return UnOpEnum::BWNOT;
                } else if (node.child(0)->text() == "#"s) {
                    return UnOpEnum::LEN;
                } else {
                    throw std::runtime_error("unknown Unary Operator: " + node.child(0)->text());
                }
            },
            [](UnOpTuple tuple) -> UnOpEnum { return std::get<OPERATOR>(tuple); }},
        this->content);
}

auto UnaryOperation::expression() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression { return Expression(node.child(1).value()); },
            [](UnOpTuple tuple) -> Expression { return *std::get<OPERAND>(tuple); }},
        this->content);
}
auto UnaryOperation::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](UnOpTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto UnaryOperation::debug_print() const -> std::string {
    std::string class_name = "unary_operation";
    return std::visit(
        overloaded{
            [class_name](UnOpTuple tuple) {
                return ast_class_to_string(
                    class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
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
WhileStatement::WhileStatement(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_WHILE_STATEMENT) {
        throw std::runtime_error("not a while_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}
WhileStatement::WhileStatement(const Expression& cond, const Body& body, minilua::Range range)
    : content(std::make_tuple(
          std::make_shared<Expression>(cond), std::make_shared<Body>(body), range)) {}
auto WhileStatement::body() const -> Body {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Body {
                std::vector<ts::Node> body = node.named_children();
                body.erase(body.begin());
                return Body(body);
            },
            [](WhileTuple tuple) -> Body { return *std::get<BODY>(tuple); }},
        this->content);
}
auto WhileStatement::repeat_conditon() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression {
                return Expression(node.named_child(0).value().named_child(0).value());
            },
            [](WhileTuple tuple) -> Expression { return *std::get<CONDITION>(tuple); }},
        this->content);
}

auto WhileStatement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](WhileTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto WhileStatement::debug_print() const -> std::string {
    std::string class_name = "while_statement";
    return std::visit(
        overloaded{
            [class_name](WhileTuple tuple) {
                return ast_class_to_string(
                    class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
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
IfStatement::IfStatement(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_IF_STATEMENT) {
        throw std::runtime_error("not an if_statement node");
    }
    assert(
        node.named_child(0).has_value() &&
        node.named_child(0).value().type_id() == ts::NODE_CONDITION_EXPRESSION);
}
IfStatement::IfStatement(const Expression& cond, const Body& body, minilua::Range range)
    : content(std::make_tuple(
          std::make_shared<Expression>(cond), std::make_shared<Body>(body), range)) {}
auto IfStatement::condition() const -> Expression {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Expression {
                return Expression(node.named_child(0).value().named_child(0).value());
            },
            [](IfTuple tuple) -> Expression { return *std::get<CONDITION>(tuple); }},
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
            [](const IfTuple& /*tuple*/) -> std::optional<Else> { return nullopt; }},
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
            [](const IfTuple& /*tuple*/) -> std::vector<ElseIf> { return std::vector<ElseIf>(); }},
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
            [](IfTuple tuple) -> Body { return *std::get<BODY>(tuple); }},
        this->content);
}
auto IfStatement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](IfTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto IfStatement::debug_print() const -> std::string {
    std::string class_name = "if_statement";
    return std::visit(
        overloaded{
            [class_name](IfTuple tuple) {
                return ast_class_to_string(
                    class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
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
    const std::vector<Expression>& v_declarations, minilua::Range range)
    : content(std::make_tuple(v_declarators, v_declarations, range)), local_dec(local) {}
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
            [](VDTuple tuple) -> std::vector<Expression> { return std::get<DECLARATIONS>(tuple); }},
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
            [](VDTuple tuple) -> std::vector<VariableDeclarator> {
                return std::get<DECLARATORS>(tuple);
            }},
        this->content);
}
auto VariableDeclaration::local() const -> bool { return this->local_dec; }
auto VariableDeclaration::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](VDTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto VariableDeclaration::debug_print() const -> std::string {
    std::string class_name = "variable_declaration";
    return std::visit(
        overloaded{
            [class_name](VDTuple tuple) {
                return ast_class_to_string(
                    class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}

// VariableDeclarator
VariableDeclarator::VariableDeclarator(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_VARIABLE_DECLARATOR && node.type_id() != ts::NODE_IDENTIFIER &&
        node.type_id() != ts::NODE_FIELD_EXPRESSION && node.type_id() != ts::NODE_TABLE_INDEX) {
        throw std::runtime_error("not a variable declarator");
    }
}
VariableDeclarator::VariableDeclarator(const Identifier& id)
    : content(std::make_tuple(id, id.range())) {}
VariableDeclarator::VariableDeclarator(const FieldExpression& fe)
    : content(std::make_tuple(fe, fe.range())) {}
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
            [](VDTuple tuple) -> VarDecVariant { return std::get<VD_VARIANT>(tuple); }},
        this->content);
}
auto VariableDeclarator::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) { return convert_range(node.range()); },
            [](VDTuple tuple) { return std::get<RANGE>(tuple); }},
        this->content);
}
auto VariableDeclarator::debug_print() const -> std::string {
    std::string class_name = "variable_declarator";
    auto var = this->options();
    if (auto* id = std::get_if<Identifier>(&var)) {
        auto id_text = id->string();
        return std::visit(
            overloaded{
                [class_name, id_text](VDTuple tuple) {
                    return ast_class_to_string(
                        class_name, std::get<RANGE>(tuple), id_text, GEN_CAUSE::PLACEHOLDER);
                },
                [class_name, id_text](ts::Node node) {
                    return ast_class_to_string(class_name, convert_range(node.range()), id_text);
                }},
            this->content);
    } else {
        return std::visit(
            overloaded{
                [class_name](VDTuple tuple) {
                    return ast_class_to_string(
                        class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
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
DoStatement::DoStatement(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_DO_STATEMENT) {
        throw std::runtime_error("not a do_statement node");
    }
}
DoStatement::DoStatement(const Body& body, minilua::Range range)
    : content(std::make_tuple(std::make_shared<Body>(body), range)) {}
auto DoStatement::body() const -> Body {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Body { return Body(node.named_children()); },
            [](const DoTuple& tuple) -> Body { return *std::get<BODY>(tuple); }},
        this->content);
}
auto DoStatement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const DoTuple& tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto DoStatement::debug_print() const -> std::string {
    stringstream ss;
    ss << "(do_statement " << range();
    if (holds_alternative<DoTuple>(this->content)) {
        ss << " artificially generated"
           << "desugar";
    };
    ss << ")";
    return ss.str();
}

// FieldExpression
FieldExpression::FieldExpression(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_FIELD_EXPRESSION) {
        throw std::runtime_error("not a field_expression node");
    }
    assert(node.named_child_count() == 2);
}
FieldExpression::FieldExpression(
    const Prefix& prefix, const Identifier& identifier, minilua::Range range)
    : content(std::make_tuple(std::make_shared<Prefix>(prefix), identifier, range)) {}
auto FieldExpression::table_id() const -> Prefix {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Prefix { return Prefix(node.named_child(0).value()); },
            [](const FieldExpTuple& tuple) -> Prefix { return *std::get<TABLE>(tuple); }},
        this->content);
}
auto FieldExpression::property_id() const -> Identifier {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Identifier { return Identifier(node.named_child(1).value()); },
            [](const FieldExpTuple& tuple) -> Identifier { return std::get<PROPERTY>(tuple); }},
        this->content);
}
auto FieldExpression::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const FieldExpTuple& tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto FieldExpression::debug_print() const -> std::string {
    std::string class_name = "field_expression";
    return std::visit(
        overloaded{
            [class_name](FieldExpTuple tuple) {
                return ast_class_to_string(
                    class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
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
Parameters::Parameters(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_PARAMETERS) {
        throw std::runtime_error("not a parameters node");
    }
}
Parameters::Parameters(const std::vector<Identifier>& params, bool spread, minilua::Range range)
    : content(std::make_tuple(params, spread, range)) {}
auto Parameters::spread() const -> bool {
    return std::visit(
        overloaded{
            [](ts::Node node) -> bool {
                return node.named_child_count() > 0 &&
                       ((node.named_child(node.named_child_count() - 1))->type_id() ==
                        ts::NODE_SPREAD);
            },
            [](ParamTuple tuple) { return std::get<SPREAD>(tuple); }},
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
            [](ParamTuple tuple) -> std::vector<Identifier> {
                return std::get<IDENTIFIERS>(tuple);
            }},
        this->content);
}
auto Parameters::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](ParamTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto Parameters::debug_print() const -> std::string {
    std::string class_name = "parameters";
    return std::visit(
        overloaded{
            [class_name](ParamTuple tuple) {
                return ast_class_to_string(
                    class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
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
FunctionDefinition::FunctionDefinition(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_FUNCTION_DEFINITION) {
        throw std::runtime_error("not a function_definition node");
    }
}
FunctionDefinition::FunctionDefinition(
    const Parameters& params, const Body& body, minilua::Range range)
    : content(std::tuple(params, std::make_shared<Body>(body), range)) {}
auto FunctionDefinition::parameters() const -> Parameters {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Parameters { return Parameters(node.named_child(0).value()); },
            [](const FuncDefTuple& tuple) -> Parameters { return std::get<PARAMETERS>(tuple); }},
        this->content);
}
auto FunctionDefinition::body() const -> Body {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Body {
                std::vector<ts::Node> children = node.named_children();
                return Body(std::vector<ts::Node>(children.begin() + 1, children.end()));
            },
            [](const FuncDefTuple& tuple) -> Body { return *std::get<BODY>(tuple); }},
        this->content);
}
auto FunctionDefinition::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](const FuncDefTuple& tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto FunctionDefinition::debug_print() const -> std::string {
    std::string class_name = "function_definition";
    return std::visit(
        overloaded{
            [class_name](FuncDefTuple tuple) {
                return ast_class_to_string(
                    class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
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
FunctionCall::FunctionCall(ts::Node node) : content(node) {
    if (node.type_id() != ts::NODE_FUNCTION_CALL) {
        throw runtime_error("not a function_call node");
    }
    assert(node.named_child_count() == 2 || node.named_child_count() == 3);
}
FunctionCall::FunctionCall(
    const Prefix& pfx, const std::optional<Identifier>& method, const std::vector<Expression>& args,
    minilua::Range range)
    : content(std::make_tuple(std::make_shared<Prefix>(pfx), method, args, range)) {}
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
            [](FuncCallTuple tuple) { return std::get<METHOD>(tuple); }},
        this->content);
}
auto FunctionCall::id() const -> Prefix {
    return std::visit(
        overloaded{
            [](ts::Node node) -> Prefix { return Prefix(node.named_child(0).value()); },
            [](FuncCallTuple tuple) -> Prefix { return *std::get<PREFIX>(tuple); }},
        this->content);
}
auto FunctionCall::args() const -> std::vector<Expression> {
    return std::visit(
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
            [](FuncCallTuple tuple) -> std::vector<Expression> { return std::get<ARGS>(tuple); }},
        this->content);
}
auto FunctionCall::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](FuncCallTuple tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto FunctionCall::debug_print() const -> std::string {
    std::string class_name = "function_call";
    return std::visit(
        overloaded{
            [class_name](FuncCallTuple tuple) {
                return ast_class_to_string(
                    class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}

// Table
Table::Table(ts::Node node) : table(node) {
    if (node.type_id() != ts::NODE_TABLE) {
        throw runtime_error("not a table node");
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
        throw runtime_error("not a field node");
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
Prefix::Prefix(ts::Node node) : content(node) {
    if (!(node.type_id() == ts::NODE_SELF || node.type_id() == ts::NODE_FUNCTION_CALL ||
          node.type_id() == ts::NODE_IDENTIFIER || node.type_id() == ts::NODE_FIELD_EXPRESSION ||
          node.type_id() == ts::NODE_TABLE_INDEX)) {
        throw std::runtime_error("Not a prefix-node");
    }
}
Prefix::Prefix(const VariableDeclarator& vd) : content(std::make_tuple(vd, vd.range())) {}
Prefix::Prefix(const FunctionCall& fc) : content(std::make_tuple(fc, fc.range())) {}
auto Prefix::options() const -> PrefixVariant {
    return std::visit(
        overloaded{
            [](ts::Node node) -> PrefixVariant {
                if (node.type_id() == ts::NODE_SELF) {
                    return Self();
                } else if (node.type_id() == ts::NODE_FUNCTION_CALL) {
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
            [](PrefixTuple tuple) -> PrefixVariant {
                auto mod_var = std::get<PFX_VARIANT>(tuple);
                return std::visit(
                    overloaded{
                        [](Self self) -> PrefixVariant { return self; },
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
            [](PrefixTuple tuple) { return std::get<RANGE>(tuple); }},
        this->content);
}
auto Prefix::debug_print() const -> std::string {
    std::string class_name = "prefix";
    return std::visit(
        overloaded{
            [class_name](PrefixTuple tuple) {
                return ast_class_to_string(
                    class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
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
            [](PrefixTuple tuple) -> std::string {
                auto variant = std::get<TupleIndex::PFX_VARIANT>(tuple);
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
Expression::Expression(ts::Node node) : content(node) {
    if (!(node.type_id() == ts::NODE_SPREAD || node.type_id() == ts::NODE_FUNCTION_DEFINITION ||
          node.type_id() == ts::NODE_TABLE || node.type_id() == ts::NODE_BINARY_OPERATION ||
          node.type_id() == ts::NODE_UNARY_OPERATION || node.type_id() == ts::NODE_STRING ||
          node.type_id() == ts::NODE_NUMBER || node.type_id() == ts::NODE_NIL ||
          node.type_id() == ts::NODE_FALSE || node.type_id() == ts::NODE_TRUE ||
          node.type_id() == ts::NODE_IDENTIFIER || node.type_id() == ts::NODE_SELF ||
          node.type_id() == ts::NODE_FUNCTION_CALL || node.type_id() == ts::NODE_FIELD_EXPRESSION ||
          node.type_id() == ts::NODE_TABLE_INDEX || node.child(0)->text() == "(")) {
        throw std::runtime_error("Not an expression-node");
    }
}
Expression::Expression(const UnaryOperation& un) : content(make_tuple(un, un.range())) {}
Expression::Expression(const BinaryOperation& bin) : content(make_tuple(bin, bin.range())) {}
Expression::Expression(const FunctionDefinition& fd) : content(make_tuple(fd, fd.range())) {}
Expression::Expression(const Literal& literal) : content(make_tuple(literal, literal.range())) {}
Expression::Expression(const Identifier& id) : content(make_tuple(id, id.range())) {}
Expression::Expression(const Prefix& pfx) : content(make_tuple(pfx, pfx.range())) {}
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
                    node.type_id() == ts::NODE_SELF || node.type_id() == ts::NODE_FUNCTION_CALL ||
                    node.type_id() == ts::NODE_FIELD_EXPRESSION ||
                    node.type_id() == ts::NODE_TABLE_INDEX ||
                    (node.child(0).has_value() && node.child(0)->text() == "(")) {
                    return Prefix(node);
                } else {
                    throw std::runtime_error("Not an expression-node");
                }
            },
            [](ExpTup tuple) -> ExpressionVariant { return std::get<EXP_VARIANT>(tuple); }},
        this->content);
}
auto Expression::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) -> minilua::Range { return convert_range(node.range()); },
            [](ExpTup tuple) -> minilua::Range { return std::get<RANGE>(tuple); }},
        this->content);
}
auto Expression::debug_print() const -> std::string {
    std::string class_name = "expression";
    auto var = this->options();
    if (auto* id = std::get_if<Identifier>(&var)) {
        auto id_text = id->string();
        return std::visit(
            overloaded{
                [class_name, id_text](ExpTup tuple) {
                    return ast_class_to_string(
                        class_name, std::get<RANGE>(tuple), id_text, GEN_CAUSE::PLACEHOLDER);
                },
                [class_name, id_text](ts::Node node) {
                    return ast_class_to_string(class_name, convert_range(node.range()), id_text);
                }},
            this->content);
    } else if (auto* literal = std::get_if<Literal>(&var)) {
        auto literal_text = literal->content();
        return std::visit(
            overloaded{
                [class_name, literal_text](ExpTup tuple) {
                    return ast_class_to_string(
                        class_name, std::get<RANGE>(tuple), literal_text, GEN_CAUSE::PLACEHOLDER);
                },
                [class_name, literal_text](ts::Node node) {
                    return ast_class_to_string(
                        class_name, convert_range(node.range()), literal_text);
                }},
            this->content);
    } else {
        return std::visit(
            overloaded{
                [class_name](ExpTup tuple) {
                    return ast_class_to_string(
                        class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
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
            [](ExpTup tuple) {
                auto var = std::get<TupleIndex::EXP_VARIANT>(tuple);
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
Statement::Statement(const IfStatement& if_statement)
    : content(make_tuple(if_statement, if_statement.range())) {}
Statement::Statement(const FunctionCall& func_call)
    : content(make_tuple(func_call, func_call.range())) {}
Statement::Statement(const WhileStatement& while_statement)
    : content(make_tuple(while_statement, while_statement.range())) {}
Statement::Statement(const VariableDeclaration& var_dec)
    : content(make_tuple(var_dec, var_dec.range())) {}
Statement::Statement(const DoStatement& do_statement)
    : content(make_tuple(do_statement, do_statement.range())) {}
Statement::Statement(const Break& bk, minilua::Range range) : content(make_tuple(bk, range)) {}
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
                } else if (node.type_id() == ts::NODE_FUNCTION) {
                    return FunctionStatement(node);
                } else if (node.type_id() == ts::NODE_LOCAL_FUNCTION) {
                    return FunctionStatement(node);
                } else if (node.type_id() == ts::NODE_FUNCTION_CALL) {
                    return FunctionCall(node);
                } else {
                    throw std::runtime_error("Not a statement-node");
                }
            },
            [](StatementTuple tuple) -> StatementVariant { return std::get<STAT_VARIANT>(tuple); }},
        this->content);
}
auto Statement::range() const -> minilua::Range {
    return std::visit(
        overloaded{
            [](ts::Node node) { return convert_range(node.range()); },
            [](StatementTuple tuple) { return std::get<RANGE>(tuple); }},
        this->content);
}
auto Statement::debug_print() const -> std::string {
    std::string class_name = "statement";
    return std::visit(
        overloaded{
            [class_name](StatementTuple tuple) {
                return ast_class_to_string(
                    class_name, std::get<RANGE>(tuple), GEN_CAUSE::PLACEHOLDER);
            },
            [class_name](ts::Node node) {
                return ast_class_to_string(class_name, convert_range(node.range()));
            }},
        this->content);
}

Literal::Literal(LiteralType type, std::string string, minilua::Range range)
    : literal_content(std::move(string)), literal_type(type), literal_range(range){};
auto Literal::type() const -> LiteralType { return this->literal_type; }
auto Literal::content() const -> std::string { return this->literal_content; }
auto Literal::range() const -> minilua::Range { return this->literal_range; }
} // namespace minilua::details::ast